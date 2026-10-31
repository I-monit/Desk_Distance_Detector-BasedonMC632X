/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#ifndef __FH_SDIO_H__
#define __FH_SDIO_H__

enum
{
    ERRNOERROR = 0,

    /* for raw interrupt status error */
    ERRRESPRECEP,  /* 1 */
    ERRRESPCRC,
    ERRDCRC,
    ERRRESPTIMEOUT,
    ERRDRTIMEOUT,
    ERRUNDERWRITE,
    ERROVERREAD,
    ERRHLE,
    ERRSTARTBIT,
    ERRENDBITERR,  /* 10 */

    /* for R1 response */
    ERRADDRESSRANGE,  /* 11 */
    ERRADDRESSMISALIGN,
    ERRBLOCKLEN,
    ERRERASESEQERR,
    ERRERASEPARAM,
    ERRPROT,
    ERRCARDLOCKED,
    ERRCRC,
    ERRILLEGALCOMMAND,
    ERRECCFAILED,
    ERRCCERR,
    ERRUNKNOWN,
    ERRUNDERRUN,
    ERROVERRUN,
    ERRCSDOVERWRITE,
    ERRERASERESET,
    ERRFSMSTATE,  /* 27 */

    /* for R5 response */
    ERRBADFUNC,  /* 28 */

    /* others */
    ERRCARDNOTCONN,  /* 29 */
    ERRCARDWPROTECT,
    ERRCMDRETRIESOVER,
    ERRNOTSUPPORTED,
    ERRHARDWARE,
    ERRDATANOTREADY,
    ERRCARDINTERNAL,
    ERRACMD41TIMEOUT,
    ERRIDMA,
    ERRNORES,

    ERRNOTEQUAL,
};

typedef struct DmaDescStruct
{
    unsigned int desc0; /* control and status information of descriptor */
    unsigned int desc1; /* buffer sizes                                 */
    unsigned int desc2; /* physical address of the buffer 1             */
    unsigned int desc3; /* physical address of the buffer 2             */
} DmaDesc;

#define SDC_WKMOD_25M_STAND_SPEED 0x0002  /* 25M standard speed */
#define SDC_WKMOD_50M_HI_SPEED    0x0004  /* 50M high speed */
#define SDC_WKMOD_1WIRE           0x0008  /* 1 wire transfer mode */
#define SDC_WKMOD_4WIRE           0x0010  /* 4 wire transfer mode */

typedef void *HSDC;


/*
 * for SDIO WIFI card
 */
extern void fh_sdio0_init(void);
extern void fh_sdio1_init(void);
extern void fh_sdio_init(void);
extern int sdio_init(unsigned int which, unsigned int wkmod,
        unsigned int *dma_desc /*4Byte aligned,16 bytes space*/,
        HSDC *phandle);
extern int sdio_resume(unsigned int which, unsigned int wkmod,
        unsigned int *dma_desc /*4Byte aligned,16 bytes space*/,
        HSDC *phandle);
extern int sdio_set_cmd53_addr_mode(HSDC handle, int fixed_addr);
extern int sdio_high_speed_mode(HSDC handle, int bitwidth, int freq);
extern int sdio_enable_card_int(HSDC handle, int enable);
extern int sdio_set_card_int_cb(HSDC handle, void (*cb)(void));
extern int sdio_drv_read(HSDC handle, unsigned int addr, unsigned int fn,
        unsigned int bcnt, unsigned int bsize,
        unsigned char *buf);
extern int sdio_drv_write(HSDC handle, unsigned int addr, unsigned int fn,
        unsigned int bcnt, unsigned int bsize,
        unsigned char *buf);
extern int sdio_drv_creg_read(HSDC handle, int addr, int fn,
        unsigned int *resp);
extern int sdio_drv_creg_write(HSDC handle, int addr, int fn,
        unsigned char data, unsigned int *resp);

#ifdef DONT_COPY_NET_PAYLOAD_TO_SEND
/* map to DmaDesc */
typedef struct _buf_chain
{
    unsigned int csi;         /* desc0 */
    unsigned int size;        /* desc1 */
    void *buf;                /* desc2 */
    struct _buf_chain *next;  /* desc3 */
} buf_chain_t;

int sdio_drv_chain_write(HSDC handle, unsigned int addr, unsigned int fn,
        unsigned int bcnt, unsigned int bsize,
        buf_chain_t *chain);
#endif

int sdc_deinit(HSDC handle);

#endif
