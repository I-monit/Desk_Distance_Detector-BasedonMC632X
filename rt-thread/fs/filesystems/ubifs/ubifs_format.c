/*
 * Date                   Author                                Notes
 * 2020/07/17     wf_wangfeng@qq.com           format ubifs.
 */
#include "ubiport_common.h"
#include "port_cpu.h"
#include "linux/mtd/ubi_mtd.h"
#include "ubi_crc32.h"
#include "ubi-media.h"

enum
{
    EB_EMPTY = 0xFFFFFFFF,
    EB_CORRUPTED = 0xFFFFFFFE,
    EB_ALIEN = 0xFFFFFFFD,
    EB_BAD = 0xFFFFFFFC,
    EC_MAX = UBI_MAX_ERASECOUNTER,
};

/**
 * struct ubigen_info - libubigen information.
 * @leb_size: logical eraseblock size
 * @peb_size: size of the physical eraseblock
 * @min_io_size: minimum input/output unit size
 * @vid_hdr_offs: offset of the VID header
 * @data_offs: data offset
 * @ubi_ver: UBI version
 * @vtbl_size: volume table size
 * @max_volumes: maximum amount of volumes
 * @image_seq: UBI image sequence number
 */
struct ubigen_info
{
    int leb_size;
    int peb_size;
    int min_io_size;
    int vid_hdr_offs;
    int data_offs;
    int ubi_ver;
    int vtbl_size;
    int max_volumes;
    uint32_t image_seq;
};

/**
 * struct ubigen_vol_info - information about a volume.
 * @id: volume id
 * @type: volume type (%UBI_VID_DYNAMIC or %UBI_VID_STATIC)
 * @alignment: volume alignment
 * @data_pad: how many bytes are unused at the end of the each physical
 *            eraseblock to satisfy the requested alignment
 * @usable_leb_size: LEB size accessible for volume users
 * @name: volume name
 * @name_len: volume name length
 * @compat: compatibility of this volume (%0, %UBI_COMPAT_DELETE,
 *          %UBI_COMPAT_IGNORE, %UBI_COMPAT_PRESERVE, or %UBI_COMPAT_REJECT)
 * @used_ebs: total number of used logical eraseblocks in this volume (relevant
 *            for static volumes only)
 * @bytes: size of the volume contents in bytes (relevant for static volumes
 *         only)
 * @flags: volume flags (%UBI_VTBL_AUTORESIZE_FLG)
 */
struct ubigen_vol_info
{
    int id;
    int type;
    int alignment;
    int data_pad;
    int usable_leb_size;
    const char *name;
    int name_len;
    int compat;
    int used_ebs;
    long long bytes;
    uint8_t flags;
};

struct ubi_scan_info
{
    uint32_t *ec;
    long long mean_ec;
    int ok_cnt;
    int empty_cnt;
    int corrupted_cnt;
    int alien_cnt;
    int bad_cnt;
    int good_cnt;
    int vid_hdr_offs;
    int data_offs;
};

static int all_ff(const void *buf, int len)
{
    int i;
    const uint8_t *p = buf;

    for (i = 0; i < len; i++)
        if (p[i] != 0xFF)
            return 0;
    return 1;
}

