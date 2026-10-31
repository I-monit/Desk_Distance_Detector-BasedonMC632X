
#include "ubi_rt_mtd.h"
#include "drivers/mtd_nand.h"
#include "string.h"
#ifdef UBI_TEST
#define DATA_CHECK
#endif
#ifdef DATA_CHECK
unsigned char *gCheckBuf = NULL;
static ubi_rt_mtd_info *gCheckMtd = NULL;
static rt_timer_t timer1;
static rt_uint8_t dataErrcount = 0;
static rt_uint8_t mtdread = 0;
static rt_uint8_t mtdwrite = 0;
static rt_uint8_t mtderase = 0;
#define set_read_in() \
    do                \
    {                 \
        mtdread++;    \
    } while (0)
#define set_read_out() \
    do                 \
    {                  \
        mtdread--;     \
    } while (0)
#define set_write_in() \
    do                 \
    {                  \
        mtdwrite++;    \
    } while (0)
#define set_write_out() \
    do                  \
    {                   \
        mtdwrite--;     \
    } while (0)
#define set_erase_in() \
    do                 \
    {                  \
        mtderase++;    \
    } while (0)
#define set_erase_out() \
    do                  \
    {                   \
        mtderase--;     \
    } while (0)
static void check_timeout(void *parameter)
{
    rt_kprintf("err:%d,r:%d,w:%d,e:%d\n", dataErrcount, mtdread, mtdwrite, mtderase);
    if (dataErrcount != 0)
    {
        rt_kprintf("data error\n");
    }
    if (mtdread != 0 && mtdread != 1)
    {
        rt_kprintf("mtdread error.%d\n", mtdread);
    }
    if (mtdwrite != 0 && mtdwrite != 1)
    {
        rt_kprintf("mtdwrite error.%d\n", mtdwrite);
    }
    if (mtderase != 0 && mtderase != 1)
    {
        rt_kprintf("mtderase error.%d\n", mtderase);
    }
}

void timer_control_init(void)
{
    /* 创建定时器1 */
    timer1 = rt_timer_create("checktimer",
                             check_timeout,           /* 超时时回调的处理函数 */
                             RT_NULL,                 /* 超时函数的入口参数 */
                             1000,                    /* 定时长度，以OS Tick为单位，即10个OS Tick */
                             RT_TIMER_FLAG_PERIODIC); /* 周期性定时器 */
    /* 启动定时器 */
    if (timer1 != RT_NULL)
        rt_timer_start(timer1);
}

void mtd_check_init(ubi_rt_mtd_info *mtd)
{
    if (gCheckBuf)
    {
        return;
    }
    gCheckBuf = rt_malloc(mtd->size);
    if (gCheckBuf == NULL)
    {
        rt_kprintf("malloc buf error\n");
        return;
    }
    rt_kprintf("init %s check\n", mtd->name);
    memset(gCheckBuf, 0xff, mtd->size);
    unsigned int ret = 0;
    ubirt_mtd_read(mtd, 0, mtd->size, &ret, gCheckBuf);
    gCheckMtd = mtd;
    timer_control_init();
    return;
}
int mtd_check_erase(ubi_rt_mtd_info *mtd, rt_uint32_t from, rt_uint32_t len)
{
    if (mtd != gCheckMtd)
    {
        return 0;
    }
    if (from + len > gCheckMtd->size)
    {
        rt_kprintf("overflow,%d,%d\n", from, len);
        return -1;
    }
    memset(gCheckBuf + from, 0xff, len);
    return 0;
}
int mtd_check_write(ubi_rt_mtd_info *mtd, rt_uint32_t to, rt_uint32_t len, const rt_uint8_t *buf)
{
    if (mtd != gCheckMtd)
    {
        return 0;
    }
    if (to + len > gCheckMtd->size)
    {
        rt_kprintf("overflow,%d,%d\n", to, len);
        return -1;
    }
    memcpy(gCheckBuf + to, buf, len);
    return 0;
}
int mtd_check_read(ubi_rt_mtd_info *mtd, rt_uint32_t from, rt_uint32_t len, rt_uint8_t *bufOfFlash)
{
    if (mtd != gCheckMtd)
    {
        return 0;
    }
    if (from + len > gCheckMtd->size)
    {
        rt_kprintf("overflow,%d,%d\n", from, len);
        return -1;
    }
    if (len == 0)
    {
        return -1;
    }
    if (memcmp(gCheckBuf + from, bufOfFlash, len) != 0)
    {
        rt_kprintf("data error at %x,%d\n", from, len);
        dataErrcount++;
        /* exit(-1); */
        return -1;
    }
    return 0;
}
#endif

