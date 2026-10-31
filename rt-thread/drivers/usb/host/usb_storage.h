#ifndef __USB_STORAGE_H__
#define __USB_STORAGE_H__
#include <rtdef.h>


#define USB_CNTL_TIMEOUT 100 /* 100ms timeout */

/* Dynamic bitflag definitions (us->dflags): used in set_bit() etc. */
#define US_FLIDX_URB_ACTIVE    0    /* current_urb is in use    */
#define US_FLIDX_SG_ACTIVE    1    /* current_sg is in use     */
#define US_FLIDX_ABORTING    2    /* abort is in progress     */
#define US_FLIDX_DISCONNECTING    3    /* disconnect in progress   */
#define US_FLIDX_RESETTING    4    /* device reset in progress */
#define US_FLIDX_TIMED_OUT    5    /* SCSI midlayer timed out  */
#define US_FLIDX_DONT_SCAN    6    /* don't scan (disconnect)  */
#define US_FLIDX_REDO_READ10    7    /* redo READ(10) command    */
#define US_FLIDX_READ10_WORKED    8    /* previous READ(10) succeeded */

#define USB_STOR_STRING_LEN 32

/*
 * We provide a DMA-mapped I/O buffer for use with small USB transfers.
 * It turns out that CB[I] needs a 12-byte buffer and Bulk-only needs a
 * 31-byte buffer.  But Freecom needs a 64-byte buffer, so that's the
 * size we'll allocate.
 */

#define US_IOBUF_SIZE        64    /* Size of the DMA-mapped I/O buffer */
#define US_SENSE_SIZE        18    /* Size of the autosense data buffer */

/* USB-status codes: */
#define USB_ST_ACTIVE           0x1        /* TD is active */
#define USB_ST_STALLED          0x2        /* TD is stalled */
#define USB_ST_BUF_ERR          0x4        /* buffer error */
#define USB_ST_BABBLE_DET       0x8        /* Babble detected */
#define USB_ST_NAK_REC          0x10    /* NAK Received*/
#define USB_ST_CRC_ERR          0x20    /* CRC/timeout Error */
#define USB_ST_BIT_ERR          0x40    /* Bitstuff error */
#define USB_ST_NOT_PROC         0x80000000L    /* Not yet processed */



#define US_SUSPEND    0
#define US_RESUME    1


/* Storage subclass codes */

#define USB_SC_RBC    0x01        /* Typically, flash devices */
#define USB_SC_8020    0x02        /* CD-ROM */
#define USB_SC_QIC    0x03        /* QIC-157 Tapes */
#define USB_SC_UFI    0x04        /* Floppy */
#define USB_SC_8070    0x05        /* Removable media */
#define USB_SC_SCSI    0x06        /* Transparent */
#define USB_SC_LOCKABLE    0x07        /* Password-protected */

#define USB_SC_ISD200    0xf0        /* ISD200 ATA */
#define USB_SC_CYP_ATACB    0xf1    /* Cypress ATACB */
#define USB_SC_DEVICE    0xff        /* Use device's value */

/* Storage protocol codes */

#define USB_PR_CBI    0x00        /* Control/Bulk/Interrupt */
#define USB_PR_CB    0x01        /* Control/Bulk w/o interrupt */
#define USB_PR_BULK    0x50        /* bulk only */
#define USB_PR_UAS    0x62        /* USB Attached SCSI */

#define USB_PR_USBAT    0x80        /* SCM-ATAPI bridge */
#define USB_PR_EUSB_SDDR09    0x81    /* SCM-SCSI bridge for SDDR-09 */
#define USB_PR_SDDR55    0x82        /* SDDR-55 (made up) */
#define USB_PR_DPCM_USB    0xf0        /* Combination CB/SDDR09 */
#define USB_PR_FREECOM    0xf1        /* Freecom */
#define USB_PR_DATAFAB    0xf2        /* Datafab chipsets */
#define USB_PR_JUMPSHOT    0xf3        /* Lexar Jumpshot */
#define USB_PR_ALAUDA    0xf4        /* Alauda chipsets */
#define USB_PR_KARMA    0xf5        /* Rio Karma */

#define USB_PR_DEVICE    0xff        /* Use device's value */


#define USB_US_TYPE_NONE   0
#define USB_US_TYPE_STOR   1        /* usb-storage */
#define USB_US_TYPE_UB     2        /* ub */

#define USB_US_TYPE(flags)         (((flags) >> 24) & 0xFF)
#define USB_US_ORIG_FLAGS(flags)    ((flags) & 0x00FFFFFF)

struct bulk_cb_wrap
{
    rt_uint32_t    Signature;        /* contains 'USBC' */
    rt_uint32_t    Tag;            /* unique per command id */
    rt_uint32_t    DataTransferLength;    /* size of data */
    rt_uint8_t    Flags;            /* direction in bit 0 */
    rt_uint8_t    Lun;            /* LUN normally 0 */
    rt_uint8_t    Length;            /* of of the CDB */
    rt_uint8_t    CDB[16];        /* max command */
};

