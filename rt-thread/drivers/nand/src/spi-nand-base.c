#include <spi_flash.h>
#include <kernel.h>
#include <bbm.h>
#include <bug.h>
#include <compiler.h>
#include <spi-nand.h>
#include <f_errno.h>

#define cond_resched() \
    do                 \
    {                  \
    } while (0)

int fh_start_debug = 0;
/*#define SPINAND_BBT_DEBUG*/

#define ONE_WIRE_SUPPORT (1 << 0)
#define DUAL_WIRE_SUPPORT (1 << 1)
#define QUAD_WIRE_SUPPORT (1 << 2)
#define MULTI_WIRE_SUPPORT (1 << 8)

static int spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo);

/**
 * __spi_nand_do_read_page - [INTERN] read data from flash to buffer
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @column :column address
 * @raw: without ecc or not
 * @corrected: how many bit error corrected
 *
 * read a page to buffer pointed by chip->buf
 */
static int __spi_nand_do_read_page(struct mtd_info *mtd, uint32_t page_addr,
                                   uint32_t colunm, bool raw, int *corrected)
{
    struct spi_nand_chip *chip = mtd->priv;
    int ret, ecc_error;
    uint8_t status;
    const nand_spi *spi = &chip->spi;

    RT_ASSERT(chip->user_data);
    RT_ASSERT(spi->user_data);

    DEBUG_TRACE("Enter %s\n", __FUNCTION__);

    /*read data from chip*/
    rt_memset(chip->buf, 0, chip->page_size + chip->page_spare_size);
    if (raw)
    {
        NAND_INFO("chip->disable_ecc\n");
        ret = chip->disable_ecc(chip);
        if (ret != 0)
        {
            NAND_INFO("disable ecc failed\n");
            return ret;
        }
    }
    ret = chip->load_page(chip, page_addr);
    if (ret != 0)
    {
        NAND_INFO("error %d loading page 0x%x to cache\n",
                  ret, page_addr);
        return ret;
    }
    ret = chip->waitfunc(chip, &status);
    if (ret != 0)
    {
        NAND_INFO("error %d waiting page 0x%x to cache\n",
                  ret, page_addr);
        return ret;
    }
    chip->get_ecc_status(chip, status, (unsigned int *)corrected, (unsigned int *)&ecc_error);
    /*
     * If there's an ECC error, print a message and notify MTD
     * about it. Then complete the read, to load actual data on
     * the buffer (instead of the status result).
     */
    if (ecc_error)
    {
        ECC_DEBUG_TRACE("internal ECC error reading page 0x%x with status 0x%02x\n",
                        page_addr, status);
        ECC_DEBUG_TRACE("ecc_status :0x%x\n", (status >> SPI_NAND_ECC_SHIFT) & chip->ecc_mask);
        mtd->ecc_stats.failed++;
        ECC_DEBUG_TRACE("mtd->ecc_stats.failed:%d\n", mtd->ecc_stats.failed);
    }
    else if (*corrected)
    {
        mtd->ecc_stats.corrected += *corrected;
        ECC_DEBUG_TRACE("mtd->ecc_status.corrected:%d page_addr:0x%x\n", mtd->ecc_stats.corrected, page_addr << chip->page_shift);
        ECC_DEBUG_TRACE("ecc_status :0x%x\n", (status >> SPI_NAND_ECC_SHIFT) & chip->ecc_mask);
    }
    /* Get page from the device cache into our internal buffer */
    ret = chip->read_cache(chip, page_addr, colunm,
                           chip->page_size + chip->page_spare_size - colunm,
                           chip->buf + colunm);
    if (ret != 0)
    {
        NAND_INFO("error %d reading page 0x%x from cache\n",
                  ret, page_addr);
        return ret;
    }
    if (raw)
    {
        ret = chip->enable_ecc(chip);
        if (ret != 0)
        {
            NAND_INFO("enable ecc failed\n");
            return ret;
        }
    }
    return 0;
}

/**
 * spi_nand_do_read_page - [INTERN] read a page from flash to buffer
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 * @corrected: how many bit error corrected
 *
 * read a page to buffer pointed by chip->buf
 */
static int spi_nand_do_read_page(struct mtd_info *mtd, uint32_t page_addr,
                                 bool raw, int *corrected)
{

    return __spi_nand_do_read_page(mtd, page_addr, 0, raw, corrected);
}

/**
 * spi_nand_do_read_page_oob - [INTERN] read page oob from flash to buffer
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 * @corrected: how many bit error corrected
 *
 * read page oob to buffer pointed by chip->oobbuf
 */
static int spi_nand_do_read_page_oob(struct mtd_info *mtd, uint32_t page_addr,
                                     bool raw, int *corrected)
{
    struct spi_nand_chip *chip = mtd->priv;

    return __spi_nand_do_read_page(mtd, page_addr, chip->page_size,
                                   raw, corrected);
}

/**
 * __spi_nand_do_write_page - [INTERN] write data from buffer to flash
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @column :column address
 * @raw: without ecc or not
 *
 * write data from buffer pointed by chip->buf to flash
 */
static int __spi_nand_do_write_page(struct mtd_info *mtd, uint32_t page_addr,
                                    uint32_t column, bool raw)
{
    struct spi_nand_chip *chip = mtd->priv;
    uint8_t status;
    bool p_fail = false;
    bool p_timeout = false;
    int ret = 0;

    DEBUG_TRACE("Enter %s, with buf \n", __FUNCTION__);

    if (raw)
    {
        ret = chip->disable_ecc(chip);
        if (ret != 0)
        {
            NAND_INFO("disable ecc failed\n");
            return ret;
        }
    }
    ret = chip->write_enable(chip);
    if (ret != 0)
    {
        NAND_INFO("write enable command failed\n");
        return ret;
    }
    /* Store the page to cache */
    ret = chip->store_cache(chip, page_addr, column,
                            chip->page_size + chip->page_spare_size - column,
                            chip->buf + column);
    if (ret != 0)
    {
        NAND_INFO("error %d storing page 0x%x to cache\n",
                  ret, page_addr);
        return ret;
    }
    /* Get page from the device cache into our internal buffer */
    ret = chip->write_page(chip, page_addr);
    if (ret != 0)
    {
        NAND_INFO("error %d reading page 0x%x from cache\n",
                  ret, page_addr);
        return ret;
    }
    ret = chip->waitfunc(chip, &status);
    if (ret != 0)
    {
        NAND_INFO("error %d write page 0x%x timeout\n",
                  ret, page_addr);
        return ret;
    }
    if ((status & STATUS_P_FAIL_MASK) == STATUS_P_FAIL)
    {
        NAND_INFO("program page 0x%x failed\n", page_addr);
        p_fail = true;
    }

    if ((status & STATUS_OIP_MASK) == STATUS_BUSY)
    {
        NAND_INFO("program page 0x%x timeout\n", page_addr);
        p_timeout = true;
    }
    if (raw)
    {
        ret = chip->enable_ecc(chip);
        if (ret != 0)
        {
            NAND_INFO("enable ecc failed\n");
            return ret;
        }
    }
    if ((p_fail == true) || (p_timeout == true))
        ret = -EIO;

    return ret;
}

/**
 * spi_nand_do_write_page - [INTERN] write page from buffer to flash
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 *
 * write page from buffer pointed by chip->buf to flash
 */
static int spi_nand_do_write_page(struct mtd_info *mtd, uint32_t page_addr,
                                  bool raw)
{
    return __spi_nand_do_write_page(mtd, page_addr, 0, raw);
}