#ifdef __FULLHAN___
#include "f_errno.h"
static int read_page_by_oobops(struct rt_mtd_nand_device *pNand, unsigned int from,
                               unsigned int len, unsigned int *retlen, unsigned char *buf)
{
    int errret = 0;
    struct mtd_oob_ops ops;
    memset(&ops, 0, sizeof(ops));
    ops.mode = MTD_OOB_AUTO;
    ops.datbuf = buf;
    ops.len = len;
    errret = pNand->ops->read_page(pNand, from, &ops);
    if (errret)
    {
        rt_kprintf("read_oob failed, from: 0x%x, mtd error %d",
                   from, errret);
    }
    *retlen = ops.retlen;
    switch (errret)
    {
    case 0:
        /* no error */
        break;

    case -EUCLEAN:
        /* MTD's ECC fixed the data */
        rt_kprintf("ecc correct data.0x%x\n", from);
        break;

    case -EBADMSG:
        *retlen = len; /* ubi/io.c,if not equal,set err code to -EIO */
    default:
        /* MTD's ECC could not fix the data */
        /* dev->n_ecc_unfixed++; */
        /* if(ecc_result) */
        /* *ecc_result = YAFFS_ECC_RESULT_UNFIXED; */
        /* return YAFFS_FAIL; */
        rt_kprintf("ecc error.addr:0x%x,error:%d\n", from, errret);
        break;
    }
    return errret;
}
static int write_page_by_oobops(struct rt_mtd_nand_device *pNand, unsigned int from,
                                unsigned int len, unsigned int *retlen, const unsigned char *buf)
{
    struct mtd_oob_ops ops;
    memset(&ops, 0, sizeof(ops));
    ops.mode = MTD_OOB_AUTO;
    ops.datbuf = (uint8_t *)buf;
    ops.len = len;
    int errret = pNand->ops->write_page(pNand, from, &ops);
    if (errret)
    {
        rt_kprintf("write_oob failed, from 0x%x, mtd error %d\n",
                   from, errret);
    }
    *retlen = ops.retlen;
    return errret;
}
#endif
int get_rt_mtd_id(struct rt_mtd_nand_device *pDev)
{
    struct rt_object *object;
    struct rt_list_node *node;
    struct rt_object_information *information;
    int index = 0;
    rt_device_t dev;
    /* enter critical */
    if (rt_thread_self() != RT_NULL)
        rt_enter_critical();

    /* try to find device object */
    information = rt_object_get_information(RT_Object_Class_Device);
    RT_ASSERT(information != RT_NULL);
    for (node = information->object_list.prev;
         node != &(information->object_list);
         node = node->prev)
    {
        object = rt_list_entry(node, struct rt_object, list);
        dev = RT_DEVICE(object);
        if (RT_Device_Class_Block == dev->type || RT_Device_Class_MTD == dev->type)
        {
            index++;
        }
        else
        {
            continue;
        }
        if ((void *)pDev == dev)
        {
            break;
        }
    }
    /* leave critical */
    if (rt_thread_self() != RT_NULL)
        rt_exit_critical();
    return index - 1;
}
int get_rt_mtd_dev(ubi_rt_mtd_info *mtd, struct rt_mtd_nand_device **pNand)
{
    if (mtd->pDevHandle != NULL)
    {
        *pNand = RT_MTD_NAND_DEVICE(mtd->pDevHandle);
        return 0;
    }
    *pNand = RT_MTD_NAND_DEVICE(rt_device_find(mtd->name));
    if (*pNand == RT_NULL)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return -1;
    }
    mtd->pDevHandle = (void *)*pNand;
    return 0;
}
int ubirt_mtd_read(ubi_rt_mtd_info *mtd, unsigned int from, unsigned int len, unsigned int *retlen, unsigned char *buf)
{
    struct rt_mtd_nand_device *pNand;
    if (get_rt_mtd_dev(mtd, &pNand) != 0 || pNand->ops == RT_NULL)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return -1;
    }