#define US_BULK_CB_WRAP_LEN    31
#define US_BULK_CB_SIGN        0x43425355    /*spells out USBC */
#define US_BULK_FLAG_IN        1
#define US_BULK_FLAG_OUT    0

/* command status wrapper */
struct bulk_cs_wrap
{
    rt_uint32_t    Signature;        /* should = 'USBS' */
    rt_uint32_t    Tag;            /* same as original command */
    rt_uint32_t    Residue;        /* amount not transferred */
    rt_uint8_t    Status;            /* see below */
    rt_uint8_t    Filler[18];
};

#define US_BULK_CS_WRAP_LEN    13
#define US_BULK_CS_SIGN        0x53425355    /* spells out 'USBS' */
#define US_BULK_STAT_OK        0
#define US_BULK_STAT_FAIL    1
#define US_BULK_STAT_PHASE    2

/* bulk-only class specific requests */
#define US_BULK_RESET_REQUEST    0xff
#define US_BULK_GET_MAX_LUN    0xfe

/*
 * usb_stor_bulk_transfer_xxx() return codes, in order of severity
 */

#define USB_STOR_XFER_GOOD    0    /* good transfer                 */
#define USB_STOR_XFER_SHORT    1    /* transferred less than expected */
#define USB_STOR_XFER_STALLED    2    /* endpoint stalled              */
#define USB_STOR_XFER_LONG    3    /* device tried to send too much */
#define USB_STOR_XFER_ERROR    4    /* transfer died in the middle   */

/* STORAGE Protocols */
#define US_PR_CB               1        /* Control/Bulk w/o interrupt */
#define US_PR_CBI              0        /* Control/Bulk/Interrupt */
#define US_PR_BULK             0x50        /* bulk only */

/* Sub STORAGE Classes */
#define US_SC_RBC              1        /* Typically, flash devices */
#define US_SC_8020             2        /* CD-ROM */
#define US_SC_QIC              3        /* QIC-157 Tapes */
#define US_SC_UFI              4        /* Floppy */
#define US_SC_8070             5        /* Removable media */
#define US_SC_SCSI             6        /* Transparent */
#define US_SC_MIN              US_SC_RBC
#define US_SC_MAX              US_SC_SCSI

/*
 * Transport return codes
 */

#define USB_STOR_TRANSPORT_GOOD       0   /* Transport good, command good       */
#define USB_STOR_TRANSPORT_FAILED  1   /* Transport good, command failed   */
#define USB_STOR_TRANSPORT_NO_SENSE 2  /* Command failed, no auto-sense    */
#define USB_STOR_TRANSPORT_ERROR   3   /* Transport bad (i.e. device dead) */

#define SCSI_CHANGE_DEF    0x40        /* Change Definition (Optional) */
#define SCSI_COMPARE        0x39        /* Compare (O) */
#define SCSI_COPY            0x18        /* Copy (O) */
#define SCSI_COP_VERIFY    0x3A        /* Copy and Verify (O) */
#define SCSI_INQUIRY        0x12        /* Inquiry (MANDATORY) */
#define SCSI_LOG_SELECT    0x4C        /* Log Select (O) */
#define SCSI_LOG_SENSE    0x4D        /* Log Sense (O) */
#define SCSI_MODE_SEL6    0x15        /* Mode Select 6-byte (Device Specific) */
#define SCSI_MODE_SEL10    0x55        /* Mode Select 10-byte (Device Specific) */
#define SCSI_MODE_SEN6    0x1A        /* Mode Sense 6-byte (Device Specific) */
#define SCSI_MODE_SEN10    0x5A        /* Mode Sense 10-byte (Device Specific) */
#define SCSI_READ_BUFF    0x3C        /* Read Buffer (O) */
#define SCSI_REQ_SENSE    0x03        /* Request Sense (MANDATORY) */
#define SCSI_SEND_DIAG    0x1D        /* Send Diagnostic (O) */
#define SCSI_TST_U_RDY    0x00        /* Test Unit Ready (MANDATORY) */
#define SCSI_WRITE_BUFF    0x3B        /* Write Buffer (O) */
#define SCSI_COMPARE        0x39        /* Compare (O) */
#define SCSI_FORMAT        0x04        /* Format Unit (MANDATORY) */
#define SCSI_LCK_UN_CAC    0x36        /* Lock Unlock Cache (O) */
#define SCSI_PREFETCH    0x34        /* Prefetch (O) */
#define SCSI_MED_REMOVL    0x1E        /* Prevent/Allow medium Removal (O) */
#define SCSI_READ6        0x08        /* Read 6-byte (MANDATORY) */
#define SCSI_READ10        0x28        /* Read 10-byte (MANDATORY) */
#define SCSI_RD_CAPAC    0x25        /* Read Capacity (MANDATORY) */
#define SCSI_RD_CAPAC10    SCSI_RD_CAPAC    /* Read Capacity (10) */
#define SCSI_RD_CAPAC16    0x9e        /* Read Capacity (16) */
#define SCSI_RD_DEFECT    0x37        /* Read Defect Data (O) */
#define SCSI_READ_LONG    0x3E        /* Read Long (O) */
#define SCSI_REASS_BLK    0x07        /* Reassign Blocks (O) */
#define SCSI_RCV_DIAG    0x1C        /* Receive Diagnostic Results (O) */
#define SCSI_RELEASE    0x17        /* Release Unit (MANDATORY) */
#define SCSI_REZERO        0x01        /* Rezero Unit (O) */
#define SCSI_SRCH_DAT_E    0x31        /* Search Data Equal (O) */
#define SCSI_SRCH_DAT_H    0x30        /* Search Data High (O) */
#define SCSI_SRCH_DAT_L    0x32        /* Search Data Low (O) */
#define SCSI_SEEK6        0x0B        /* Seek 6-Byte (O) */
#define SCSI_SEEK10        0x2B        /* Seek 10-Byte (O) */
#define SCSI_SEND_DIAG    0x1D        /* Send Diagnostics (MANDATORY) */
#define SCSI_SET_LIMIT    0x33        /* Set Limits (O) */
#define SCSI_START_STP    0x1B        /* Start/Stop Unit (O) */
#define SCSI_SYNC_CACHE    0x35        /* Synchronize Cache (O) */
#define SCSI_VERIFY        0x2F        /* Verify (O) */
#define SCSI_WRITE6        0x0A        /* Write 6-Byte (MANDATORY) */
#define SCSI_WRITE10    0x2A        /* Write 10-Byte (MANDATORY) */
#define SCSI_WRT_VERIFY    0x2E        /* Write and Verify (O) */
#define SCSI_WRITE_LONG    0x3F        /* Write Long (O) */
#define SCSI_WRITE_SAME    0x41        /* Write Same (O) */