/**
 * spi_nand_do_write_page_oob - [INTERN] write oob from buffer to flash
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 *
 * write oob from buffer pointed by chip->oobbuf to flash
 */
static int spi_nand_do_write_page_oob(struct mtd_info *mtd, uint32_t page_addr,
                                      bool raw)
{
    struct spi_nand_chip *chip = mtd->priv;

    return __spi_nand_do_write_page(mtd, page_addr, chip->page_size, raw);
}

/**
 * spi_nand_transfer_oob - [INTERN] Transfer oob to client buffer
 * @chip: SPI-NAND device structure
 * @oob: oob destination address
 * @ops: oob ops structure
 * @len: size of oob to transfer
 */
static void spi_nand_transfer_oob(struct spi_nand_chip *chip, uint8_t *oob,
                                  struct mtd_oob_ops *ops, size_t len)
{
    switch (ops->mode)
    {

    case MTD_OOB_PLACE: /*MTD_OPS_PLACE_OOB:*/
    case MTD_OOB_RAW:   /*MTD_OPS_RAW:*/
        rt_memcpy(oob, chip->oobbuf + ops->ooboffs, len);
        return;

    case MTD_OOB_AUTO:
    { /*MTD_OPS_AUTO_OOB:*/
        struct nand_oobfree *free = chip->ecclayout->oobfree;
        uint32_t boffs = 0, roffs = ops->ooboffs;
        size_t bytes = 0;

        for (; free->length && len; free++, len -= bytes)
        {
            /* Read request not from offset 0? */
            if (unlikely(roffs))
            {
                if (roffs >= free->length)
                {
                    roffs -= free->length;
                    continue;
                }
                boffs = free->offset + roffs;
                bytes = min_t(size_t, len,
                              (free->length - roffs));
                roffs = 0;
            }
            else
            {
                bytes = min_t(size_t, len, free->length);
                boffs = free->offset;
            }
            rt_memcpy(oob, chip->oobbuf + boffs, bytes);
            oob += bytes;
        }
        return;
    }
    default:
        BUG();
    }
}

/**
 * spi_nand_fill_oob - [INTERN] Transfer client buffer to oob
 * @chip: SPI-NAND device structure
 * @oob: oob data buffer
 * @len: oob data write length
 * @ops: oob ops structure
 */
static void spi_nand_fill_oob(struct spi_nand_chip *chip, uint8_t *oob,
                              size_t len, struct mtd_oob_ops *ops)
{
    DEBUG_TRACE("Enter %s\n", __FUNCTION__);
    rt_memset(chip->oobbuf, 0xff, chip->page_spare_size);

    switch (ops->mode)
    {

    case MTD_OOB_PLACE:
    case MTD_OOB_RAW:
        rt_memcpy(chip->oobbuf + ops->ooboffs, oob, len);
        return;

    case MTD_OOB_AUTO:
    {
        struct nand_oobfree *free = chip->ecclayout->oobfree;
        uint32_t boffs = 0, woffs = ops->ooboffs;
        size_t bytes = 0;

        for (; free->length && len; free++, len -= bytes)
        {
            /* Write request not from offset 0? */
            if (unlikely(woffs))
            {
                if (woffs >= free->length)
                {
                    woffs -= free->length;
                    continue;
                }
                boffs = free->offset + woffs;
                bytes = min_t(size_t, len,
                              (free->length - woffs));
                woffs = 0;
            }
            else
            {
                bytes = min_t(size_t, len, free->length);
                boffs = free->offset;
            }
            rt_memcpy(chip->oobbuf + boffs, oob, bytes);
            oob += bytes;
        }
        return;
    }
    default:
        BUG();
    }
}

/**
 * spi_nand_do_read_ops - [INTERN] Read data with ECC
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob ops structure
 *
 * Internal function. Called with chip held.
 */
static int spi_nand_do_read_ops(struct mtd_info *mtd, loff_t from,
                                struct mtd_oob_ops *ops)
{
    struct spi_nand_chip *chip = mtd->priv;
    int page_addr, page_offset, size;
    int ret;
    unsigned int corrected = 0;
    struct mtd_ecc_stats stats;
    unsigned int max_bitflips = 0;
    int readlen = ops->len;
    int oobreadlen = ops->ooblen;
    int ooblen = ops->mode == MTD_OOB_AUTO ? mtd->oobavail : mtd->oobsize;

    DEBUG_TRACE("Enter %s\n", __FUNCTION__);

    /* Do not allow reads past end of device */
    if (unlikely(from >= mtd->size))
    {
        NAND_INFO("%s: attempt to read beyond end of device\n",
                  __FUNCTION__);
        return -EINVAL;
    }
    stats = mtd->ecc_stats;

    page_addr = from >> chip->page_shift;

    /* for main data */
    page_offset = from & chip->page_mask;
    ops->retlen = 0;

    /* for oob */
    if (oobreadlen > 0)
    {
        if (unlikely(ops->ooboffs >= ooblen))
        {
            NAND_INFO("%s: attempt to start read outside oob\n",
                      __FUNCTION__);
            return -EINVAL;
        }

        if (unlikely(ops->ooboffs + oobreadlen >
                     ((mtd->size >> chip->page_shift) - (from >> chip->page_shift)) * ooblen))
        {
            NAND_INFO("%s: attempt to read beyond end of device\n",
                      __FUNCTION__);
            return -EINVAL;
        }
        ooblen -= ops->ooboffs;
        ops->oobretlen = 0;
    }

    while (1)
    {
        if (page_addr != chip->pagebuf || oobreadlen > 0)
        {
            ret = spi_nand_do_read_page(mtd, page_addr,
                                        ops->mode == MTD_OOB_RAW, (int *)&corrected);
            if (ret)
            {
                NAND_INFO("error %d reading page 0x%x\n",
                          ret, page_addr);
                return ret;
            }
            chip->pagebuf_bitflips = corrected;
            chip->pagebuf = page_addr;
        }
        max_bitflips = max(max_bitflips, chip->pagebuf_bitflips);
        size = min(readlen, chip->page_size - page_offset);
        rt_memcpy(ops->datbuf + ops->retlen,
                  chip->buf + page_offset, size);

        ops->retlen += size;
        readlen -= size;
        page_offset = 0;

        if (unlikely(ops->oobbuf))
        {
            size = min(oobreadlen, ooblen);
            spi_nand_transfer_oob(chip,
                                  ops->oobbuf + ops->oobretlen, ops, size);

            ops->oobretlen += size;
            oobreadlen -= size;
        }
        if (!readlen)
            break;
        DEBUG_TRACE("[%s:%d]page_addr:0x%08x\n", __FUNCTION__, __LINE__, page_addr << chip->page_shift);
        page_addr++;
    }

    if (mtd->ecc_stats.failed - stats.failed)
    {
        ECC_DEBUG_TRACE("mtd->ecc_stats.failed:%d  stats.failed:%d \n", mtd->ecc_stats.failed, stats.failed);
        return -EBADMSG;
    }

    return max_bitflips;
}

/**
 * spi_nand_do_write_ops - [INTERN] SPI-NAND write with ECC
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operations description structure
 *
 */
static int spi_nand_do_write_ops(struct mtd_info *mtd, loff_t to,
                                 struct mtd_oob_ops *ops)
{
    struct spi_nand_chip *chip = mtd->priv;
    int page_addr, page_offset, size;
    int writelen = ops->len;
    int oobwritelen = ops->ooblen;
    int ret;
    int ooblen = ops->mode == MTD_OOB_AUTO ? mtd->oobavail : mtd->oobsize;