int ubi_scan(struct mtd_info *mtd, struct ubi_scan_info **info)
{
    int eb;
    struct ubi_scan_info *si;
    unsigned long long sum = 0;
    unsigned int retlen = 0;
    int eb_cnt = mtd->size / mtd->erasesize;
    si = kmalloc(sizeof(struct ubi_scan_info), __GFP_ZERO);
    if (!si)
    {
        ubiPort_Err("cannot allocate %zd bytes of memory",
                    sizeof(struct ubi_scan_info));
        return -1;
    }
    si->ec = kmalloc(eb_cnt * sizeof(uint32_t), __GFP_ZERO);
    if (!si->ec)
    {
        ubiPort_Err("cannot allocate %zd bytes of memory",
                    sizeof(struct ubi_scan_info));
        goto out_si;
    }

    si->vid_hdr_offs = si->data_offs = -1;

    for (eb = 0; eb < eb_cnt; eb++)
    {
        int ret;
        uint32_t crc;
        struct ubi_ec_hdr ech;
        unsigned long long ec;

        ubiPort_Log1("scanning eraseblock %d", eb);

        ubiPort_Log1("\r"
                     ": scanning eraseblock %d -- %ld %% complete  ",
                     eb, (eb + 1) * 100 / eb_cnt);
        ret = mtd_block_isbad(mtd, eb * mtd->erasesize);
        if (ret == -1)
            goto out_ec;
        if (ret)
        {
            si->bad_cnt += 1;
            si->ec[eb] = EB_BAD;
            ubiPort_Log1(": bad\n");
            continue;
        }
        ret = mtd_read(mtd, eb * mtd->erasesize, sizeof(struct ubi_ec_hdr), &retlen, (u_char *)&ech);
        if (ret < 0)
            goto out_ec;

        if (be32_to_cpu(ech.magic) != UBI_EC_HDR_MAGIC)
        {
            if (all_ff(&ech, sizeof(struct ubi_ec_hdr)))
            {
                si->empty_cnt += 1;
                si->ec[eb] = EB_EMPTY;
                ubiPort_Log1(": empty\n");
            }
            else
            {
                si->alien_cnt += 1;
                si->ec[eb] = EB_ALIEN;
                ubiPort_Log1(": alien\n");
            }
            continue;
        }

        crc = crc32(UBI_CRC32_INIT, &ech, UBI_EC_HDR_SIZE_CRC);
        if (be32_to_cpu(ech.hdr_crc) != crc)
        {
            si->corrupted_cnt += 1;
            si->ec[eb] = EB_CORRUPTED;
            ubiPort_Log(": bad CRC %#08x, should be %#08x\n",
                        crc, be32_to_cpu(ech.hdr_crc));
            continue;
        }

        ec = be64_to_cpu(ech.ec);
        if (ec > EC_MAX)
        {
            ubiPort_Log("\n");
            ubiPort_Err("erase counter in EB %d is %llu, while this "
                        "program expects them to be less than %u",
                        eb, ec, EC_MAX);
            goto out_ec;
        }

        if (si->vid_hdr_offs == -1)
        {
            si->vid_hdr_offs = be32_to_cpu(ech.vid_hdr_offset);
            si->data_offs = be32_to_cpu(ech.data_offset);
            if (si->data_offs % mtd->writesize)
            {
                ubiPort_Log1("\n");
                ubiPort_Log1(": corrupted because of the below\n");
                ubiPort_Log1("bad data offset %d at eraseblock %d (n"
                             "of multiple of min. I/O unit size %d)",
                             si->data_offs, eb, mtd->writesize);
                ubiPort_Log1("treat eraseblock %d as corrupted", eb);
                si->corrupted_cnt += 1;
                si->ec[eb] = EB_CORRUPTED;
                continue;
            }
        }
        else
        {
            if ((int)be32_to_cpu(ech.vid_hdr_offset) != si->vid_hdr_offs)
            {
                ubiPort_Log("\n");
                ubiPort_Log(": corrupted because of the below\n");
                ubiPort_Log("inconsistent VID header offset: was "
                            "%d, but is %d in eraseblock %d",
                            si->vid_hdr_offs,
                            be32_to_cpu(ech.vid_hdr_offset), eb);
                ubiPort_Log("treat eraseblock %d as corrupted", eb);
                si->corrupted_cnt += 1;
                si->ec[eb] = EB_CORRUPTED;
                continue;
            }
            if ((int)be32_to_cpu(ech.data_offset) != si->data_offs)
            {
                ubiPort_Log("\n");
                ubiPort_Log(": corrupted because of the below\n");
                ubiPort_Log("inconsistent data offset: was %d, but"
                            " is %d in eraseblock %d",
                            si->data_offs,
                            be32_to_cpu(ech.data_offset), eb);
                ubiPort_Log("treat eraseblock %d as corrupted", eb);
                si->corrupted_cnt += 1;
                si->ec[eb] = EB_CORRUPTED;
                continue;
            }
        }

        si->ok_cnt += 1;
        si->ec[eb] = ec;
        ubiPort_Log1(": OK, erase counter %u\n", si->ec[eb]);
    }

    if (si->ok_cnt != 0)
    {
        /* Calculate mean erase counter */
        for (eb = 0; eb < eb_cnt; eb++)
        {
            if (si->ec[eb] > EC_MAX)
                continue;
            sum += si->ec[eb];
        }
        si->mean_ec = sum / si->ok_cnt;
    }

    si->good_cnt = eb_cnt - si->bad_cnt;
    ubiPort_Log("finished, mean EC %lld, %d OK, %d corrupted, %d empty, %d "
                "alien, bad %d",
                si->mean_ec, si->ok_cnt, si->corrupted_cnt,
                si->empty_cnt, si->alien_cnt, si->bad_cnt);

    *info = si;
    return 0;

out_ec:
    kfree(si->ec);
out_si:
    kfree(si);
    *info = NULL;
    return -1;
}