#ifdef DATA_CHECK
    set_read_in();
#endif
    rt_off_t page = from / mtd->writesize;
    rt_uint32_t offset = from % mtd->writesize;
    *retlen = 0;
    rt_err_t errret = 0;
    int ret = 0;
    if (offset != 0)
    {
        rt_uint8_t tmpbuf[mtd->writesize];
        memset(tmpbuf, 0, sizeof(tmpbuf));
#ifdef __FULLHAN___
        unsigned int ret_length = 0;
        ret = read_page_by_oobops(pNand, page * mtd->writesize, mtd->writesize, &ret_length, tmpbuf);
#else
        ret = pNand->ops->read_page(pNand, page, tmpbuf, mtd->writesize, NULL, 0);
#endif
        if (ret != 0 && errret != -EBADMSG)
        {
            rt_kprintf("mtd_read readpage error.0x%x\n", from);
            errret = ret;
        }
        rt_uint32_t validBufSize = mtd->writesize - offset;
        *retlen = validBufSize < len ? validBufSize : len;
        memcpy(buf, tmpbuf + offset, *retlen);
        page += 1;
    }

    rt_uint32_t leftLen = len - *retlen;
    while (leftLen >= mtd->writesize)
    {
/* char tmpbuf1[mtd->writesize]; */
#ifdef __FULLHAN___
        unsigned int ret_length = 0;
        ret = read_page_by_oobops(pNand, page * mtd->writesize, mtd->writesize, &ret_length, buf + *retlen);
#else
        ret = pNand->ops->read_page(pNand, page, buf + *retlen, mtd->writesize, NULL, 0);
#endif
        if (ret != 0 && errret != -EBADMSG)
        {
            rt_kprintf("mtd_read readpage error.0x%x\n", from);
            errret = ret;
        }
        *retlen += mtd->writesize;
        leftLen = len - *retlen;
        page++;
    }
    if (leftLen > 0)
    {
        rt_uint8_t tmpbuf1[mtd->writesize];
#ifdef __FULLHAN___
        unsigned int ret_length = 0;
        ret = read_page_by_oobops(pNand, page * mtd->writesize, mtd->writesize, &ret_length, tmpbuf1);
#else
        ret = pNand->ops->read_page(pNand, page, tmpbuf1, mtd->writesize, NULL, 0);
#endif
        if (ret != 0 && errret != -EBADMSG)
        {
            rt_kprintf("mtd_read readpage error.0x%x\n", from);
            errret = ret;
        }
        memcpy(buf + *retlen, tmpbuf1, leftLen);
        *retlen += leftLen;
        leftLen = len - *retlen;
    }
#ifdef DATA_CHECK
    mtd_check_read(mtd, from, *retlen, buf);
    set_read_out();