    DEBUG_TRACE("Enter %s\n", __FUNCTION__);

    /* Do not allow reads past end of device */
    if (unlikely(to >= mtd->size))
    {
        NAND_INFO("%s: attempt to write beyond end of device\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    page_addr = to >> chip->page_shift;

    /* for main data */
    page_offset = to & chip->page_mask;
    ops->retlen = 0;

    /* for oob */
    if (oobwritelen > 0)
    {
        /* Do not allow write past end of page */
        if ((ops->ooboffs + oobwritelen) > ooblen)
        {
            NAND_INFO("%s: attempt to write past end of page\n",
                      __FUNCTION__);
            return -EINVAL;
        }

        if (unlikely(ops->ooboffs >= ooblen))
        {
            NAND_INFO("%s: attempt to start write outside oob\n",
                      __FUNCTION__);
            return -EINVAL;
        }

        if (unlikely(ops->ooboffs + oobwritelen >
                     ((mtd->size >> chip->page_shift) - (to >> chip->page_shift)) * ooblen))
        {
            NAND_INFO("%s: attempt to write beyond end of device\n",
                      __FUNCTION__);
            return -EINVAL;
        }
        ooblen -= ops->ooboffs;
        ops->oobretlen = 0;
    }

    chip->pagebuf = -1;

    while (1)
    {
        rt_memset(chip->buf, 0xFF,
                  chip->page_size + chip->page_spare_size);

        size = min(writelen, chip->page_size - page_offset);

        DEBUG_TRACE("[%s:%d]size:%d\n", __FUNCTION__, __LINE__, size);

        DEBUG_TRACE("[%s:%d]ops->datbuf + ops->retlen:0x%x\n", __FUNCTION__, __LINE__, *(ops->datbuf + ops->retlen));

        rt_memcpy(chip->buf + page_offset,
                  ops->datbuf + ops->retlen, size);

        ops->retlen += size;
        writelen -= size;
        page_offset = 0;

        if (unlikely(ops->oobbuf))
        {
            size = min(oobwritelen, ooblen);

            spi_nand_fill_oob(chip, ops->oobbuf + ops->oobretlen,
                              size, ops);

            ops->oobretlen += size;
            oobwritelen -= size;
        }
        DEBUG_TRACE("[%s:%d]page_addr:0x%08x\n", __FUNCTION__, __LINE__, page_addr << chip->page_shift);
        ret = spi_nand_do_write_page(mtd, page_addr,
                                     ops->mode == MTD_OOB_RAW);
        if (ret)
        {
            NAND_INFO("error %d writing page 0x%x\n",
                      ret, page_addr);
            return ret;
        }
        if (!writelen)
            break;
        DEBUG_TRACE("[%s:%d]page_addr:0x%x\n", __FUNCTION__, __LINE__, page_addr << chip->page_shift);
        page_addr++;
    }
    return 0;
}

/**
 * nand_read - [MTD Interface] SPI-NAND read
 * @mtd: MTD device structure
 * @from: offset to read from
 * @len: number of bytes to read
 * @retlen: pointer to variable to store the number of read bytes
 * @buf: the databuffer to put data
 *
 */
static int spi_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
                         size_t *retlen, u_char *buf)
{
    struct mtd_oob_ops ops = {0};
    struct spi_nand_chip *chip = mtd->priv;
    const nand_spi *spi = &chip->spi;
    int ret;

    if (spi->lock)
        spi->lock(spi);

    ops.len = len;
    ops.datbuf = buf;
    ret = spi_nand_do_read_ops(mtd, from, &ops);

    *retlen = ops.retlen;

    if (spi->unlock)
        spi->unlock(spi);

    return ret;
}

/**
 * spi_nand_write - [MTD Interface] SPI-NAND write
 * @mtd: MTD device structure
 * @to: offset to write to
 * @len: number of bytes to write
 * @retlen: pointer to variable to store the number of written bytes
 * @buf: the data to write
 *
 */
static int spi_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
                          size_t *retlen, const u_char *buf)
{
    struct mtd_oob_ops ops = {0};
    struct spi_nand_chip *chip = mtd->priv;
    const nand_spi *spi = &chip->spi;
    int ret;

    if (spi->lock)
        spi->lock(spi);

    ops.len = len;
    ops.datbuf = (uint8_t *)buf;

    ret = spi_nand_do_write_ops(mtd, to, &ops);

    *retlen = ops.retlen;

    if (spi->unlock)
        spi->unlock(spi);

    return ret;
}

/**
 * spi_nand_do_read_oob - [INTERN] SPI-NAND read out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operations description structure
 *
 * SPI-NAND read out-of-band data from the spare area.
 */
static int spi_nand_do_read_oob(struct mtd_info *mtd, loff_t from,
                                struct mtd_oob_ops *ops)
{
    struct spi_nand_chip *chip = mtd->priv;
    int page_addr;
    int corrected = 0;
    struct mtd_ecc_stats stats;
    int readlen = ops->ooblen;
    int len;
    int ret = 0;

    DEBUG_TRACE("Enter %s\n", __FUNCTION__);

    stats = mtd->ecc_stats;

    len = ops->mode == MTD_OOB_AUTO ? mtd->oobavail : mtd->oobsize;