void ubi_scan_free(struct ubi_scan_info *si)
{
    kfree(si->ec);
    kfree(si);
}

int mtd_write_data(struct mtd_info *mtd, int blockNO, int offs, void *data, int len)
{
    unsigned int retLen = 0;
    int ret = 0;
    struct erase_info eInfo;
    memset(&eInfo, 0, sizeof(eInfo));
    int addr = blockNO * mtd->erasesize + offs;
    eInfo.addr = addr;
    eInfo.len = len;
    eInfo.mtd = mtd;
    mtd_erase(mtd, &eInfo);
    ret = mtd_write(mtd, addr, len, &retLen, data);
    return ret;
}
void ubigen_init_ec_hdr(const struct ubigen_info *ui,
                        struct ubi_ec_hdr *hdr, long long ec)
{
    uint32_t crc;

    memset(hdr, 0, sizeof(struct ubi_ec_hdr));

    hdr->magic = cpu_to_be32(UBI_EC_HDR_MAGIC);
    hdr->version = ui->ubi_ver;
    hdr->ec = cpu_to_be64(ec);
    hdr->vid_hdr_offset = cpu_to_be32(ui->vid_hdr_offs);
    hdr->data_offset = cpu_to_be32(ui->data_offs);
    hdr->image_seq = cpu_to_be32(ui->image_seq);

    crc = crc32(UBI_CRC32_INIT, hdr, UBI_EC_HDR_SIZE_CRC);
    hdr->hdr_crc = cpu_to_be32(crc);
}
void ubigen_init_vid_hdr(const struct ubigen_info *ui,
                         const struct ubigen_vol_info *vi,
                         struct ubi_vid_hdr *hdr, int lnum,
                         const void *data, int data_size)
{
    uint32_t crc;

    memset(hdr, 0, sizeof(struct ubi_vid_hdr));

    hdr->magic = cpu_to_be32(UBI_VID_HDR_MAGIC);
    hdr->version = ui->ubi_ver;
    hdr->vol_type = vi->type;
    hdr->vol_id = cpu_to_be32(vi->id);
    hdr->lnum = cpu_to_be32(lnum);
    hdr->data_pad = cpu_to_be32(vi->data_pad);
    hdr->compat = vi->compat;

    if (vi->type == UBI_VID_STATIC)
    {
        hdr->data_size = cpu_to_be32(data_size);
        hdr->used_ebs = cpu_to_be32(vi->used_ebs);
        crc = crc32(UBI_CRC32_INIT, data, data_size);
        hdr->data_crc = cpu_to_be32(crc);
    }

    crc = crc32(UBI_CRC32_INIT, hdr, UBI_VID_HDR_SIZE_CRC);
    hdr->hdr_crc = cpu_to_be32(crc);
}

int ubigen_write_layout_vol(struct mtd_info *mtd, const struct ubigen_info *ui, int peb1, int peb2,
                            long long ec1, long long ec2,
                            struct ubi_vtbl_record *vtbl)
{
    struct ubigen_vol_info vi;
    char *outbuf;
    struct ubi_vid_hdr *vid_hdr;
    off_t seek;

    vi.bytes = ui->leb_size * UBI_LAYOUT_VOLUME_EBS;
    vi.id = UBI_LAYOUT_VOLUME_ID;
    vi.alignment = UBI_LAYOUT_VOLUME_ALIGN;
    vi.data_pad = ui->leb_size % UBI_LAYOUT_VOLUME_ALIGN;
    vi.usable_leb_size = ui->leb_size - vi.data_pad;
    vi.data_pad = ui->leb_size - vi.usable_leb_size;
    vi.type = UBI_LAYOUT_VOLUME_TYPE;
    vi.name = UBI_LAYOUT_VOLUME_NAME;
    vi.name_len = strlen(UBI_LAYOUT_VOLUME_NAME);
    vi.compat = UBI_LAYOUT_VOLUME_COMPAT;