#endif
    /* if(mtd->index == 8 && from >= 0x200000 && from <= 0x300000) */
    /* { */
    /*        rt_kprintf("Euclean:0x%x\n",from);*/
    /* errret=-EUCLEAN; */
    /* } */
    /* static int ucleanHit = 0; */
    /* char hit = 0; */
    /* if (mtd->index == 8) */
    /* { */
    /* if (from >= 0x200000 && from <= 0x200800 && ((ucleanHit&0x01) == 0)) */
    /* { */
    /*                rt_kprintf("from = 0x%x\n",from);*/
    /* hit = 1; */
    /* ucleanHit |= 1; */
    /*                rt_kprintf("ucleanHit |= 1\n");*/
    /* } */
    /* else if (from >= 0x200800 && from <= 0x201000 && ((ucleanHit & 0x02) == 0)) */
    /* { */
    /*                rt_kprintf("from = 0x%x\n",from);*/
    /* hit = 1; */
    /* ucleanHit |= 2; */
    /*                rt_kprintf("ucleanHit |= 2\n");*/
    /* } */
    /* else if (from >= 0x201000 && from <= 0x201800 && ((ucleanHit & 0x04) == 0)) */
    /* { */
    /*                rt_kprintf("from = 0x%x\n",from);*/
    /* hit = 1; */
    /* ucleanHit |= 4; */
    /*                rt_kprintf("ucleanHit |= 4\n");*/
    /* } */
    /* else if (from >= 0x201800 && from <= 0x202000 && ((ucleanHit & 0x08) == 0)) */
    /* { */
    /*                rt_kprintf("from = 0x%x\n",from);*/
    /* hit = 1; */
    /* ucleanHit |= 8; */
    /*                rt_kprintf("ucleanHit |= 8\n");*/
    /* } */
    /* else if (from >= 0x202000 && from <= 0x202800 && ((ucleanHit & 0x10) == 0)) */
    /* { */
    /*                rt_kprintf("from = 0x%x\n",from);*/
    /* hit = 1; */
    /* ucleanHit |= 0x10; */
    /*                rt_kprintf("ucleanHit |= 0x10\n");*/
    /* } */
    /* else if (from >= 0x202800 && from <= 0x203000 && ((ucleanHit & 0x20) == 0)) */
    /* { */
    /*                rt_kprintf("from = 0x%x\n",from);*/
    /* hit = 1; */
    /* ucleanHit |= 0x20; */
    /*                rt_kprintf("ucleanHit |= 0x20\n");*/
    /* } */

    /* if (hit == 1) */
    /* { */
    /*                rt_kprintf("EUCLEAN.0x%x\n", from);*/
    /* errret = -EUCLEAN; */
    /* // errret = -77; */
    /* } */
    /* } */
    if (*retlen != len)
    {
        rt_kprintf("mtd_read error:from:%d,ret=%d,len=%d\n", from, *retlen, len);
        return errret;
    }
    return errret;
}
#ifdef UBI_TEST
#include <sys/time.h>
unsigned long mtd_writel = 0;
unsigned long mtd_writet = 0;
#endif
int ubirt_mtd_write(ubi_rt_mtd_info *mtd, unsigned int to, unsigned int len, unsigned int *retlen, const unsigned char *buf)
{
#ifdef UBI_TEST
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
#endif
    struct rt_mtd_nand_device *pNand;
    if (get_rt_mtd_dev(mtd, &pNand) != 0 || pNand->ops == RT_NULL)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return -1;
    }
#ifdef DATA_CHECK
    set_write_in();