    if (unlikely(ops->ooboffs >= len))
    {
        NAND_INFO("%s: attempt to start read outside oob\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(from >= mtd->size ||
                 ops->ooboffs + readlen > ((mtd->size >> chip->page_shift) -
                                           (from >> chip->page_shift)) *
                                              len))
    {
        NAND_INFO("%s: attempt to read beyond end of device\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    /* Shift to get page */
    page_addr = (from >> chip->page_shift);
    len -= ops->ooboffs;
    ops->oobretlen = 0;

    while (1)
    {
        /*read data from chip*/
        ret = spi_nand_do_read_page_oob(mtd, page_addr,
                                        ops->mode == MTD_OOB_RAW, &corrected);
        if (ret)
        {
            DEBUG_TRACE("error %d reading page 0x%x\n",
                        ret, page_addr);
            return ret;
        }
        if (page_addr == chip->pagebuf)
            chip->pagebuf = -1;

        len = min(len, readlen);
        spi_nand_transfer_oob(chip, ops->oobbuf + ops->oobretlen,
                              ops, len);

        readlen -= len;
        ops->oobretlen += len;
        if (!readlen)
            break;

        page_addr++;
    }

    if (ret < 0)
        return ret;

    if (mtd->ecc_stats.failed - stats.failed)
        return -EBADMSG;

    return mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}

/**
 * spi_nand_do_write_oob - [MTD Interface] SPI-NAND write out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 *
 * SPI-NAND write out-of-band.
 */
static int spi_nand_do_write_oob(struct mtd_info *mtd, loff_t to,
                                 struct mtd_oob_ops *ops)
{
    int page_addr, len, ret;
    struct spi_nand_chip *chip = mtd->priv;
    int writelen = ops->ooblen;

    DEBUG_TRACE("Enter %s\n", __FUNCTION__);

    DEBUG_TRACE("%s: to = 0x%08x, len = %i\n",
                __FUNCTION__, (unsigned int)to, (int)writelen);

    len = ops->mode == MTD_OOB_AUTO ? mtd->oobavail : mtd->oobsize;

    /* Do not allow write past end of page */
    if ((ops->ooboffs + writelen) > len)
    {
        NAND_INFO("%s: attempt to write past end of page\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    if (unlikely(ops->ooboffs >= len))
    {
        NAND_INFO("%s: attempt to start write outside oob\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    /* Do not allow write past end of device */
    if (unlikely(to >= mtd->size ||
                 ops->ooboffs + writelen >
                     ((mtd->size >> chip->page_shift) -
                      (to >> chip->page_shift)) *
                         len))
    {
        NAND_INFO("%s: attempt to write beyond end of device\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    /* Shift to get page */
    page_addr = to >> chip->page_shift;
    /* Invalidate the page cache, if we write to the cached page */
    if (page_addr == chip->pagebuf)
        chip->pagebuf = -1;

    spi_nand_fill_oob(chip, ops->oobbuf, writelen, ops);

    ret = spi_nand_do_write_page_oob(mtd, page_addr,
                                     ops->mode == MTD_OOB_RAW);
    if (ret)
    {
        NAND_INFO("error %d writing page 0x%x\n",
                  ret, page_addr);
        return ret;
    }
    ops->oobretlen = writelen;

    return 0;
}

/**
 * spi_nand_read_oob - [MTD Interface] SPI-NAND read data and/or out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operation description structure
 *
 * SPI-NAND read data and/or out-of-band data.
 */
static int spi_nand_read_oob(struct mtd_info *mtd, loff_t from,
                             struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;
    struct spi_nand_chip *chip = mtd->priv;
    const nand_spi *spi = &chip->spi;

    DEBUG_TRACE("Enter %s, from 0x%08x \n", __FUNCTION__, from);
    ops->retlen = 0;

    /* Do not allow reads past end of device */
    if (ops->datbuf && (from + ops->len) > mtd->size)
    {
        NAND_INFO("%s: attempt to read beyond end of device\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    if (spi->lock)
        spi->lock(spi);

    switch (ops->mode)
    {
    case MTD_OOB_PLACE:
    case MTD_OOB_AUTO:
    case MTD_OOB_RAW:
        break;

    default:
        goto out;
    }

    if (!ops->datbuf)
        ret = spi_nand_do_read_oob(mtd, from, ops);
    else
        ret = spi_nand_do_read_ops(mtd, from, ops);

out:
    if (spi->unlock)
        spi->unlock(spi);

    return ret;
}

/**
 * spi_nand_write_oob - [MTD Interface] SPI-NAND write data and/or out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 */
static int spi_nand_write_oob(struct mtd_info *mtd, loff_t to,
                              struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;
    struct spi_nand_chip *this = mtd->priv;
    const nand_spi *spi = &this->spi;
    DEBUG_TRACE("Enter %s\n", __FUNCTION__);

    ops->retlen = 0;

    /* Do not allow writes past end of device */
    if (ops->datbuf && (to + ops->len) > mtd->size)
    {
        NAND_INFO("%s: attempt to write beyond end of device\n",
                  __FUNCTION__);
        return -EINVAL;
    }

    if (spi->lock)
        spi->lock(spi);

    switch (ops->mode)
    {
    case MTD_OOB_PLACE:
    case MTD_OOB_AUTO:
    case MTD_OOB_RAW:
        break;

    default:
        goto out;
    }

    if (!ops->datbuf)
        ret = spi_nand_do_write_oob(mtd, to, ops);
    else
        ret = spi_nand_do_write_ops(mtd, to, ops);

out:
    if (spi->unlock)
        spi->unlock(spi);

    return ret;
}

/**
 * spi_nand_block_bad - [INTERN] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int spi_nand_block_bad(struct mtd_info *mtd, loff_t ofs)
{
    struct spi_nand_chip *chip = mtd->priv;
    struct mtd_oob_ops ops = {0};
    uint32_t block_addr;
    uint8_t bad[2] = {0, 0};
    uint8_t ret = 0;

    block_addr = ofs >> chip->block_shift;
    ops.mode = MTD_OOB_PLACE;
    ops.ooblen = 2;
    ops.oobbuf = bad;

    ret = spi_nand_do_read_oob(mtd, block_addr << chip->block_shift, &ops);
    if (bad[0] != 0xFF || bad[1] != 0xFF)
        ret = 1;

    return ret;
}

/**
 * spi_nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int spi_nand_block_checkbad(struct mtd_info *mtd, loff_t ofs,
                                   int allowbbt)
{
    struct spi_nand_chip *chip = mtd->priv;

    if (!chip->bbt)
        return spi_nand_block_bad(mtd, ofs);
    /* Return info from the table */
    return spi_nand_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * spi_nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int spi_nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
    struct spi_nand_chip *chip = mtd->priv;
    return chip->block_bad(mtd, offs, 0);
}

/**
 * spi_nand_block_markbad_lowlevel - mark a block bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This function performs the generic bad block marking steps (i.e., bad
 * block table(s) and/or marker(s)). We only allow the hardware driver to
 * specify how to write bad block markers to OOB (chip->block_markbad).
 *
 * We try operations in the following order:
 *  (1) erase the affected block, to allow OOB marker to be written cleanly
 *  (2) write bad block marker to OOB area of affected block (unless flag
 *      NAND_BBT_NO_OOB_BBM is present)
 *  (3) update the BBT
 * Note that we retain the first error encountered in (2) or (3), finish the
 * procedures, and dump the error in the end.
*/
static int spi_nand_block_markbad_lowlevel(struct mtd_info *mtd, loff_t ofs)
{
    struct spi_nand_chip *chip = mtd->priv;
    struct mtd_oob_ops ops = {0};
    struct erase_info einfo = {0};
    uint32_t block_addr;
    uint8_t buf[2] = {0, 0};
    int res, ret = 0;

    if (!(chip->bbt_options & NAND_BBT_NO_OOB_BBM))
    {
        /*erase bad block before mark bad block*/
        einfo.mtd = mtd;
        einfo.addr = ofs;
        einfo.len = 1UL << chip->block_shift;
        spi_nand_erase(mtd, &einfo);

        block_addr = ofs >> chip->block_shift;
        ops.mode = MTD_OOB_PLACE;
        ops.ooblen = 2;
        ops.oobbuf = buf;

        ret = spi_nand_do_write_oob(mtd,
                                    block_addr << chip->block_shift, &ops);
    }

    /* Mark block bad in BBT */
    if (chip->bbt)
    {
        res = spi_nand_markbad_bbt(mtd, ofs);
        if (!ret)
            ret = res;
    }

    if (!ret)
        mtd->ecc_stats.badblocks++;

    return ret;
}

/**
 * spi_nand_block_markbad - [MTD Interface] Mark block at the given offset
 * as bad
 * @mtd: MTD device structure
 * @ofs: offset relative to mtd start
 */
static int spi_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    int ret;

    ret = spi_nand_block_isbad(mtd, ofs);
    if (ret)
    {
        /* If it was bad already, return success and do nothing */
        if (ret > 0)
            return 0;
        return ret;
    }

    return spi_nand_block_markbad_lowlevel(mtd, ofs);
}

/**
 * __spi_nand_erase - [INTERN] erase block(s)
 * @mtd: MTD device structure
 * @einfo: erase instruction
 * @allowbbt: allow to access bbt
 *
 * Erase one ore more blocks
 */
int __spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo,
                     int allowbbt)
{
    struct spi_nand_chip *chip = mtd->priv;
    int page_addr, pages_per_block;
    loff_t len;
    uint8_t status;
    int ret = 0;

    /* check address align on block boundary */
    if (einfo->addr & (chip->block_size - 1))
    {
        NAND_INFO("%s: Unaligned address\n", __FUNCTION__);
        return -EINVAL;
    }

    if (einfo->len & (chip->block_size - 1))
    {
        NAND_INFO("%s: Length not block aligned\n", __FUNCTION__);
        return -EINVAL;
    }

    /* Do not allow erase past end of device */
    if ((einfo->len + einfo->addr) > chip->size)
    {
        NAND_INFO("%s: Erase past end of device\n", __FUNCTION__);
        return -EINVAL;
    }

    einfo->fail_addr = MTD_FAIL_ADDR_UNKNOWN;

    pages_per_block = 1 << (chip->block_shift - chip->page_shift);
    page_addr = einfo->addr >> chip->page_shift;
    len = einfo->len;

    einfo->state = MTD_ERASING;
    while (len)
    {
        /* Check if we have a bad block, we do not erase bad blocks! */
        if (chip->block_bad(mtd, ((loff_t)page_addr) << chip->page_shift, allowbbt))
        {
            NAND_INFO("%s: attempt to erase a bad block at page 0x%08x\n",
                      __FUNCTION__, page_addr);
            einfo->state = MTD_ERASE_FAILED;
            goto erase_exit;
        }
        /*
         * Invalidate the page cache, if we erase the block which
         * contains the current cached page.
         */
        if (page_addr <= chip->pagebuf && chip->pagebuf < (page_addr + pages_per_block))
            chip->pagebuf = -1;

        ret = chip->write_enable(chip);
        if (ret != 0)
        {
            NAND_INFO("write enable command failed\n");
            einfo->state = MTD_ERASE_FAILED;
            goto erase_exit;
        }

        ret = chip->erase_block(chip, page_addr);
        if (ret != 0)
        {
            NAND_INFO("block erase command failed\n");
            einfo->state = MTD_ERASE_FAILED;
            einfo->fail_addr = (loff_t)page_addr
                               << chip->page_shift;
            goto erase_exit;
        }

        ret = chip->waitfunc(chip, &status);
        if (ret != 0)
        {
            NAND_INFO("block erase command wait failed\n");
            einfo->state = MTD_ERASE_FAILED;
            goto erase_exit;
        }
        if ((status & STATUS_E_FAIL_MASK) == STATUS_E_FAIL)
        {
            NAND_INFO("erase block 0x%08x failed\n",
                      ((loff_t)page_addr) << chip->page_shift);
            einfo->state = MTD_ERASE_FAILED;
            einfo->fail_addr = (loff_t)page_addr
                               << chip->page_shift;
            goto erase_exit;
        }

        /* Increment page address and decrement length */
        len -= (1ULL << chip->block_shift);
        page_addr += pages_per_block;
    }

    einfo->state = MTD_ERASE_DONE;

erase_exit:

    ret = einfo->state == MTD_ERASE_DONE ? 0 : -EIO;

    /* Return more or less happy */
    return ret;
}

/**
 * spi_nand_erase - [MTD Interface] erase block(s)
 * @mtd: MTD device structure
 * @einfo: erase instruction
 *
 * Erase one ore more blocks
 */
static int spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo)
{
    struct spi_nand_chip *chip = mtd->priv;
    const nand_spi *spi = &chip->spi;
    int ret;

    if (spi->lock)
        spi->lock(spi);

    ret = __spi_nand_erase(mtd, einfo, 0);

    if (spi->unlock)
        spi->unlock(spi);

    return ret;
}

#ifdef RT_SFUD_USING_QSPI
static nand_err qspi_read(const struct __nand_spi *spi, uint32_t addr, nand_qspi_read_cmd_format *qspi_read_cmd_format,
                          uint8_t *read_buf, size_t read_size)
{
    struct rt_qspi_message message;
    nand_err result = NAND_SUCCESS;

    struct spi_nand_chip *chip = (struct spi_nand_chip *)(spi->user_data);
    struct rt_qspi_device *qspi_dev = (struct rt_qspi_device *)(chip->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(chip);
    RT_ASSERT(qspi_dev);

    /* set message struct */
    message.instruction.content = qspi_read_cmd_format->instruction;
    message.instruction.qspi_lines = qspi_read_cmd_format->instruction_lines;

    message.address.content = addr;
    message.address.size = qspi_read_cmd_format->address_size;
    message.address.qspi_lines = qspi_read_cmd_format->address_lines;

    message.alternate_bytes.content = 0;
    message.alternate_bytes.size = 0;
    message.alternate_bytes.qspi_lines = 0;

    message.dummy_cycles = qspi_read_cmd_format->dummy_cycles;

    message.parent.send_buf = RT_NULL;
    message.parent.recv_buf = read_buf;
    message.parent.length = read_size;
    message.parent.cs_release = 1;
    message.parent.cs_take = 1;
    message.qspi_data_lines = qspi_read_cmd_format->data_lines;

    if (rt_qspi_transfer_message(qspi_dev, &message) != read_size)
        result = NAND_ERR_TIMEOUT;

    return result;
}
#endif

static nand_err spi_nand_write_read(const nand_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
                                    size_t read_size)
{
    nand_err result = NAND_SUCCESS;
    RT_ASSERT(spi);
    struct spi_nand_chip *chip = (struct spi_nand_chip *)(spi->user_data);
    struct rt_spi_device *spi_device = (struct rt_spi_device *)(chip->user_data);
    RT_ASSERT(spi_device);
#ifdef RT_SFUD_USING_QSPI
    struct rt_qspi_device *qspi_dev = RT_NULL;
#endif
    if (write_size)
        RT_ASSERT(write_buf);
    if (read_size)
        RT_ASSERT(read_buf);
#ifdef RT_SFUD_USING_QSPI
    if (spi_device->bus->mode & RT_SPI_BUS_MODE_QSPI)
    {
        qspi_dev = (struct rt_qspi_device *)(spi_device);
        if (write_size && read_size)
        {
            if (rt_qspi_send_then_recv(qspi_dev, write_buf, write_size, read_buf, read_size) == 0)
                result = NAND_ERR_TIMEOUT;
        }
        else if (write_size)
        {
            if (rt_qspi_send(qspi_dev, write_buf, write_size) == 0)
                result = NAND_ERR_TIMEOUT;
        }
    }
    else
#endif
    {
        if (write_size && read_size)
        {
            if (rt_spi_send_then_recv(spi_device, write_buf, write_size, read_buf, read_size) != RT_EOK)
                result = NAND_ERR_TIMEOUT;
        }
        else if (write_size)
        {
            if (rt_spi_send(spi_device, write_buf, write_size) == 0)
                result = NAND_ERR_TIMEOUT;
        }
        else
        {
            if (rt_spi_recv(spi_device, read_buf, read_size) == 0)
                result = NAND_ERR_TIMEOUT;
        }
    }
    return result;
}

static void spi_lock(const nand_spi *spi)
{
    struct spi_nand_chip *chip = (struct spi_nand_chip *)(spi->user_data);
    RT_ASSERT(chip);

    rt_mutex_take(&(chip->chip_lock), RT_WAITING_FOREVER);
}

static void spi_unlock(const nand_spi *spi)
{
    struct spi_nand_chip *chip = (struct spi_nand_chip *)(spi->user_data);
    RT_ASSERT(chip);

    rt_mutex_release(&(chip->chip_lock));
}

static void retry_delay_100us(void)
{
    /* 100 microsecond delay */
    rt_thread_delay((RT_TICK_PER_SECOND * 1 + 9999) / 10000);
}

nand_err nand_spi_port_init(struct spi_nand_chip *chip)
{
    nand_err result = NAND_SUCCESS;

    RT_ASSERT(chip);

    /* port SPI device interface */
    chip->spi.wr = spi_nand_write_read;
#ifdef RT_SFUD_USING_QSPI
    chip->spi.qspi_read = qspi_read;
#endif
    chip->spi.lock = spi_lock;
    chip->spi.unlock = spi_unlock;
    chip->spi.user_data = chip;
    /* 100 microsecond delay */
    chip->retry.delay = retry_delay_100us;
    /* 60 seconds timeout */
    chip->retry.times = 60 * 10000;

    return result;
}

/*
 * spi_nand_read_status- send command 0x0f to the SPI-NAND status register value
 * @spi: spi device structure
 * @status: buffer to store value
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the
 *    busy status.
 *    This function is to allow reading the status of the command: read,
 *    write, and erase.
 *    Once the status turns to be ready, the other status bits also are
 *    valid status bits.
 */
static int spi_nand_read_status(const nand_spi *spi, uint8_t *status)
{
    nand_err ret;
    uint8_t cmd_data[4];

    cmd_data[0] = SPINAND_CMD_READ_REG;
    cmd_data[1] = REG_STATUS;
    cmd_data[2] = 0xff;
    cmd_data[3] = 0xff;
    ret = spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), status, sizeof(status));
    if (ret != 0)
        NAND_INFO("err: %d read status register\n", ret);
    return ret;
}

/**
 * spi_nand_get_otp- send command 0x0f to read the SPI-NAND OTP register
 * @spi: spi device structure
 * @opt: buffer to store value
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_get_otp(const nand_spi *spi, uint8_t *otp)
{
    int ret;
    uint8_t cmd_data[4];

    RT_ASSERT(spi->user_data);

    cmd_data[0] = SPINAND_CMD_READ_REG;
    cmd_data[1] = REG_OTP;
    cmd_data[2] = 0xff;
    cmd_data[3] = 0xff;

    ret = spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), otp, sizeof(otp));
    if (ret != 0)
        NAND_INFO("err: %d read status register\n", ret);
    return ret;
}

/**
 * spi_nand_set_otp- send command 0x1f to write the SPI-NAND OTP register
 * @spi: spi device structure
 * @status: buffer stored value
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_set_otp(const nand_spi *spi, uint8_t *otp)
{
    int ret;
    uint8_t cmd_data[4];

    cmd_data[0] = SPINAND_CMD_WRITE_REG;
    cmd_data[1] = REG_OTP;
    cmd_data[2] = otp[0];
    cmd_data[3] = otp[0];

    ret = spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), NULL, 0);
    if (ret < 0)
        NAND_INFO("error %d get otp\n", ret);
    return ret;
}

/**
 * spi_nand_enable_ecc- enable internal ECC
 * @chip: SPI-NAND device structure
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
int spi_nand_enable_ecc(struct spi_nand_chip *chip)
{
    nand_spi *spi = &chip->spi;
    int ret;
    uint8_t otp = 0;

    ret = spi_nand_get_otp(spi, &otp);
    if (ret < 0)
        return ret;

    if ((otp & OTP_ECC_MASK) == OTP_ECC_ENABLE)
        return 0;

    otp |= OTP_ECC_ENABLE;
    ret = spi_nand_set_otp(spi, &otp);
    if (ret < 0)
        return ret;

    return spi_nand_get_otp(spi, &otp);
}

static int spi_nand_get_qe_value(struct spi_nand_chip *chip, uint8_t *otp)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[4];
    int ret;

    cmd_data[0] = SPINAND_CMD_READ_REG;
    cmd_data[1] = chip->qe_addr;
    cmd_data[2] = 0xff;
    cmd_data[3] = 0xff;

    ret = spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), otp, sizeof(otp));
    return ret;
}

static int spi_nand_set_qe_value(struct spi_nand_chip *chip, uint8_t *otp)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[4];
    int ret;

    cmd_data[0] = SPINAND_CMD_WRITE_REG;
    cmd_data[1] = chip->qe_addr;
    cmd_data[2] = otp[0];
    cmd_data[3] = otp[0];

    ret = spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), NULL, 0);
    return ret;
}

static int spi_nand_set_qe(struct spi_nand_chip *chip)
{
    int ret;
    uint8_t otp = 0;

    if (!chip->qe_addr)
        return 0;

    ret = spi_nand_get_qe_value(chip, &otp);
    if (ret < 0)
        return ret;

    if (chip->qe_flag)
        otp |= chip->qe_mask;
    else
        otp &= (~chip->qe_mask);
    ret = spi_nand_set_qe_value(chip, &otp);
    if (ret < 0)
        return ret;
    return spi_nand_get_qe_value(chip, &otp);
}

/**
 * spi_nand_disable_ecc- disable internal ECC
 * @chip: SPI-NAND device structure
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_disable_ecc(struct spi_nand_chip *chip)
{
    const nand_spi *spi = &chip->spi;
    int ret;
    uint8_t otp = 0;

    ret = spi_nand_get_otp(spi, &otp);
    if (ret < 0)
        return ret;

    if ((otp & OTP_ECC_MASK) == OTP_ECC_ENABLE)
    {
        otp &= ~OTP_ECC_ENABLE;
        ret = spi_nand_set_otp(spi, &otp);
        if (ret < 0)
            return ret;
        return spi_nand_get_otp(spi, &otp);
    }
    else
        return 0;
}

/**
 * spi_nand_write_enable- send command 0x06 to enable write or erase the
 * Nand cells
 * @chip: SPI-NAND device structure
 * Description:
 *   Before write and erase the Nand cells, the write enable has to be set.
 *   After the write or erase, the write enable bit is automatically
 *   cleared (status register bit 2)
 *   Set the bit 2 of the status register has the same effect
 */
static int spi_nand_write_enable(struct spi_nand_chip *chip)
{
    nand_err result = NAND_SUCCESS;
    uint8_t cmd_data[1];
    const nand_spi *spi = &chip->spi;

    cmd_data[0] = SPINAND_CMD_WR_ENABLE;
    result = spi_nand_write_read(spi, cmd_data, 1, NULL, 0);
    return result;
}

/*
 * spi_nand_read_from_cache- send command 0x13 to read data from Nand to cache
 * @chip: SPI-NAND device structure
 * @page_addr: page to read
 */
static int spi_nand_read_page_to_cache(struct spi_nand_chip *chip, unsigned int page_addr)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[4];
    nand_err ret;

    cmd_data[0] = SPINAND_CMD_READ;
    cmd_data[1] = (uint8_t)(page_addr >> 16);
    cmd_data[2] = (uint8_t)(page_addr >> 8);
    cmd_data[3] = (uint8_t)page_addr;

    ret = spi_nand_write_read(spi, cmd_data, 4, NULL, 0);

    return ret;
}

/*
 * spi_nand_read_from_cache- send command 0x03 to read out the data from the
 * cache register
 * Description:
 *   The read can specify 1 to (page size + spare size) bytes of data read at
 *   the corresponding locations.
 *   No tRd delay.
 */
int spi_nand_read_from_cache(struct spi_nand_chip *chip, unsigned int page_addr,
                             unsigned int column, size_t len, uint8_t *rbuf)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[4];
    nand_err ret;

    cmd_data[0] = SPINAND_CMD_READ_RDM;
    if (chip->options & FIRST_DUMMY_BYTE)
    { /*FIXME: early GD chips, test 1G*/
        cmd_data[1] = 0;
        cmd_data[2] = (uint8_t)(column >> 8);
        if (chip->options & SPINAND_NEED_PLANE_SELECT)
            cmd_data[1] |= (uint8_t)(((page_addr >>
                                       (chip->block_shift - chip->page_shift)) &
                                      0x1)
                                     << 4);
        cmd_data[3] = (uint8_t)column;
    }
    else
    {
        cmd_data[1] = (uint8_t)(column >> 8);
        if (chip->options & SPINAND_NEED_PLANE_SELECT)
            cmd_data[2] |= (uint8_t)(((page_addr >>
                                       (chip->block_shift - chip->page_shift)) &
                                      0x1)
                                     << 4);
        cmd_data[2] = (uint8_t)column;
        cmd_data[3] = 0;
    }

    column = (cmd_data[1] << 16) | (cmd_data[2] << 8) | (cmd_data[3]);
#ifdef RT_SFUD_USING_QSPI
    if (chip->read_cmd_format.instruction != SPINAND_CMD_READ_RDM)
        ret = qspi_read(spi, column, (nand_qspi_read_cmd_format *)&chip->read_cmd_format, rbuf, len);
    else
#endif
        ret = spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), rbuf, len);

    return ret;
}

/*
 * spi_nand_read_from_cache_snor_protocol- send command 0x03 to read out the
 * data from the cache register, 0x03 command protocol is same as SPI NOR
 * read command
 * Description:
 *   The read can specify 1 to (page size + spare size) bytes of data read at
 *   the corresponding locations.
 *   No tRd delay.
 */
int spi_nand_read_from_cache_snor_protocol(struct spi_nand_chip *chip,
                                           uint32_t page_addr, uint32_t column, size_t len, uint8_t *rbuf)
{
    const nand_spi *spi = &chip->spi;
    DEBUG_TRACE("Enter %s\n", __FUNCTION__);
    uint8_t cmd_data[4];

    cmd_data[0] = SPINAND_CMD_READ_RDM;
    cmd_data[1] = 0;
    cmd_data[2] = (uint8_t)(column >> 8);
    if (chip->options & SPINAND_NEED_PLANE_SELECT)
        cmd_data[2] |= (uint8_t)(((page_addr >>
                                   (chip->block_shift - chip->page_shift)) &
                                  0x1)
                                 << 4);
    cmd_data[3] = (uint8_t)column;

    return spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), rbuf, len);
}
// EXPORT_SYMBOL(spi_nand_read_from_cache_snor_protocol);