    outbuf = kmalloc(ui->peb_size, 0);
    if (!outbuf)
    {
        ubiPort_Err("failed to allocate %d bytes",
                    ui->peb_size);
        return -1;
    }

    memset(outbuf, 0xFF, ui->data_offs);
    vid_hdr = (struct ubi_vid_hdr *)(&outbuf[ui->vid_hdr_offs]);
    memcpy(outbuf + ui->data_offs, vtbl, ui->vtbl_size);
    memset(outbuf + ui->data_offs + ui->vtbl_size, 0xFF,
           ui->peb_size - ui->data_offs - ui->vtbl_size);

    seek = peb1 * ui->peb_size;
    /*    if (lseek(fd, seek, SEEK_SET) != seek) {
        sys_errmsg("cannot seek output file");
        goto out_free;
    }*/

    ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec1);
    ubigen_init_vid_hdr(ui, &vi, vid_hdr, 0, NULL, 0);
    unsigned int retLen = 0;
    mtd_write(mtd, seek, ui->peb_size, &retLen, (u_char *)outbuf);
    if (retLen != ui->peb_size)
    {
        ubiPort_Err("cannot write %d bytes", ui->peb_size);
        goto out_free;
    }

    seek = peb2 * ui->peb_size;
    /*if (lseek(fd, seek, SEEK_SET) != seek) {
        ubiPort_Err("cannot seek output file");
        goto out_free;
    }*/
    ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec2);
    ubigen_init_vid_hdr(ui, &vi, vid_hdr, 1, NULL, 0);
    mtd_write(mtd, seek, ui->peb_size, &retLen, (u_char *)outbuf);
    if (retLen != ui->peb_size)
    {
        ubiPort_Err("cannot write %d bytes", ui->peb_size);
        goto out_free;
    }

    kfree(outbuf);
    return 0;

out_free:
    kfree(outbuf);
    return -1;
}
struct ubi_vtbl_record *ubigen_create_empty_vtbl(const struct ubigen_info *ui)
{
    struct ubi_vtbl_record *vtbl;
    int i;

    vtbl = kmalloc(ui->vtbl_size, __GFP_ZERO);
    if (!vtbl)
    {
        ubiPort_Err("cannot allocate %d bytes of memory", ui->vtbl_size);
        return NULL;
    }

    for (i = 0; i < ui->max_volumes; i++)
    {
        uint32_t crc = crc32(UBI_CRC32_INIT, &vtbl[i],
                             UBI_VTBL_RECORD_SIZE_CRC);
        vtbl[i].crc = cpu_to_be32(crc);
    }

    return vtbl;
}
static void ubigen_info_init(struct ubigen_info *ui, int peb_size, int min_io_size,
                             int subpage_size, int vid_hdr_offs, int ubi_ver,
                             uint32_t image_seq)
{
    if (!vid_hdr_offs)
    {
        vid_hdr_offs = UBI_EC_HDR_SIZE + subpage_size - 1;
        vid_hdr_offs /= subpage_size;
        vid_hdr_offs *= subpage_size;
    }

    ui->peb_size = peb_size;
    ui->min_io_size = min_io_size;
    ui->vid_hdr_offs = vid_hdr_offs;
    ui->data_offs = vid_hdr_offs + UBI_VID_HDR_SIZE + min_io_size - 1;
    ui->data_offs /= min_io_size;
    ui->data_offs *= min_io_size;
    ui->leb_size = peb_size - ui->data_offs;
    ui->ubi_ver = ubi_ver;
    ui->image_seq = image_seq;

    ui->max_volumes = ui->leb_size / UBI_VTBL_RECORD_SIZE;
    if (ui->max_volumes > UBI_MAX_VOLUMES)
        ui->max_volumes = UBI_MAX_VOLUMES;
    ui->vtbl_size = ui->max_volumes * UBI_VTBL_RECORD_SIZE;
}