#endif
    int ret = 0, errret = 0;
    rt_off_t page = to / mtd->writesize;
    rt_uint32_t offset = to % mtd->writesize;
    *retlen = 0;
    if (offset != 0)
    {
        rt_kprintf("offset =%d, to=%d,len=%d\n", offset, to, len);
        rt_uint8_t tmpbuf[mtd->writesize];
        memset(tmpbuf, 0, mtd->writesize);
        unsigned int needWriteSize = mtd->writesize - offset;
#ifdef __FULLHAN___
        unsigned int ret_length = 0;
        /* no need to read first. */
        /* errret = read_page_by_oobops(pNand,page*mtd->writesize,mtd->writesize,&ret_length,tmpbuf); */
        memcpy(tmpbuf + offset, buf, needWriteSize);
        write_page_by_oobops(pNand, page * mtd->writesize, mtd->writesize, &ret_length, tmpbuf);
#else
        /* errret = pNand->ops->read_page(pNand, page, tmpbuf, mtd->writesize, NULL, 0); */
        memcpy(tmpbuf + offset, buf, needWriteSize);
        pNand->ops->write_page(pNand, page, tmpbuf, mtd->writesize, NULL, 0);
#endif
        page++;
        *retlen += needWriteSize;
    }
    rt_uint32_t leftLen = len - *retlen;
    while (leftLen >= mtd->writesize)
    { /* no need to read bf write.nand must erase before write. */
#ifdef __FULLHAN___
        unsigned int ret_length = 0;
        ret = write_page_by_oobops(pNand, page * mtd->writesize, mtd->writesize, &ret_length, buf + *retlen);
#else
        ret = pNand->ops->write_page(pNand, page, buf + *retlen, mtd->writesize, NULL, 0);
#endif
        if (ret != 0)
        {
            rt_kprintf("mtd_write writepage error.0x%x\n", to);
            errret = ret;
        }
        *retlen += mtd->writesize;
        leftLen = len - *retlen;
        page++;
    }
    if (leftLen > 0)
    {
        rt_uint8_t tmpbuf1[mtd->writesize];
        memset(tmpbuf1, 0, mtd->writesize);
        memcpy(tmpbuf1, buf + *retlen, leftLen);
#ifdef __FULLHAN___
        unsigned int ret_length = 0;
        ret = write_page_by_oobops(pNand, page * mtd->writesize, mtd->writesize, &ret_length, tmpbuf1);
#else
        ret = pNand->ops->write_page(pNand, page, tmpbuf1, mtd->writesize, NULL, 0);
#endif
        if (ret != 0)
        {
            rt_kprintf("mtd_write writepage error.0x%x\n", to);
            errret = ret;
        }

        *retlen += leftLen;
        leftLen = len - *retlen;
    }

    if (*retlen != len)
    {
        rt_kprintf("mtd_write error:ret=%d,len=%d\n", *retlen, len);
        return errret;
    }
#ifdef UBI_TEST
    gettimeofday(&time2, NULL);
    mtd_writet += (time2.tv_sec - time1.tv_sec) * 1000000 + (time2.tv_usec - time1.tv_usec); /* us */
    mtd_writel += len;
#endif
#ifdef DATA_CHECK
    mtd_check_write(mtd, to, *retlen, buf);
    set_write_out();
#endif
    return errret;
}
int ubirt_mtd_erase(ubi_rt_mtd_info *mtd, unsigned int from, unsigned int length, unsigned char *state)
{
    struct rt_mtd_nand_device *pNand;
    if (get_rt_mtd_dev(mtd, &pNand) != 0 || pNand->ops == RT_NULL)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return -1;
    }
#ifdef DATA_CHECK
    set_erase_in();
#endif
    if (from % mtd->erasesize != 0)
    {
        rt_kprintf("erase error, from addr:0x%x,0x%x\n", from, mtd->erasesize);
        /* return -1; */
    }
    int ret = 0;
#ifdef __FULLHAN___
    struct erase_info info;
    memset(&info, 0, sizeof(info));
    info.addr = from;
    info.len = length;
    info.time = 1000;
    info.retries = 2;
    ret = pNand->ops->erase_block(pNand, &info);

    /* int i = 0;
    //wait erase complete.
    while (i<100)
    {
        rt_thread_mdelay(10);
        if (info.state != 0 )
        {
            break;
        }
        i++;
    }*/ /* have waited in driver. */
    *state = info.state;
#else
    int i = 0;
    for (i = 0; i < length / mtd->erasesize; i++)
    {
        ret = pNand->ops->erase_block(pNand, (from / mtd->erasesize + i));
    }
    *state = 0x08; /* MTD_ERASE_DONE; */
#endif
#ifdef DATA_CHECK
    mtd_check_erase(mtd, from, length);
    set_erase_out();