/*
 * spi_nand_program_data_to_cache--to write a page to cache
 * @chip: SPI-NAND device structure
 * @page_addr: page to write
 * @column: the location to write to the cache
 * @len: number of bytes to write
 * wrbuf: buffer held @len bytes
 *
 * Description:
 *   The write command used here is 0x02--indicating that the cache is
 *   cleared first.
 *   Since it is writing the data to cache, there is no tPROG time.
 */
static int spi_nand_program_data_to_cache(struct spi_nand_chip *chip,
                                          unsigned int page_addr, unsigned int column, size_t len, uint8_t *wbuf)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[len + 3];
    size_t cmd_size;

    cmd_data[0] = SPINAND_CMD_PROG_LOAD;
    cmd_data[1] = (uint8_t)(column >> 8);
    if (chip->options & SPINAND_NEED_PLANE_SELECT)
        cmd_data[1] |= (uint8_t)(((page_addr >>(chip->block_shift - chip->page_shift)) &0x1)<< 4);
    cmd_data[2] = (uint8_t)column;
    cmd_size = 3;

    rt_memcpy(&cmd_data[cmd_size], wbuf, len);
    return spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), NULL, 0);
}

/**
 * spi_nand_program_execute--to write a page from cache to the Nand array
 * @chip: SPI-NAND device structure
 * @page_addr: the physical page location to write the page.
 *
 * Description:
 *   The write command used here is 0x10--indicating the cache is writing to
 *   the Nand array.
 *   Need to wait for tPROG time to finish the transaction.
 */