typedef struct SCSI_cmd_block
{
    unsigned char        cmd[16];                    /* command                   */
    /* for request sense */
    unsigned char        sense_buf[64]
        __attribute__((aligned(64)));
    unsigned char        status;                        /* SCSI Status             */
    unsigned char        target;                        /* Target ID                 */
    unsigned char        lun;                            /* Target LUN        */
    unsigned char        cmdlen;                        /* command len                */
    unsigned long        datalen;                    /* Total data length    */
    unsigned char        *pdata;                        /* pointer to data        */
    unsigned char        msgout[12];                /* Messge out buffer (NOT USED) */
    unsigned char        msgin[12];                /* Message in buffer    */
    unsigned char        sensecmdlen;            /* Sense command len    */
    unsigned long        sensedatalen;            /* Sense data len            */
    unsigned char        sensecmd[6];            /* Sense command            */
    unsigned long        contr_stat;                /* Controller Status    */
    unsigned long        trans_bytes;            /* tranfered bytes        */

    unsigned int        priv;
}ccb;
struct usb_interface;
typedef struct block_dev_desc
{
    int        if_type;    /* type of the interface */
    int        dev;        /* device number */
    char *name;
    unsigned char    part_type;    /* partition type */
    unsigned char    target;        /* target SCSI ID */
    unsigned char    lun;        /* target LUN */
    unsigned char    type;        /* device type */
    unsigned char    removable;    /* removable device */
#ifdef CONFIG_LBA48
    unsigned char    lba48;        /* device can use 48bit addr (ATA/ATAPI v7) */
#endif
    unsigned long    lba;        /* number of blocks */
    unsigned long    blksz;        /* block size */
    int        log2blksz;    /* for convenience: log2(blksz) */
    char        vendor[40+1];    /* IDE model, SCSI Vendor */
    char        product[20+1];    /* IDE Serial no, SCSI product */
    char        revision[8+1];    /* firmware revision */
    unsigned long   (*block_read)(char *name,
            unsigned long start,
            unsigned long blkcnt,
                      void *buffer);
    unsigned long   (*block_write)(char *name,
            unsigned long start,
            unsigned long blkcnt,
                        void *buffer);
    unsigned long   (*block_erase)(char *name,
            unsigned long start,
            unsigned long blkcnt);
    void        *priv;        /* driver private struct pointer */
}block_dev_desc_t;

typedef struct dos_partition
{
    unsigned char boot_ind;        /* 0x80 - active            */
    unsigned char head;        /* starting head            */
    unsigned char sector;        /* starting sector            */
    unsigned char cyl;        /* starting cylinder            */
    unsigned char sys_ind;        /* What partition type            */
    unsigned char end_head;        /* end head                */
    unsigned char end_sector;    /* end sector                */
    unsigned char end_cyl;        /* end cylinder                */
    unsigned char start4[4];    /* starting sector counting from 0    */
    unsigned char size4[4];        /* nr of sectors in partition        */
} dos_partition_t;
extern void rt_udelay(int n);
extern int  usb_stor_init(void);


#endif