#endif
    return ret;
}
int ubirt_mtd_block_isbad(ubi_rt_mtd_info *mtd, unsigned int ofs)
{
    /*    rt_kprintf("call %s\n", __FUNCTION__);*/
    struct rt_mtd_nand_device *pNand;
    if (get_rt_mtd_dev(mtd, &pNand) != 0 || pNand->ops == RT_NULL)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return -1;
    }
    if (pNand->ops->check_block)
    {
#ifdef __FULLHAN___
        return pNand->ops->check_block(pNand, ofs);
#else
        return pNand->ops->check_block(pNand, ofs / mtd->erasesize);
#endif
    }
    else
    {
        return 0;
    }
}
int ubirt_mtd_block_markbad(ubi_rt_mtd_info *mtd, unsigned int ofs)
{
    rt_kprintf("!!!not!!! call %s\n", __FUNCTION__);
    struct rt_mtd_nand_device *pNand;
    if (get_rt_mtd_dev(mtd, &pNand) != 0 || pNand->ops == RT_NULL)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return -1;
    }
    if (pNand->ops->mark_badblock)
    {
#ifdef __FULLHAN___
        return pNand->ops->mark_badblock(pNand, ofs);
#else
        return pNand->ops->mark_badblock(pNand, ofs / mtd->erasesize);
#endif
    }
    return 0;
}
int ubirt_get_mtd_device(void *pNandDev, ubi_rt_mtd_info *rt_mtdinfo)
{
    /*    rt_kprintf("call %s,%p\n", __FUNCTION__, pNandDev);*/
    struct rt_mtd_nand_device *pNand = RT_MTD_NAND_DEVICE(pNandDev);
    if (pNand == RT_NULL)
    {
        rt_kprintf("error not find device\n");
        return -1;
    }
    rt_kprintf("page:%d,%d,total:%d,oob:%d\n", pNand->page_size, pNand->pages_per_block, pNand->block_total, pNand->oob_size);
    /* rt_mtdinfo->type = MTD_NANDFLASH; */
    /* rt_mtdinfo->flags = MTD_WRITEABLE; */
    rt_mtdinfo->erasesize = pNand->page_size * pNand->pages_per_block;
    rt_mtdinfo->writesize = pNand->page_size;
    rt_mtdinfo->size = pNand->block_total * rt_mtdinfo->erasesize;
    rt_mtdinfo->oobavail = pNand->oob_free;
    rt_mtdinfo->oobsize = pNand->oob_size;
    rt_mtdinfo->pDevHandle = pNandDev;
    rt_mtdinfo->index = get_rt_mtd_id(pNand);
#ifdef DATA_CHECK
    mtd_check_init(rt_mtdinfo);
#endif
    return 0;
}
int ubirt_get_mtd_device_nm(const char *name, ubi_rt_mtd_info *rt_mtdinfo)
{
    rt_kprintf("call %s,%s\n", __FUNCTION__, name);
    struct rt_mtd_nand_device *pNand;
    char mName[RT_NAME_MAX];
    rt_memset(mName, 0, sizeof(RT_NAME_MAX));
    strncpy(mName, name, RT_NAME_MAX);
    pNand = RT_MTD_NAND_DEVICE(rt_device_find(mName));
    if (pNand == RT_NULL)
    {
        rt_kprintf("error not find device %s\n", mName);
        return -1;
    }
    strncpy(rt_mtdinfo->name, mName, sizeof(rt_mtdinfo->name));
    return ubirt_get_mtd_device((void *)pNand, rt_mtdinfo);
}
/* should return the whole device's size. */
unsigned int ubirt_mtd_get_device_size(const ubi_rt_mtd_info *mtd)
{
#ifdef __FULLHAN___
#include "flash_layout.h"
    unsigned int devSize = 0;
    int i = 0;
    for (i = 0; i < MAX_FLASH_PART_NUM; i++)
    {
        if (g_flash_layout_info[i].part_type & PART_LAST)
        {
            devSize += g_flash_layout_info[i].part_size;
            break;
        }
        devSize += g_flash_layout_info[i].part_size;
    }
    return devSize;
#else
    rt_kprintf("call %s\n", __FUNCTION__);
    struct rt_mtd_nand_device *pNand;
    if (get_rt_mtd_dev((ubi_rt_mtd_info *)mtd, &pNand) != 0)
    {
        rt_kprintf("error not find device %s or opt\n", mtd->name);
        return 0;
    }
    /* return 256 * 1024 * 1024; */
    return pNand->block_total * mtd->erasesize;
#endif
}