static int spi_nand_program_execute(struct spi_nand_chip *chip, unsigned int page_addr)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[4];

    cmd_data[0] = SPINAND_CMD_PROG;
    cmd_data[1] = (uint8_t)(page_addr >> 16);
    cmd_data[2] = (uint8_t)(page_addr >> 8);
    cmd_data[3] = (uint8_t)page_addr;

    return spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), NULL, 0);
}

/**
 * spi_nand_erase_block_erase--to erase a block
 * @chip: SPI-NAND device structure
 * @page_addr: the page to erase.
 *
 * Description:
 *   The command used here is 0xd8--indicating an erase command to erase
 *   one block
 *   Need to wait for tERS.
 */
static int spi_nand_erase_block(struct spi_nand_chip *chip,
                                uint32_t page_addr)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[4];

    cmd_data[0] = SPINAND_CMD_ERASE_BLK;
    cmd_data[1] = (uint8_t)(page_addr >> 16);
    cmd_data[2] = (uint8_t)(page_addr >> 8);
    cmd_data[3] = (uint8_t)page_addr;

    return spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), NULL, 0);
}

/**
 * spi_nand_wait - [DEFAULT] wait until the command is done
 * @chip: SPI-NAND device structure
 * @s: buffer to store status register(can be NULL)
 *
 * Wait for command done. This applies to erase and program only. Erase can
 * take up to 400ms and program up to 20ms.
 */