static int format(struct mtd_info *mtd,
                  const struct ubigen_info *ui, struct ubi_scan_info *si,
                  int start_eb, int novtbl)
{
    int eb, err, write_size;
    struct ubi_ec_hdr *hdr;
    struct ubi_vtbl_record *vtbl;
    int eb1 = -1, eb2 = -1;
    long long ec1 = -1, ec2 = -1;

    write_size = UBI_EC_HDR_SIZE + mtd->writesize - 1;
    write_size /= mtd->writesize;
    write_size *= mtd->writesize;
    hdr = kmalloc(write_size, 0);
    if (!hdr)
    {
        ubiPort_Err("cannot allocate %d bytes of memory", write_size);
        return -1;
    }
    memset(hdr, 0xFF, write_size);
    int eb_cnt = mtd->size / mtd->erasesize;
    for (eb = start_eb; eb < eb_cnt; eb++)
    {
        long long ec = 0;
        ubiPort_Log1("\r"
                     ": formatting eraseblock %d -- %ld %% complete  ",
                     eb, (eb + 1 - start_eb) * 100 / (eb_cnt - start_eb));

        if (si->ec[eb] == EB_BAD)
            continue;
        if (si->ec[eb] <= EC_MAX)
            ec = si->ec[eb] + 1;
        else
            ec = si->mean_ec;
        ubigen_init_ec_hdr(ui, hdr, ec);
        ubiPort_Log1("eraseblock %d: erase", eb);
        struct erase_info instr;
        instr.mtd = mtd;
        instr.addr = eb;
        instr.len = mtd->erasesize;
        err = mtd_erase(mtd, &instr);
        if (err)
        {
            ubiPort_Err("\n");
            ubiPort_Err("failed to erase eraseblock %d", eb);
            if (err != -1)
                goto out_free;
            /*  if (mark_bad(mtd, si, eb)) goto out_free; del for compile */
            continue;
        }

        if ((eb1 == -1 || eb2 == -1) && !novtbl)
        {
            if (eb1 == -1)
            {
                eb1 = eb;
                ec1 = ec;
            }
            else if (eb2 == -1)
            {
                eb2 = eb;
                ec2 = ec;
            }
            ubiPort_Log1(", do not write EC, leave for vtbl\n");
            continue;
        }
        ubiPort_Log1(", write EC %ld\n", ec);

        err = mtd_write_data(mtd, eb, 0, hdr, write_size);
        if (err)
        {
            ubiPort_Err("\n");
            ubiPort_Err("cannot write EC header (%d bytes buffer) to eraseblock %d",
                        write_size, eb);

            /* if (errno != EIO)
            {
                if (!args.subpage_size != mtd->min_io_size) ubiPort_Err("may be sub-page size is "
                                                                        "incorrect?");
                goto out_free;
            }*/
            /* write add read data,check if error. */
            /*    err = mtd_torture(libmtd, mtd, args.node_fd, eb);
                if (err) {
                    if (mark_bad(mtd, si, eb))
                        goto out_free;
                }*/
            continue;
        }
    }
    ubiPort_Log1("\n");

    if (!novtbl)
    {
        if (eb1 == -1 || eb2 == -1)
        {
            ubiPort_Err("no eraseblocks for volume table");
            goto out_free;
        }

        ubiPort_Log("write volume table to eraseblocks %d and %d", eb1, eb2);
        vtbl = ubigen_create_empty_vtbl(ui);
        if (!vtbl)
            goto out_free;

        err = ubigen_write_layout_vol(mtd, ui, eb1, eb2, ec1, ec2, vtbl);
        kfree(vtbl);
        if (err)
        {
            ubiPort_Err("cannot write layout volume");
            goto out_free;
        }
    }

    kfree(hdr);
    return 0;

out_free:
    kfree(hdr);
    return -1;
}
int ubi_format(struct mtd_info *mtd)
{
    struct ubigen_info ui;
    struct ubi_scan_info *si;
    int err = ubi_scan(mtd, &si);
    if (err)
    {
        ubiPort_Err("failed to scan mtd\n");
        return -1;
    }
    ubigen_info_init(&ui, mtd->erasesize, mtd->writesize, mtd->writesize, 0, 1, 0x00ff55dd);
    format(mtd, &ui, si, 0, 0);
    ubi_scan_free(si);
    return 0;
}