static int spi_nand_wait(struct spi_nand_chip *chip, uint8_t *s)
{
    const nand_spi *spi = &chip->spi;
    uint8_t status;
    int ret = -ETIMEDOUT;
    int time_out = 1000;

    do
    {
        spi_nand_read_status(spi, &status);
        if ((status & STATUS_OIP_MASK) == STATUS_READY)
        {
            ret = 0;
            goto out;
        }
        time_out--;
    } while ((status & (1 << 0)) && (time_out));

    if (time_out == 0)
    {
        rt_kprintf("flash in busy status time out");
        return ETIMEDOUT;
    }
out:
    if (s)
        *s = status;

    return ret;
}

/*
 * spi_nand_reset- send RESET command "0xff" to the SPI-NAND.
 * @chip: SPI-NAND device structure
 */
static int spi_nand_reset(struct spi_nand_chip *chip)
{
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[1];

    cmd_data[0] = SPINAND_CMD_RESET;

    if (spi_nand_write_read(spi, cmd_data, sizeof(cmd_data), NULL, 0) < 0)
        NAND_INFO("spi_nand reset failed!\n");

    /* elapse 1ms before issuing any other command */
    rt_thread_mdelay(1);

    return 0;
}

static int spi_nand_lock_block(struct spi_nand_chip *chip, uint8_t lock)
{
    const nand_spi *spi = &chip->spi;
    int ret;
    uint8_t cmd_data[4];

    cmd_data[0] = SPINAND_CMD_WRITE_REG;
    cmd_data[1] = REG_BLOCK_LOCK;
    cmd_data[2] = lock;
    cmd_data[3] = lock;

    ret = spi->wr(spi, cmd_data, sizeof(cmd_data), NULL, 0);
    if (ret != 0)
    {
        DEBUG_TRACE("error %d lock block\n", ret);
    }
    return ret;
}

int nand_read_id(struct spi_nand_chip *chip)
{
    int result;
    const nand_spi *spi = &chip->spi;
    uint8_t cmd_data[2], recv_data[4], tx_len, rx_len;
    extern void spi_nand_set_defaults(struct spi_nand_chip *chip);

    spi_nand_set_defaults(chip);
    result = chip->reset(chip);
    if (result != NAND_SUCCESS)
    {
        DEBUG_TRACE("reset chip fail result:%d\n", result);
        return result;
    }

    RT_ASSERT(spi->wr);

    cmd_data[0] = SPINAND_CMD_READ_ID;
    tx_len = 1;
    rx_len = 4;
    result = spi->wr(spi, cmd_data, tx_len, recv_data, rx_len);

    if (recv_data[0] == 0xff || recv_data[0] == 0x00)
    {
        /*add dummy byte 0x00*/
        tx_len = 2;
        cmd_data[1] = 0x00;
        result = spi->wr(spi, cmd_data, tx_len, recv_data, rx_len);
    }

    if (result == NAND_SUCCESS)
    {
        NAND_INFO("SPI-nand read ID is 0x%02X,0x%02X,0x%02X,0x%02X.",
                  recv_data[0], recv_data[1], recv_data[2], recv_data[3]);
    }

    result = spi_nand_scan_id_table(chip, recv_data);
    if (result == 0)
        return NAND_ERR_NOT_FOUND;

    chip->capacity = chip->size;
    chip->buf = rt_malloc(chip->page_size + chip->page_spare_size);
    rt_memset(chip->buf, 0, chip->page_size + chip->page_spare_size);
    if (!chip->buf)
        return -ENOMEM;

    DEBUG_TRACE("chip->page_size : 0x%08x\n", chip->page_size);
    chip->oobbuf = chip->buf + chip->page_size;
    result = spi_nand_lock_block(chip, BL_ALL_UNLOCKED);
    result = chip->enable_ecc(chip);
    if (result != 0)
    {
        DEBUG_TRACE("enable_ecc fai,result :%d\n", result);
    }

#ifdef CTL_QUAD_WIRE_SUPPORT
    result = chip->set_qe(chip);
    if (result != 0)
    {
        DEBUG_TRACE("set_qe fai,result :%d\n", result);
    }
#endif

    return result;
}

void spi_nand_set_defaults(struct spi_nand_chip *chip)
{
    /*struct spi_device *spi = chip->spi;*/

    /*if (spi->mode & SPI_RX_QUAD)
            chip->read_cache = spi_nand_read_from_cache_x4;
        else if (spi->mode & SPI_RX_DUAL)
            chip->read_cache = spi_nand_read_from_cache_x2;
        else*/
    chip->read_cache = spi_nand_read_from_cache;

    if (!chip->reset)
        chip->reset = spi_nand_reset;
    if (!chip->erase_block)
        chip->erase_block = spi_nand_erase_block;
    if (!chip->load_page)
        chip->load_page = spi_nand_read_page_to_cache;
    if (!chip->store_cache)
        chip->store_cache = spi_nand_program_data_to_cache;
    if (!chip->write_page)
        chip->write_page = spi_nand_program_execute;
    if (!chip->write_enable)
        chip->write_enable = spi_nand_write_enable;
    if (!chip->waitfunc)
        chip->waitfunc = spi_nand_wait;
    if (!chip->enable_ecc)
        chip->enable_ecc = spi_nand_enable_ecc;
    if (!chip->disable_ecc)
        chip->disable_ecc = spi_nand_disable_ecc;
    if (!chip->block_bad)
        chip->block_bad = spi_nand_block_checkbad;
    if (!chip->set_qe)
        chip->set_qe = spi_nand_set_qe;
}

int spi_nand_check(struct spi_nand_chip *chip)
{
    if (!chip->reset)
        return -ENODEV;
/*    if (!chip->read_id)
        return -ENODEV;*/
    if (!chip->load_page)
        return -ENODEV;
    if (!chip->read_cache)
        return -ENODEV;
    if (!chip->store_cache)
        return -ENODEV;
    if (!chip->write_page)
        return -ENODEV;
    if (!chip->erase_block)
        return -ENODEV;
    if (!chip->waitfunc)
        return -ENODEV;
    if (!chip->write_enable)
        return -ENODEV;
    if (!chip->get_ecc_status)
        return -ENODEV;
    if (!chip->enable_ecc)
        return -ENODEV;
    if (!chip->disable_ecc)
        return -ENODEV;
/*    if (!chip->ecclayout)
        return -ENODEV;*/

    return 0;
}

/**
 * spi_nand_scan_tail - [SPI-NAND Interface] Scan for the SPI-NAND device
 * @mtd: MTD device structure
 *
 * This is the second phase of the normal spi_nand_scan() function. It fills out
 * all the uninitialized function pointers with the defaults.
 */
int spi_nand_scan_tail(struct mtd_info *mtd)
{
    struct spi_nand_chip *chip = mtd->priv;
    int ret;

    ret = spi_nand_check(chip);
    if (ret)
        return ret;
    /* Initialize state */
    chip->state = FL_READY;
    /* Invalidate the pagebuffer reference */
    chip->pagebuf = -1;

    chip->bbt_options |= NAND_BBT_USE_FLASH;
    chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
    /*spin_lock_init(&chip->chip_lock);*/

    /*mtd->name = chip->name;*/
    mtd->size = chip->size;
    mtd->block_size = chip->block_size;
    mtd->sector_size = chip->page_size;
    mtd->oobsize = chip->page_spare_size;
/*    mtd->owner = THIS_MODULE;*/
    mtd->type = MTD_NANDFLASH;
    /*mtd->flags = MTD_CAP_NANDFLASH;*/
    /*xxx:porting down: if (!mtd->ecc_strength)
            mtd->ecc_strength = chip->ecc_strength_ds ?
                        chip->ecc_strength_ds : 1;*/
    mtd->bitflip_threshold = chip->bitflip_threshold ?
                chip->bitflip_threshold : 1;
/*    mtd->_ecclayout = chip->ecclayout;*/
    mtd->oobsize = chip->page_spare_size;
    mtd->oobavail = chip->ecclayout->oobavail;
    /* remove _* */
    mtd->_erase = spi_nand_erase;
    mtd->_read = spi_nand_read;
    mtd->_write = spi_nand_write;
    mtd->_read_oob = spi_nand_read_oob;
    mtd->_write_oob = spi_nand_write_oob;
    mtd->_block_isbad = spi_nand_block_isbad;
    mtd->_block_markbad = spi_nand_block_markbad;
#ifndef CONFIG_SPI_NAND_BBT
    /* Build bad block table */
    return spi_nand_default_bbt(mtd);
#else
    return 0;
#endif
}

/**
 * spi_nand_scan_ident_release - [SPI-NAND Interface] Free resources
 * applied by spi_nand_scan_ident
 * @mtd: MTD device structure
 */
int spi_nand_scan_ident_release(struct mtd_info *mtd)
{
    struct spi_nand_chip *chip = mtd->priv;

    rt_free(chip->buf);

    return 0;
}

/**
 * spi_nand_scan_tail_release - [SPI-NAND Interface] Free resources
 * applied by spi_nand_scan_tail
 * @mtd: MTD device structure
 */
int spi_nand_scan_tail_release(struct mtd_info *mtd)
{
    return 0;
}

/**
 * spi_nand_release - [SPI-NAND Interface] Free resources held by the SPI-NAND
 * device
 * @mtd: MTD device structure
 */
