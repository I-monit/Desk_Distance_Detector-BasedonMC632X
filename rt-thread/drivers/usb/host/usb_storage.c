#include "usb_storage.h"
#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>

#define USUAL_DEV(useProto, useTrans, useType) \
    { USB_INTERFACE_INFO(USB_CLASS_MASS_STORAGE, useProto, useTrans), \
      .driver_info = ((useType)<<24) }

struct usb_device_id usb_storage_usb_ids[] = {
    USUAL_DEV(USB_SC_RBC, USB_PR_CB, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_8020, USB_PR_CB, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_QIC, USB_PR_CB, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_UFI, USB_PR_CB, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_8070, USB_PR_CB, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_SCSI, USB_PR_CB, USB_US_TYPE_STOR),

    /* Control/Bulk/Interrupt transport for all SubClass values */
    USUAL_DEV(USB_SC_RBC, USB_PR_CBI, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_8020, USB_PR_CBI, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_QIC, USB_PR_CBI, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_UFI, USB_PR_CBI, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_8070, USB_PR_CBI, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_SCSI, USB_PR_CBI, USB_US_TYPE_STOR),

    /* Bulk-only transport for all SubClass values */
    USUAL_DEV(USB_SC_RBC, USB_PR_BULK, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_8020, USB_PR_BULK, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_QIC, USB_PR_BULK, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_UFI, USB_PR_BULK, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_8070, USB_PR_BULK, USB_US_TYPE_STOR),
    USUAL_DEV(USB_SC_SCSI, USB_PR_BULK, 0),
    { }
};
struct us_data;
typedef int (*trans_cmnd)(ccb *cb, struct us_data *data);
typedef int (*trans_reset)(struct us_data *data);
struct us_data
{
    struct usb_device    *pusb_dev;
    struct usb_interface    *pusb_intf;
    unsigned int    flags;            /* from filter initially */
#    define USB_READY    (1 << 0)
    unsigned char    ifnum;            /* interface number */
    unsigned char    ep_in;            /* in endpoint */
    unsigned char    ep_out;            /* out ....... */
    unsigned char    ep_int;            /* interrupt . */
    unsigned char    subclass;        /* as in overview */
    unsigned char    protocol;        /* .............. */
    unsigned char    attention_done;        /* force attn on first cmd */
    unsigned short    ip_data;        /* interrupt data */
    int        action;            /* what to do */
    int        ip_wanted;        /* needed */
    int (*irq_handle)(struct usb_interface *intf);        /* for USB int requests */
    unsigned int    irqpipe;         /* pipe for release_irq */
    unsigned char    irqmaxp;        /* max packed for irq Pipe */
    unsigned char    irqinterval;        /* Intervall for IRQ Pipe */
    ccb        *srb;            /* current srb */
    trans_reset    transport_reset;    /* reset routine */
    trans_cmnd    transport;        /* transport routine */
    rt_uint8_t            max_lun;
};

/* direction table -- this indicates the direction of the data
 * transfer for each command code -- a 1 indicates input
 */
static const unsigned char us_direction[256/8] = {
    0x28, 0x81, 0x14, 0x14, 0x20, 0x01, 0x90, 0x77,
    0x0C, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#define US_DIRECTION(x) ((us_direction[x>>3] >> (x & 7)) & 1)


#define USB_MAX_XFER_BLK    200


/* Command Block Wrapper */
 struct umass_bbb_cbw_t
{
    rt_uint32_t        dCBWSignature;
#    define CBWSIGNATURE    0x43425355
    rt_uint32_t        dCBWTag;
    rt_uint32_t        dCBWDataTransferLength;
    rt_uint8_t        bCBWFlags;
#    define CBWFLAGS_OUT    0x00
#    define CBWFLAGS_IN    0x80
    rt_uint8_t        bCBWLUN;
    rt_uint8_t        bCDBLength;
#    define CBWCDBLENGTH    16
    rt_uint8_t        CBWCDB[CBWCDBLENGTH];
}  __attribute__ ((packed));
#define UMASS_BBB_CBW_SIZE    31
static rt_uint32_t CBWTag;

/* Command Status Wrapper */
 struct umass_bbb_csw_t
{
    rt_uint32_t        dCSWSignature;
#    define CSWSIGNATURE    0x53425355
    rt_uint32_t        dCSWTag;
    rt_uint32_t        dCSWDataResidue;
    rt_uint8_t        bCSWStatus;
#    define CSWSTATUS_GOOD    0x0
#    define CSWSTATUS_FAILED 0x1
#    define CSWSTATUS_PHASE    0x2
} __attribute__ ((packed));
#define UMASS_BBB_CSW_SIZE    13
#define US_BBB_RESET        0xff
#define US_BBB_GET_MAX_LUN    0xfe

#define IF_TYPE_USB        4
#define PART_TYPE_UNKNOWN    0x00
#define PART_TYPE_DOS        0x02
#define DEV_TYPE_UNKNOWN    0xff    /* not connected */

static block_dev_desc_t usb_dev_desc;

static ccb usb_ccb __attribute__((aligned(64)));

unsigned long usb_stor_read(char *name, unsigned long blknr,
        unsigned long blkcnt, void *buffer);
unsigned long usb_stor_write(char *name, unsigned long blknr,
                 unsigned long blkcnt,  void *buffer);

inline static rt_uint32_t swap32(rt_uint32_t x)
{
    rt_uint32_t __x = (x);
    return ((rt_uint32_t)((((rt_uint32_t)(__x) & (rt_uint32_t)0x000000ffUL) << 24) |
                    (((rt_uint32_t)(__x) & (rt_uint32_t)0x0000ff00UL) << 8) |
                    (((rt_uint32_t)(__x) & (rt_uint32_t)0x00ff0000UL) >> 8) |
                    (((rt_uint32_t)(__x) & (rt_uint32_t)0xff000000UL) >> 24)));
}

#if 0
static int get_pipes(struct us_data *us)
{
    struct usb_host_interface *altsetting =
        us->pusb_intf->cur_altsetting;
    int i;
    struct usb_endpoint_descriptor *ep;
    struct usb_endpoint_descriptor *ep_in = NULL;
    struct usb_endpoint_descriptor *ep_out = NULL;
    struct usb_endpoint_descriptor *ep_int = NULL;

    /*
     * Find the first endpoint of each type we need.
     * We are expecting a minimum of 2 endpoints - in and out (bulk).
     * An optional interrupt-in is OK (necessary for CBI protocol).
     * We will ignore any others.
     */
    for (i = 0; i < altsetting->desc.bNumEndpoints; i++)
    {
        ep = &altsetting->endpoint[i].desc;
/* RT_DEBUG_LOG(rt_debug_usb,("endpoint desc epaddr:0x%x,eptype:0x%x,eptype:0x%x\n",ep->bEndpointAddress,ep->bDescriptorType,ep->bmAttributes)); */

        if (usb_endpoint_xfer_bulk(ep))
        {
            if (usb_endpoint_dir_in(ep))
            {
                if (!ep_in)
                    ep_in = ep;
            } else
            {
                if (!ep_out)
                    ep_out = ep;
            }
        }

        else if (usb_endpoint_is_int_in(ep))
        {
            if (!ep_int)
                ep_int = ep;
        }
    }

/* RT_DEBUG_LOG(rt_debug_usb,("ep_in:0x%x,ep_out:0x%x\n",ep_in,ep_out)); */
    if (!ep_in || !ep_out || (us->protocol == USB_PR_CBI && !ep_int))
    {
		RT_DEBUG_LOG(rt_debug_usb, ("Endpoint sanity check failed! Rejecting dev.\n"));
        return -EIO;
    }

    /* Calculate and store the pipe values */
    us->send_ctrl_pipe = usb_sndctrlpipe(us->pusb_dev, 0);
    us->recv_ctrl_pipe = usb_rcvctrlpipe(us->pusb_dev, 0);
    us->send_bulk_pipe = usb_sndbulkpipe(us->pusb_dev,
        usb_endpoint_num(ep_out));
    us->recv_bulk_pipe = usb_rcvbulkpipe(us->pusb_dev,
        usb_endpoint_num(ep_in));
    if (ep_int)
    {
        us->recv_intr_pipe = usb_rcvintpipe(us->pusb_dev,
            usb_endpoint_num(ep_int));
        us->ep_bInterval = ep_int->bInterval;
    }
    return 0;
}
#endif

#if 0
int usb_stor_Bulk_max_lun(struct us_data *us)
{
    int result;

    /* issue the command */
    us->iobuf[0] = 0;
    result = usb_stor_control_msg(us, us->recv_ctrl_pipe,
                 US_BULK_GET_MAX_LUN,
                 USB_DIR_IN | USB_TYPE_CLASS |
                 USB_RECIP_INTERFACE,
                 0, us->ifnum, us->iobuf, 1, 10*HZ);

    US_DEBUGP("GetMaxLUN command result is %d, data is %d\n",
          result, us->iobuf[0]);

    /* if we have a successful request, return the result */
    if (result > 0)
        return us->iobuf[0];

    /*
     * Some devices don't like GetMaxLUN.  They may STALL the control
     * pipe, they may return a zero-length result, they may do nothing at
     * all and timeout, or they may fail in even more bizarrely creative
     * ways.  In these cases the best approach is to use the default
     * value: only one LUN.
     */
    return 0;
}
#endif

#if 0
static void usb_stor_blocking_completion(struct urb *urb)
{
    struct rt_completion *urb_done_ptr = urb->context;

    rt_completion_done(urb_done_ptr);
}

static int usb_stor_msg_common(struct us_data *us, int timeout)
{
    struct rt_completion urb_done;
    int status;

    /* set up data structures for the wakeup system */
    rt_completion_init(&urb_done);

    /* fill the common fields in the URB */
    us->current_urb->context = &urb_done;
    us->current_urb->transfer_flags = 0;

    /* we assume that if transfer_buffer isn't us->iobuf then it
     * hasn't been mapped for DMA.  Yes, this is clunky, but it's
     * easier than always having the caller tell us whether the
     * transfer buffer has already been mapped. */
    if (us->current_urb->transfer_buffer == us->iobuf)
        us->current_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    /* us->current_urb->transfer_dma = us->iobuf_dma; */

    /* submit the URB */
    status = usb_submit_urb(us->current_urb, 0);
    if (status)
    {
        /* something went wrong */
        return status;
    }



    clear_bit(US_FLIDX_URB_ACTIVE, &us->dflags);

    if (rt_completion_wait(&urb_done, timeout ? : 0x100))
    {
		RT_DEBUG_LOG(rt_debug_usb, ("%s -- cancelling URB\n"));
        usb_kill_urb(us->current_urb);
    }

    /* return the URB status */
    return us->current_urb->status;
}

static int interpret_urb_result(struct us_data *us, unsigned int pipe,
        unsigned int length, int result, unsigned int partial)
{
	RT_DEBUG_LOG(rt_debug_usb, ("Status code %d; transferred %u/%u\n",
            result, partial, length));
    switch (result)
    {

    /* no error code; did we send all the data? */
    case 0:
        if (partial != length)
        {
			RT_DEBUG_LOG(rt_debug_usb, ("-- short transfer\n"));
            return USB_STOR_XFER_SHORT;
        }

		RT_DEBUG_LOG(rt_debug_usb, ("-- transfer complete\n"));
        return USB_STOR_XFER_GOOD;

    /* stalled */
    case -EPIPE:
        /* for control endpoints, (used by CB[I]) a stall indicates
         * a failed command */
        if (usb_pipecontrol(pipe))
        {
			RT_DEBUG_LOG(rt_debug_usb, ("-- stall on control pipe\n"));
            return USB_STOR_XFER_STALLED;
        }

        /* for other sorts of endpoint, clear the stall */
		RT_DEBUG_LOG(rt_debug_usb, ("clearing endpoint halt for pipe 0x%x\n", pipe));
        return USB_STOR_XFER_STALLED;

    /* babble - the device tried to send more than we wanted to read */
    case -EOVERFLOW:
		RT_DEBUG_LOG(rt_debug_usb, ("-- babble\n"));
        return USB_STOR_XFER_LONG;

    /* the transfer was cancelled by abort, disconnect, or timeout */
    case -ECONNRESET:
		RT_DEBUG_LOG(rt_debug_usb, ("-- transfer cancelled\n"));
        return USB_STOR_XFER_ERROR;

    /* short scatter-gather read transfer */
    case -EREMOTEIO:
		RT_DEBUG_LOG(rt_debug_usb, ("-- short read transfer\n"));
        return USB_STOR_XFER_SHORT;

    /* abort or disconnect in progress */
    case -EIO:
		RT_DEBUG_LOG(rt_debug_usb, ("-- abort or disconnect in progress\n"));
        return USB_STOR_XFER_ERROR;

    /* the catch-all error case */
    default:
		RT_DEBUG_LOG(rt_debug_usb, ("-- unknown error\n"));
        return USB_STOR_XFER_ERROR;
    }
}


int usb_stor_bulk_transfer_buf(struct us_data *us, unsigned int pipe,
    void *buf, unsigned int length, unsigned int *act_len)
{
    int result;

	RT_DEBUG_LOG(rt_debug_usb, ("%s: xfer %u bytes\n", __func__, length));

    /* fill and submit the URB */
    usb_fill_bulk_urb(us->current_urb, us->pusb_dev, pipe, buf, length,
              usb_stor_blocking_completion, NULL);
    result = usb_stor_msg_common(us, 0);

    /* store the actual length of the data transferred */
    if (act_len)
        *act_len = us->current_urb->actual_length;
    return interpret_urb_result(us, pipe, length, result,
            us->current_urb->actual_length);
}

 void usb_stor_control_thread(void *__us)
{
    struct us_data *us = (struct us_data *)__us;
    unsigned int cswlen;
    unsigned int cbwlen = US_BULK_CB_WRAP_LEN;
    unsigned int result;

    for (;;)
    {

        struct bulk_cb_wrap *bcb = (struct bulk_cb_wrap *) us->iobuf;
        struct bulk_cs_wrap *bcs = (struct bulk_cs_wrap *) us->iobuf;

		RT_DEBUG_LOG(rt_debug_usb, ("Busb_stor_control_thread start!!\n"));
        bcb->Signature = US_BULK_CB_SIGN;
		bcb->DataTransferLength = 0;
		bcb->Flags = 1 << 7;
		bcb->Tag = 1;
        bcb->Lun = 0;
        bcb->Length = 12;
        rt_memset(bcb->CDB, 0, sizeof(bcb->CDB));
		bcb->CDB[0] = 0x12 ; /*inquiry*/
		bcb->CDB[4] = 0x24 ; /*Allocation Length*/

        /*bcb*/
        result = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe,
                        bcb, cbwlen, NULL);
		RT_DEBUG_LOG(rt_debug_usb, ("Bulk command transfer result=%d\n", result));
            if (result != USB_STOR_XFER_GOOD)
                break;

        /*bcs*/
        result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
                        bcs, US_BULK_CS_WRAP_LEN, &cswlen);

		RT_DEBUG_LOG(rt_debug_usb, ("Bulk status result = %d cswlen= %d\n", result, cswlen));
        if (result != USB_STOR_XFER_GOOD)
            break;

        rt_thread_delay(100);


    }

}
#endif

#if 0
 static int us_one_transfer(struct us_data *us, int pipe, char *buf, int length)
{
     int max_size;
     int this_xfer;
     int result;
     int partial;
     int maxtry;
     int stat;

     /* determine the maximum packet size for these transfers */
     max_size = usb_maxpacket(us->pusb_dev, pipe) * 16;

     /* while we have data left to transfer */
     while (length)
    {

         /* calculate how long this will be -- maximum or a remainder */
         this_xfer = length > max_size ? max_size : length;
         length -= this_xfer;

         /* setup the retry counter */
         maxtry = 10;

         /* set up the transfer loop */
         do
        {
             /* transfer the data */
             rt_kprintf("Bulk xfer 0x%x(%d) try #%d\n",
                   (unsigned int)buf, this_xfer, 11 - maxtry);
             result = usb_bulk_msg(us->pusb_dev, pipe, buf,
                           this_xfer, &partial,
                           USB_CNTL_TIMEOUT * 5);
             rt_kprintf("bulk_msg returned %d xferred %d/%d\n",
                   result, partial, this_xfer);
             if (us->pusb_dev->status != 0)
            {
                 /* if we stall, we need to clear it before
                  * we go on
                  */
 /* #ifdef DEBUG */
/* display_int_status(us->pusb_dev->status); */
/* #endif */
                 if (us->pusb_dev->status & USB_ST_STALLED)
                {
                     rt_kprintf("stalled ->clearing endpoint" \
                           "halt for pipe 0x%x\n", pipe);
                     stat = us->pusb_dev->status;
                     usb_clear_halt(us->pusb_dev, pipe);
                     us->pusb_dev->status = stat;
                     if (this_xfer == partial)
                    {
                         rt_kprintf("bulk transferred" \
                               "with error %lX," \
                               " but data ok\n",
                               us->pusb_dev->status);
                         return 0;
                     }
                     else
                         return result;
                 }
                 if (us->pusb_dev->status & USB_ST_NAK_REC)
                {
                     rt_kprintf("Device NAKed bulk_msg\n");
                     return result;
                 }
                 rt_kprintf("bulk transferred with error");
                 if (this_xfer == partial)
                {
                     debug(" %ld, but data ok\n",
                           us->pusb_dev->status);
                     return 0;
                 }
                 /* if our try counter reaches 0, bail out */
                 rt_kprintf(" %ld, data %d\n",
                           us->pusb_dev->status, partial);
                 if (!maxtry--)
                         return result;
             }
             /* update to show what data was transferred */
             this_xfer -= partial;
             buf += partial;
             /* continue until this transfer is done */
         } while (this_xfer);
     }

     /* if we get here, we're done and successful */
     return 0;
 }


 static int usb_stor_CB_comdat(ccb *srb, struct us_data *us)
{
     int result = 0;
     int dir_in, retry;
     unsigned int pipe;
     unsigned long status;

     retry = 5;
     dir_in = US_DIRECTION(srb->cmd[0]);

     if (dir_in)
         pipe = usb_rcvbulkpipe(us->pusb_dev, us->ep_in);
     else
         pipe = usb_sndbulkpipe(us->pusb_dev, us->ep_out);

     while (retry--)
    {
         rt_kprintf("CBI gets a command: Try %d\n", 5 - retry);
/* #ifdef DEBUG */
 /* usb_show_srb(srb); */
/* #endif */
         /* let's send the command via the control pipe */
         result = usb_control_msg(us->pusb_dev,
 					 usb_sndctrlpipe(us->pusb_dev, 0),
                      US_CBI_ADSC,
                      USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                      0, us->ifnum,
                      srb->cmd, srb->cmdlen,
                      USB_CNTL_TIMEOUT * 5);
         rt_kprintf("CB_transport: control msg returned %d, status %lX\n",
               result, us->pusb_dev->status);
         /* check the return code for the command */
         if (result < 0)
        {
             if (us->pusb_dev->status & USB_ST_STALLED)
            {
                 status = us->pusb_dev->status;
                 rt_kprintf(" stall during command found," \
                       " clear pipe\n");
                 usb_clear_halt(us->pusb_dev,
                           usb_sndctrlpipe(us->pusb_dev, 0));
                 us->pusb_dev->status = status;
             }
             rt_kprintf(" error during command %02X" \
                   " Stat = %lX\n", srb->cmd[0],
                   us->pusb_dev->status);
             return result;
         }
         /* transfer the data payload for this command, if one exists*/

         rt_kprintf("CB_transport: control msg returned %d," \
               " direction is %s to go 0x%lx\n", result,
               dir_in ? "IN" : "OUT", srb->datalen);
         if (srb->datalen)
        {
             result = us_one_transfer(us, pipe, (char *)srb->pdata,
                          srb->datalen);
             rt_kprintf("CBI attempted to transfer data," \
                   " result is %d status %lX, len %d\n",
                   result, us->pusb_dev->status,
                 us->pusb_dev->act_len);
             if (!(us->pusb_dev->status & USB_ST_NAK_REC))
                 break;
         } /* if (srb->datalen) */
         else
             break;
     }
     /* return result */

     return result;
 }

 static int usb_stor_CBI_get_status(ccb *srb, struct us_data *us)
{
     int timeout;

     us->ip_wanted = 1;
     submit_int_msg(us->pusb_dev, us->irqpipe,
             (void *) &us->ip_data, us->irqmaxp, us->irqinterval);
     timeout = 1000;
     while (timeout--)
    {
         if ((volatile int *) us->ip_wanted == NULL)
             break;
         mdelay(10);
     }
     if (us->ip_wanted)
    {
         rt_kprintf("    Did not get interrupt on CBI\n");
         us->ip_wanted = 0;
         return USB_STOR_TRANSPORT_ERROR;
     }
     rt_kprintf("Got interrupt data 0x%x, transfered %d status 0x%lX\n",
           us->ip_data, us->pusb_dev->irq_act_len,
           us->pusb_dev->irq_status);
     /* UFI gives us ASC and ASCQ, like a request sense */
     if (us->subclass == US_SC_UFI)
    {
         if (srb->cmd[0] == SCSI_REQ_SENSE ||
             srb->cmd[0] == SCSI_INQUIRY)
             return USB_STOR_TRANSPORT_GOOD; /* Good */
         else if (us->ip_data)
             return USB_STOR_TRANSPORT_FAILED;
         else
             return USB_STOR_TRANSPORT_GOOD;
     }
     /* otherwise, we interpret the data normally */
     switch (us->ip_data)
    {
     case 0x0001:
         return USB_STOR_TRANSPORT_GOOD;
     case 0x0002:
         return USB_STOR_TRANSPORT_FAILED;
     default:
         return USB_STOR_TRANSPORT_ERROR;
     }            /* switch */
     return USB_STOR_TRANSPORT_ERROR;
 }

 static int usb_stor_CB_transport(ccb *srb, struct us_data *us)
{
     int result, status;
     ccb *psrb;
     ccb reqsrb;
     int retry, notready;

     psrb = &reqsrb;
     status = USB_STOR_TRANSPORT_GOOD;
     retry = 0;
     notready = 0;
     /* issue the command */
 do_retry:
     result = usb_stor_CB_comdat(srb, us);
     rt_kprintf("command / Data returned %d, status %lX\n",
           result, us->pusb_dev->status);
     /* if this is an CBI Protocol, get IRQ */
     if (us->protocol == US_PR_CBI)
    {
         status = usb_stor_CBI_get_status(srb, us);
         /* if the status is error, report it */
         if (status == USB_STOR_TRANSPORT_ERROR)
        {
             rt_kprintf(" USB CBI Command Error\n");
             return status;
         }
         srb->sense_buf[12] = (unsigned char)(us->ip_data >> 8);
         srb->sense_buf[13] = (unsigned char)(us->ip_data & 0xff);
         if (!us->ip_data)
        {
             /* if the status is good, report it */
             if (status == USB_STOR_TRANSPORT_GOOD)
            {
                 rt_kprintf(" USB CBI Command Good\n");
                 return status;
             }
         }
     }
     /* do we have to issue an auto request? */
     /* HERE we have to check the result */
     if ((result < 0) && !(us->pusb_dev->status & USB_ST_STALLED))
    {
         rt_kprintf("ERROR %lX\n", us->pusb_dev->status);
         us->transport_reset(us);
         return USB_STOR_TRANSPORT_ERROR;
     }
     if ((us->protocol == US_PR_CBI) &&
         ((srb->cmd[0] == SCSI_REQ_SENSE) ||
         (srb->cmd[0] == SCSI_INQUIRY))) {
         /* do not issue an autorequest after request sense */
         rt_kprintf("No auto request and good\n");
         return USB_STOR_TRANSPORT_GOOD;
     }
     /* issue an request_sense */
     memset(&psrb->cmd[0], 0, 12);
     psrb->cmd[0] = SCSI_REQ_SENSE;
     psrb->cmd[1] = srb->lun << 5;
     psrb->cmd[4] = 18;
     psrb->datalen = 18;
     psrb->pdata = &srb->sense_buf[0];
     psrb->cmdlen = 12;
     /* issue the command */
     result = usb_stor_CB_comdat(psrb, us);
     debug("auto request returned %d\n", result);
     /* if this is an CBI Protocol, get IRQ */
     if (us->protocol == US_PR_CBI)
         status = usb_stor_CBI_get_status(psrb, us);

     if ((result < 0) && !(us->pusb_dev->status & USB_ST_STALLED))
    {
         rt_kprintf(" AUTO REQUEST ERROR %ld\n",
               us->pusb_dev->status);
         return USB_STOR_TRANSPORT_ERROR;
     }
     rt_kprintf("autorequest returned 0x%02X 0x%02X 0x%02X 0x%02X\n",
           srb->sense_buf[0], srb->sense_buf[2],
           srb->sense_buf[12], srb->sense_buf[13]);
     /* Check the auto request result */
     if ((srb->sense_buf[2] == 0) &&
         (srb->sense_buf[12] == 0) &&
         (srb->sense_buf[13] == 0)) {
         /* ok, no sense */
         return USB_STOR_TRANSPORT_GOOD;
     }

     /* Check the auto request result */
     switch (srb->sense_buf[2])
    {
     case 0x01:
         /* Recovered Error */
         return USB_STOR_TRANSPORT_GOOD;
         break;
     case 0x02:
         /* Not Ready */
         if (notready++ > USB_TRANSPORT_NOT_READY_RETRY)
        {
 			printf("cmd 0x%02X returned 0x%02X 0x%02X 0x%02X 0x%02X (NOT READY)\n",
 			       srb->cmd[0],
                 srb->sense_buf[0], srb->sense_buf[2],
                 srb->sense_buf[12], srb->sense_buf[13]);
             return USB_STOR_TRANSPORT_FAILED;
         } else
        {
             rt_udelay(100);
             goto do_retry;
         }
         break;
     default:
         if (retry++ > USB_TRANSPORT_UNKNOWN_RETRY)
        {
 			rt_kprintf("cmd 0x%02X returned 0x%02X 0x%02X 0x%02X 0x%02X\n",
 			       srb->cmd[0], srb->sense_buf[0],
                 srb->sense_buf[2], srb->sense_buf[12],
                 srb->sense_buf[13]);
             return USB_STOR_TRANSPORT_FAILED;
         }
                else
             goto do_retry;
         break;
     }
     return USB_STOR_TRANSPORT_FAILED;
 }

 /* FIXME: this reset function doesn't really reset the port, and it
  * should. Actually it should probably do what it's doing here, and
  * reset the port physically
  */
 static int usb_stor_CB_reset(struct us_data *us)
{
     unsigned char cmd[12];
     int result;

     rt_kprintf("CB_reset\n");
     memset(cmd, 0xff, sizeof(cmd));
     cmd[0] = SCSI_SEND_DIAG;
     cmd[1] = 4;
     result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0),
                  US_CBI_ADSC,
                  USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                  0, us->ifnum, cmd, sizeof(cmd),
                  USB_CNTL_TIMEOUT * 5);

     /* long wait for reset */
     rt_udelay(1500);
     rt_kprintf("CB_reset result %d: status %lX clearing endpoint halt\n",
           result, us->pusb_dev->status);
     usb_clear_halt(us->pusb_dev, usb_rcvbulkpipe(us->pusb_dev, us->ep_in));
     usb_clear_halt(us->pusb_dev, usb_rcvbulkpipe(us->pusb_dev, us->ep_out));

     rt_kprintf("CB_reset done\n");
     return 0;
 }
#endif

 static int usb_stor_BBB_comdat(ccb *srb, struct us_data *us)
{
     int result;
     int actlen;
     int dir_in;
     unsigned int pipe;
     /* ALLOC_CACHE_ALIGN_BUFFER(umass_bbb_cbw_t, cbw, 1); */
 	struct umass_bbb_cbw_t *cbw = RT_NULL;
     /* cbw=rt_malloc(sizeof(struct umass_bbb_cbw_t)); */
 	cbw = usb_dma_buffer_alloc(sizeof(struct umass_bbb_csw_t), us->pusb_dev->bus);
 	rt_memset(cbw, 0, sizeof(struct umass_bbb_cbw_t));
     dir_in = US_DIRECTION(srb->cmd[0]);

 #ifdef BBB_COMDAT_TRACE
     rt_kprintf("dir %d lun %d cmdlen %d cmd %p datalen %lu pdata %p\n",
         dir_in, srb->lun, srb->cmdlen, srb->cmd, srb->datalen,
         srb->pdata);
     if (srb->cmdlen)
    {
         for (result = 0; result < srb->cmdlen; result++)
             rt_kprintf("cmd[%d] %#x ", result, srb->cmd[result]);
         rt_kprintf("\n");
     }
 #endif
     /* sanity checks */
     if (!(srb->cmdlen <= CBWCDBLENGTH))
    {
         rt_kprintf("usb_stor_BBB_comdat:cmdlen too large\n");
         return -1;
     }

     /* always OUT to the ep */
 	rt_kprintf("endpoint number:%x\n", us->ep_out);
     pipe = usb_sndbulkpipe(us->pusb_dev, us->ep_out);

     cbw->dCBWSignature = (CBWSIGNATURE);
     cbw->dCBWTag = (CBWTag++);
     cbw->dCBWDataTransferLength = (srb->datalen);
     cbw->bCBWFlags = (dir_in ? CBWFLAGS_IN : CBWFLAGS_OUT);
     cbw->bCBWLUN = srb->lun;
     cbw->bCDBLength = srb->cmdlen;
     /* copy the command data into the CBW command data buffer */
     /* DST SRC LEN!!! */
     rt_memcpy(cbw->CBWCDB, srb->cmd, srb->cmdlen);
     result = usb_bulk_msg(us->pusb_dev, pipe, cbw, UMASS_BBB_CBW_SIZE,
                   &actlen, USB_CNTL_TIMEOUT * 5);
     if (result < 0)
 		rt_kprintf("usb_stor_BBB_comdat:usb_bulk_msg error:%d\n", result);
 	usb_dma_buffer_free(cbw, us->pusb_dev->bus);
     return result;
 }

 static int usb_stor_BBB_reset(struct us_data *us)
{
     int result;
     unsigned int pipe;

     /*
      * Reset recovery (5.3.4 in Universal Serial Bus Mass Storage Class)
      *
      * For Reset Recovery the host shall issue in the following order:
      * a) a Bulk-Only Mass Storage Reset
      * b) a Clear Feature HALT to the Bulk-In endpoint
      * c) a Clear Feature HALT to the Bulk-Out endpoint
      *
      * This is done in 3 steps.
      *
      * If the reset doesn't succeed, the device should be port reset.
      *
      * This comment stolen from FreeBSD's /sys/dev/usb/umass.c.
      */
     rt_kprintf("BBB_reset\n");
     result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0),
                  US_BBB_RESET,
                  USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                  0, us->ifnum, NULL, 0, USB_CNTL_TIMEOUT * 5);

     if ((result < 0))
    {
         rt_kprintf("RESET:stall\n");
         return -1;
     }

     /* long wait for reset */
     rt_udelay(150);
     rt_kprintf("BBB_reset result %d: status %lX reset\n",
           result, us->pusb_dev->state);
     pipe = usb_rcvbulkpipe(us->pusb_dev, us->ep_in);
     result = usb_clear_halt(us->pusb_dev, pipe);
     /* long wait for reset */
     rt_udelay(150);
     rt_kprintf("BBB_reset result %d: status %lX clearing IN endpoint\n",
           result, us->pusb_dev->state);
     /* long wait for reset */
     pipe = usb_sndbulkpipe(us->pusb_dev, us->ep_out);
     result = usb_clear_halt(us->pusb_dev, pipe);
     rt_udelay(150);
     rt_kprintf("BBB_reset result %d: status %lX clearing OUT endpoint\n",
           result, us->pusb_dev->state);
     rt_kprintf("BBB_reset done\n");
     return 0;
 }

 static int usb_stor_BBB_clear_endpt_stall(struct us_data *us, rt_uint32_t endpt)
{
     int result;

     /* ENDPOINT_HALT = 0, so set value to 0 */
     result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0),
                 USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
                 0, endpt, NULL, 0, USB_CNTL_TIMEOUT * 5);
     return result;
 }

 static int usb_stor_BBB_transport(ccb *srb, struct us_data *us)
{
     int result, retry;
     int dir_in;
     int actlen, data_actlen;
     unsigned int pipe, pipein, pipeout;
 /* ALLOC_CACHE_ALIGN_BUFFER(umass_bbb_csw_t, csw; */
     /* struct umass_bbb_csw_t *csw=rt_malloc(sizeof(struct umass_bbb_csw_t)); */
 #ifdef BBB_XPORT_TRACE
     unsigned char *ptr;
     int index;
 #endif
 	struct umass_bbb_csw_t *csw = usb_dma_buffer_alloc(sizeof(struct umass_bbb_csw_t), us->pusb_dev->bus);

 	rt_memset(csw, 0, sizeof(struct umass_bbb_csw_t));
     dir_in = US_DIRECTION(srb->cmd[0]);

     /* COMMAND phase */
     rt_kprintf("COMMAND phase\n");
     result = usb_stor_BBB_comdat(srb, us);
     if (result < 0)
    {
         rt_kprintf("failed to send CBW status %x\n",
                 result);
         usb_stor_BBB_reset(us);
         return USB_STOR_TRANSPORT_FAILED;
     }
     if (!(us->flags & USB_READY))
         rt_udelay(5);
     pipein = usb_rcvbulkpipe(us->pusb_dev, us->ep_in);
     pipeout = usb_sndbulkpipe(us->pusb_dev, us->ep_out);
     /* DATA phase + error handling */
     data_actlen = 0;
     /* no data, go immediately to the STATUS phase */
     if (srb->datalen == 0)
         goto st;
     rt_kprintf("DATA phase\n");
     if (dir_in)
         pipe = pipein;
     else
         pipe = pipeout;

     result = usb_bulk_msg(us->pusb_dev, pipe, srb->pdata, srb->datalen,
                   &data_actlen, USB_CNTL_TIMEOUT * 5);
     /* special handling of STALL in DATA phase */
     if ((result < 0))
    {
         rt_kprintf("DATA:stall\n");
         /* clear the STALL on the endpoint */
         result = usb_stor_BBB_clear_endpt_stall(us,
                     dir_in ? us->ep_in : us->ep_out);
         if (result >= 0)
             /* continue on to STATUS phase */
             goto st;
     }
     if (result < 0)
    {
         rt_kprintf("usb_bulk_msg error status %x\n",
                 result);
         usb_stor_BBB_reset(us);
         return USB_STOR_TRANSPORT_FAILED;
     }
 #ifdef BBB_XPORT_TRACE
     for (index = 0; index < data_actlen; index++)
         rt_kprintf("pdata[%d] %#x ", index, srb->pdata[index]);
     rt_kprintf("\n");
 #endif
     /* STATUS phase + error handling */
 st:
     retry = 0;
 again:
 rt_kprintf("STATUS phase\n");
     result = usb_bulk_msg(us->pusb_dev, pipein, csw, UMASS_BBB_CSW_SIZE,
                 &actlen, USB_CNTL_TIMEOUT*5);

     /* special handling of STALL in STATUS phase */
     if ((result < 0) && (retry < 1))
    {
         rt_kprintf("STATUS:stall\n");
         /* clear the STALL on the endpoint */
         result = usb_stor_BBB_clear_endpt_stall(us, us->ep_in);
         if (result >= 0 && (retry++ < 1))
             /* do a retry */
             goto again;
     }
     if (result < 0)
    {
         rt_kprintf("usb_bulk_msg error status %ld\n",
               us->pusb_dev->state);
         usb_stor_BBB_reset(us);
         return USB_STOR_TRANSPORT_FAILED;
     }
 #ifdef BBB_XPORT_TRACE
     ptr = (unsigned char *)csw;
     for (index = 0; index < UMASS_BBB_CSW_SIZE; index++)
         rt_kprintf("ptr[%d] %#x ", index, ptr[index]);
     rt_kprintf("\n");
 #endif
     /* misuse pipe to get the residue */
     pipe = csw->dCSWDataResidue;
     if (pipe == 0 && srb->datalen != 0 && srb->datalen - data_actlen != 0)
         pipe = srb->datalen - data_actlen;
     if (CSWSIGNATURE != (csw->dCSWSignature))
    {
         rt_kprintf("!CSWSIGNATURE\n");
         usb_stor_BBB_reset(us);
 		usb_dma_buffer_free(csw, us->pusb_dev->bus);
         return USB_STOR_TRANSPORT_FAILED;
     } else if ((CBWTag - 1) != (csw->dCSWTag))
    {
         rt_kprintf("!Tag\n");
         usb_stor_BBB_reset(us);
 		usb_dma_buffer_free(csw, us->pusb_dev->bus);
         return USB_STOR_TRANSPORT_FAILED;
     } else if (csw->bCSWStatus > CSWSTATUS_PHASE)
    {
         rt_kprintf(">PHASE\n");
         usb_stor_BBB_reset(us);
 		usb_dma_buffer_free(csw, us->pusb_dev->bus);
         return USB_STOR_TRANSPORT_FAILED;
     } else if (csw->bCSWStatus == CSWSTATUS_PHASE)
    {
         rt_kprintf("=PHASE\n");
         usb_stor_BBB_reset(us);
 		usb_dma_buffer_free(csw, us->pusb_dev->bus);
         return USB_STOR_TRANSPORT_FAILED;
     } else if (data_actlen > srb->datalen)
    {
         rt_kprintf("transferred %dB instead of %ldB\n",
               data_actlen, srb->datalen);
 		usb_dma_buffer_free(csw, us->pusb_dev->bus);
         return USB_STOR_TRANSPORT_FAILED;
     } else if (csw->bCSWStatus == CSWSTATUS_FAILED)
    {
         rt_kprintf("FAILED\n");
 		usb_dma_buffer_free(csw, us->pusb_dev->bus);
         return USB_STOR_TRANSPORT_FAILED;
     }
 	usb_dma_buffer_free(csw, us->pusb_dev->bus);
     return result;
 }

 static int usb_stor_irq(struct usb_interface *intf)
{
     struct us_data *us;

     us = (struct us_data *)intf->user_data;

     if (us->ip_wanted)
         us->ip_wanted = 0;
     return 0;
 }
 /*
  * BULK only
  */
 static unsigned int usb_get_max_lun(struct us_data *us)
{
     int len;
     unsigned char result[1];
 	unsigned char *dma_addr = (unsigned char *)usb_dma_buffer_alloc(1, us->pusb_dev->bus);
     /* rt_memset(dma_addr, 0x55, 1); */
     len = usb_control_msg(us->pusb_dev,
                   usb_rcvctrlpipe(us->pusb_dev, 0),
                   US_BBB_GET_MAX_LUN,
                   USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
                   0, us->ifnum,
                  dma_addr, sizeof(char),
                   (USB_CNTL_TIMEOUT * 5));
 	result[0] =  *dma_addr;
     rt_kprintf("Get Max LUN -> len = %x, result = %x\n", len, (int) *result);
 	usb_dma_buffer_free(dma_addr, us->pusb_dev->bus);
     return (len > 0) ? *result : 0;
 }

 static int usb_inquiry(ccb *srb, struct us_data *ss)
{
     int retry, i;

     retry = 5;
     do
    {
         rt_memset(&srb->cmd[0], 0, 12);
         srb->cmd[0] = SCSI_INQUIRY;
         srb->cmd[1] = srb->lun << 5;
         srb->cmd[4] = 36;
         srb->datalen = 36;
         srb->cmdlen = 12;
         i = ss->transport(srb, ss);
         rt_kprintf("inquiry returns %d\n", i);
         if (i == 0)
             break;
     } while (--retry);

     if (!retry)
    {
         rt_kprintf("error in inquiry\n");
         return -1;
     }
     return 0;
 }

 static int usb_request_sense(ccb *srb, struct us_data *ss)
{
     char *ptr;

     ptr = (char *)srb->pdata;
     rt_memset(&srb->cmd[0], 0, 12);
     srb->cmd[0] = SCSI_REQ_SENSE;
     srb->cmd[1] = srb->lun << 5;
     srb->cmd[4] = 18;
     srb->datalen = 18;
     /* srb->pdata = &srb->sense_buf[0]; */
 	srb->pdata = usb_dma_buffer_alloc(64, ss->pusb_dev->bus);
     srb->cmdlen = 12;
     ss->transport(srb, ss);
 	rt_memcpy(&srb->sense_buf[0], srb->pdata, 64);
     rt_kprintf("Request Sense returned %02X %02X %02X\n",
           srb->sense_buf[2], srb->sense_buf[12],
           srb->sense_buf[13]);
     srb->pdata = (unsigned char *)ptr;
 	usb_dma_buffer_free(srb->pdata, ss->pusb_dev->bus);
     return 0;
 }

 static int usb_test_unit_ready(ccb *srb, struct us_data *ss)
{
     int retries = 10;

     do
    {
         rt_memset(&srb->cmd[0], 0, 12);
         srb->cmd[0] = SCSI_TST_U_RDY;
         srb->cmd[1] = srb->lun << 5;
         srb->datalen = 0;
         srb->cmdlen = 12;
         if (ss->transport(srb, ss) == USB_STOR_TRANSPORT_GOOD)
        {
             ss->flags |= USB_READY;
             return 0;
         }
         usb_request_sense(srb, ss);
         /*
          * Check the Key Code Qualifier, if it matches
          * "Not Ready - medium not present"
          * (the sense Key equals 0x2 and the ASC is 0x3a)
          * return immediately as the medium being absent won't change
          * unless there is a user action.
          */
         if ((srb->sense_buf[2] == 0x02) &&
             (srb->sense_buf[12] == 0x3a))
             return -1;
         rt_udelay(100);
     } while (retries--);

     return -1;
 }

 static int usb_read_capacity(ccb *srb, struct us_data *ss)
{
     int retry;
     /* XXX retries */
     retry = 3;
     do
    {
         rt_memset(&srb->cmd[0], 0, 12);
         srb->cmd[0] = SCSI_RD_CAPAC;
         srb->cmd[1] = srb->lun << 5;
         srb->datalen = 8;
         srb->cmdlen = 12;
         if (ss->transport(srb, ss) == USB_STOR_TRANSPORT_GOOD)
             return 0;
     } while (retry--);

     return -1;
 }
#define DOS_PART_DISKSIG_OFFSET    0x1b8
#define DOS_PART_TBL_OFFSET    0x1be
#define DOS_PART_MAGIC_OFFSET    0x1fe
#define DOS_PBR_FSTYPE_OFFSET    0x36
#define DOS_PBR32_FSTYPE_OFFSET    0x52
#define DOS_PBR_MEDIA_TYPE_OFFSET    0x15
#define DOS_MBR    0
#define DOS_PBR    1
 static int test_block_type(unsigned char *buffer)
{
     int slot;
     struct dos_partition *p;

     if ((buffer[DOS_PART_MAGIC_OFFSET + 0] != 0x55) ||
         (buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa)) {
         return (-1);
     } /* no DOS Signature at all */
     p = (struct dos_partition *)&buffer[DOS_PART_TBL_OFFSET];
     for (slot = 0; slot < 3; slot++)
    {
         if (p->boot_ind != 0 && p->boot_ind != 0x80)
        {
             if (!slot &&
                 (rt_strncmp((char *)&buffer[DOS_PBR_FSTYPE_OFFSET],
                      "FAT", 3) == 0 ||
                  rt_strncmp((char *)&buffer[DOS_PBR32_FSTYPE_OFFSET],
                      "FAT32", 5) == 0)) {
                 return DOS_PBR; /* is PBR */
             } else
            {
                 return -1;
             }
         }
     }
     return DOS_MBR;        /* Is MBR */
 }


 int test_part_dos (block_dev_desc_t *dev_desc,struct us_data *ss)
{
     /* unsigned char buffer[dev_desc->blksz]; */
 	unsigned char *buffer = usb_dma_buffer_alloc(dev_desc->blksz, ss->pusb_dev->bus);

     if (dev_desc->block_read(dev_desc->name, 0, 1, (ulong *) buffer) != 1)
     {
 		usb_dma_buffer_free(buffer, ss->pusb_dev->bus);
         return -1;
     }

     if (test_block_type(buffer) != DOS_MBR)
     {
 		usb_dma_buffer_free(buffer, ss->pusb_dev->bus);
         return -1;
     }
 	usb_dma_buffer_free(buffer, ss->pusb_dev->bus);
     return 0;
 }
 void init_part (block_dev_desc_t * dev_desc,struct us_data *ss)
{
	if (test_part_dos(dev_desc, ss) == 0)
    {
         dev_desc->part_type = PART_TYPE_DOS;
         return;
     }

     dev_desc->part_type = PART_TYPE_UNKNOWN;
 }

 int usb_stor_get_info(struct usb_device *dev, struct us_data *ss,
 		      block_dev_desc_t *dev_desc)
 {
     unsigned char perq, modi;
     unsigned long cap[2];
     unsigned char usb_stor_buf[36];
     unsigned long *capacity, *blksz;
     ccb *pccb = &usb_ccb;

 	pccb->pdata = usb_dma_buffer_alloc(36, ss->pusb_dev->bus);

     dev_desc->target = dev->devnum;
     pccb->lun = dev_desc->lun;
     rt_kprintf(" address %d\n", dev_desc->target);

     if (usb_inquiry(pccb, ss))
     {
 		usb_dma_buffer_free(pccb->pdata, ss->pusb_dev->bus);
         return -1;
     }
    rt_memcpy(usb_stor_buf, pccb->pdata, 36);
    usb_dma_buffer_free(pccb->pdata, ss->pusb_dev->bus);
     perq = usb_stor_buf[0];
     modi = usb_stor_buf[1];

     if ((perq & 0x1f) == 0x1f)
    {
         /* skip unknown devices */
         return 0;
     }
     if ((modi&0x80) == 0x80)
    {
         /* drive is removable */
         dev_desc->removable = 1;
     }
     rt_memcpy(&dev_desc->vendor[0], (const void *) &usb_stor_buf[8], 8);
     rt_memcpy(&dev_desc->product[0], (const void *) &usb_stor_buf[16], 16);
     rt_memcpy(&dev_desc->revision[0], (const void *) &usb_stor_buf[32], 4);
     dev_desc->vendor[8] = 0;
     dev_desc->product[16] = 0;
     dev_desc->revision[4] = 0;
/* #ifdef CONFIG_USB_BIN_FIXUP */
 /* usb_bin_fixup(dev->descriptor, (uchar *)dev_desc->vendor, */
/* (uchar *)dev_desc->product); */
/* #endif /* CONFIG_USB_BIN_FIXUP */
     rt_kprintf("ISO Vers %X, Response Data %X\n", usb_stor_buf[2],
           usb_stor_buf[3]);
     if (usb_test_unit_ready(pccb, ss))
    {
         rt_kprintf("Device NOT ready\n"
                "   Request Sense returned %02X %02X %02X\n",
                pccb->sense_buf[2], pccb->sense_buf[12],
                pccb->sense_buf[13]);
         if (dev_desc->removable == 1)
        {
             dev_desc->type = perq;
             return 1;
         }
         return 0;
     }
     /* pccb->pdata = (unsigned char *)&cap[0]; */
 	pccb->pdata = usb_dma_buffer_alloc(8, ss->pusb_dev->bus);
     rt_memset(pccb->pdata, 0, 8);
     if (usb_read_capacity(pccb, ss) != 0)
    {
         rt_kprintf("READ_CAP ERROR\n");
         cap[0] = 2880;
         cap[1] = 0x200;
     }
 	rt_memcpy(&cap[0], pccb->pdata, 8);
 	usb_dma_buffer_free(pccb->pdata, ss->pusb_dev->bus);
     ss->flags &= ~USB_READY;
     rt_kprintf("Read Capacity returns: 0x%lx, 0x%lx\n", cap[0], cap[1]);
 #if 0
     if (cap[0] > (0x200000 * 10)) /* greater than 10 GByte */
         cap[0] >>= 16;
 #endif
     cap[0] = swap32(cap[0]);
     cap[1] = swap32(cap[1]);

     /* this assumes bigendian! */
     cap[0] += 1;
     capacity = &cap[0];
     blksz = &cap[1];
     rt_kprintf("Capacity = 0x%lx, blocksz = 0x%lx\n", *capacity, *blksz);
     dev_desc->lba = *capacity;
     dev_desc->blksz = *blksz;
     /* dev_desc->log2blksz = LOG2(dev_desc->blksz); */
     dev_desc->type = perq;
     rt_kprintf(" address %d\n", dev_desc->target);
     rt_kprintf("partype: %d\n", dev_desc->part_type);


 	init_part(dev_desc, ss);

     /* int i=0; */
     /* void *addr=rt_malloc(122880); */
     /* for(i=0;i<10;i++) */
     /* usb_stor_read(dev_desc->name,0,240,addr); */

     rt_kprintf("partype: %d\n", dev_desc->part_type);
     return 1;
 }

static int storage_probe(struct usb_interface *intf,
             const struct usb_device_id *id)
{
    struct us_data *us;
    struct usb_endpoint_descriptor *ep_desc;
    struct usb_host_endpoint *endpoint;
    /* unsigned int flags = 0; */
    unsigned int i;

	RT_DEBUG_LOG(rt_debug_usb, (" USB Mass Storage driver setup success...\n"));

    rt_memset(&usb_dev_desc, 0, sizeof(block_dev_desc_t));
    usb_dev_desc.if_type = IF_TYPE_USB;
    usb_dev_desc.dev = intf->usb_dev->devnum;
	usb_dev_desc.name = intf->dev.parent.name;
    usb_dev_desc.part_type = PART_TYPE_UNKNOWN;
    usb_dev_desc.target = 0xff;
    usb_dev_desc.type = DEV_TYPE_UNKNOWN;
    usb_dev_desc.block_read = usb_stor_read;
    usb_dev_desc.block_write = usb_stor_write;


	us = rt_malloc(sizeof(*us));
	rt_memset(us, 0, sizeof(*us));
	us->pusb_intf = intf;
	us->pusb_dev = intf->usb_dev;
	us->protocol = intf->cur_altsetting->desc.bInterfaceProtocol;
	intf->user_data = us;
	intf->dev.user_data = intf;
/* us->cr=rt_malloc(sizeof(us->cr)); */
/* us->iobuf=rt_malloc(64); */

    /* If the device has subclass and protocol, then use that.  Otherwise,
         * take data from the specific interface.
         */

    us->subclass = intf->cur_altsetting->desc.bInterfaceSubClass;
    us->protocol = intf->cur_altsetting->desc.bInterfaceProtocol;

    switch (us->protocol)
    {
#if 0
        case US_PR_CB:
            rt_kprintf("Control/Bulk\n");
            us->transport = usb_stor_CB_transport;
            us->transport_reset = usb_stor_CB_reset;
            break;

        case US_PR_CBI:
            rt_kprintf("Control/Bulk/Interrupt\n");
            us->transport = usb_stor_CB_transport;
            us->transport_reset = usb_stor_CB_reset;
            break;
#endif
        case US_PR_BULK:
            rt_kprintf("Bulk/Bulk/Bulk\n");
            us->transport = usb_stor_BBB_transport;
            us->transport_reset = usb_stor_BBB_reset;
            break;
        default:
            rt_kprintf("USB Storage Transport unknown / not yet implemented\n");
            return 0;
            break;
        }

    /*
         * We are expecting a minimum of 2 endpoints - in and out (bulk).
         * An optional interrupt is OK (necessary for CBI protocol).
         * We will ignore any others.
         */
        for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++)
        {
			endpoint = intf->cur_altsetting->endpoint+i;
            ep_desc = &(endpoint->desc);
            /* is it an BULK endpoint? */
            if ((ep_desc->bmAttributes &
                 USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
                if (ep_desc->bEndpointAddress & USB_DIR_IN)
                    us->ep_in = ep_desc->bEndpointAddress &
                            USB_ENDPOINT_NUMBER_MASK;
                else
                    us->ep_out =
                        ep_desc->bEndpointAddress &
                        USB_ENDPOINT_NUMBER_MASK;
            }

            /* is it an interrupt endpoint? */
            if ((ep_desc->bmAttributes &
                 USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
                us->ep_int = ep_desc->bEndpointAddress &
                            USB_ENDPOINT_NUMBER_MASK;
                us->irqinterval = ep_desc->bInterval;
            }
        }
        rt_kprintf("Endpoints In %d Out %d Int %d\n",
              us->ep_in, us->ep_out, us->ep_int);


        /* Do some basic sanity checks, and bail if we find a problem */
            if (usb_set_interface(intf->usb_dev, intf->cur_altsetting->desc.bInterfaceNumber, 0) ||
               !us->ep_in || !us->ep_out ||
                (us->protocol == US_PR_CBI && us->ep_int == 0)) {
                rt_kprintf("Problems with device\n");
                return 0;
            }

            if (us->subclass != US_SC_UFI && us->subclass != US_SC_SCSI &&
                    us->subclass != US_SC_8070) {
                rt_kprintf("Sorry, protocol %d not yet supported.\n", us->subclass);
                    return 0;
                }
                if (us->ep_int)
                {
                    /* we had found an interrupt endpoint, prepare irq pipe
                     * set up the IRQ pipe and handler
                     */
                    us->irqinterval = (us->irqinterval > 0) ? us->irqinterval : 255;
                    us->irqpipe = usb_rcvintpipe(us->pusb_dev, us->ep_int);
					us->irqmaxp = usb_maxpacket(us->pusb_dev, us->irqpipe, usb_pipeout(us->irqpipe));
                    us->irq_handle = usb_stor_irq;
                }
                /* dev->privptr = (void *)us; */
                rt_device_register(&(intf->dev), intf->dev.parent.name, RT_DEVICE_FLAG_RDWR);
                us->max_lun = usb_get_max_lun(us);
				usb_dev_desc.lun = us->max_lun;
				usb_stor_get_info(intf->usb_dev, us, &usb_dev_desc);
                return 0;
}



static void usb_show_progress(void)
{
    rt_kprintf(".");
}

static int usb_read_10(ccb *srb, struct us_data *ss, unsigned long start,
               unsigned short blocks)
{
    rt_memset(&srb->cmd[0], 0, 12);
    srb->cmd[0] = SCSI_READ10;
    srb->cmd[1] = srb->lun << 5;
    srb->cmd[2] = ((unsigned char) (start >> 24)) & 0xff;
    srb->cmd[3] = ((unsigned char) (start >> 16)) & 0xff;
    srb->cmd[4] = ((unsigned char) (start >> 8)) & 0xff;
    srb->cmd[5] = ((unsigned char) (start)) & 0xff;
    srb->cmd[7] = ((unsigned char) (blocks >> 8)) & 0xff;
    srb->cmd[8] = (unsigned char) blocks & 0xff;
    srb->cmdlen = 12;
    rt_kprintf("read10: start %lx blocks %x\n", start, blocks);
    return ss->transport(srb, ss);
}

unsigned long usb_stor_read(char *name, unsigned long blknr,
        unsigned long blkcnt, void *buffer)
{
    unsigned long start, blks;
    unsigned long int buf_addr;
    unsigned short smallblks;
/* struct usb_device *dev; */
    struct usb_interface *intf;
    struct us_data *ss;
    int retry;
    ccb *srb = &usb_ccb;
	rt_device_t usb_storage = RT_NULL;

    if (blkcnt == 0)
        return 0;
    /* Setup  device */
    rt_kprintf("\nusb_read: dev %s\n", name);
	usb_storage = rt_device_find(name);
	if (usb_storage == RT_NULL)
    {
        rt_kprintf("%s can not find rt_device\n", __func__);
        return -1;

    }
    intf = usb_storage->user_data;
/* dev=intf->usb_dev; */
    ss = (struct us_data *)intf->user_data;

    /* usb_disable_asynch(1);  asynch transfer not allowed */

    srb->lun = usb_dev_desc.lun;
    buf_addr = (unsigned long)buffer;
    start = blknr;
    blks = blkcnt;

	rt_kprintf("\nusb_read: dev startblk %lx, blccnt %lx buffer %lx\n", start, blks, buf_addr);

    do
    {
        /* XXX need some comment here */
        retry = 2;
        srb->pdata = (unsigned char *)buf_addr;
        if (blks > USB_MAX_XFER_BLK)
            smallblks = USB_MAX_XFER_BLK;
        else
            smallblks = (unsigned short) blks;
retry_it:
        if (smallblks == USB_MAX_XFER_BLK)
            usb_show_progress();
        srb->datalen = usb_dev_desc.blksz * smallblks;
        srb->pdata = (unsigned char *)buf_addr;
        if (usb_read_10(srb, ss, start, smallblks))
        {
            rt_kprintf("Read ERROR\n");
            usb_request_sense(srb, ss);
            if (retry--)
                goto retry_it;
            blkcnt -= blks;
            break;
        }
        start += smallblks;
        blks -= smallblks;
        buf_addr += srb->datalen;
    } while (blks != 0);
    ss->flags &= ~USB_READY;

    rt_kprintf("usb_read: end startblk %lx, blccnt %x buffer %lx\n",
          start, smallblks, buf_addr);

/* usb_disable_asynch(0);  asynch transfer allowed  */
    if (blkcnt >= USB_MAX_XFER_BLK)
        rt_kprintf("\n");
    return blkcnt;
}


static int usb_write_10(ccb *srb, struct us_data *ss, unsigned long start,
            unsigned short blocks)
{
    rt_memset(&srb->cmd[0], 0, 12);
    srb->cmd[0] = SCSI_WRITE10;
    srb->cmd[1] = srb->lun << 5;
    srb->cmd[2] = ((unsigned char) (start >> 24)) & 0xff;
    srb->cmd[3] = ((unsigned char) (start >> 16)) & 0xff;
    srb->cmd[4] = ((unsigned char) (start >> 8)) & 0xff;
    srb->cmd[5] = ((unsigned char) (start)) & 0xff;
    srb->cmd[7] = ((unsigned char) (blocks >> 8)) & 0xff;
    srb->cmd[8] = (unsigned char) blocks & 0xff;
    srb->cmdlen = 12;
    rt_kprintf("write10: start %lx blocks %x\n", start, blocks);
    return ss->transport(srb, ss);
}


unsigned long usb_stor_write(char *name, unsigned long blknr,
                 unsigned long blkcnt,  void *buffer)
{
    unsigned long start, blks;
    unsigned long int buf_addr;
    unsigned short smallblks;
    struct usb_interface *intf;
    /* struct usb_device *dev; */
    struct us_data *ss;
    int retry;
    ccb *srb = &usb_ccb;
	rt_device_t usb_storage = RT_NULL;

    if (blkcnt == 0)
        return 0;

    /* device &= 0xff; */
    /* Setup  device */
    rt_kprintf("\nusb_write: dev %s\n", name);
	usb_storage = rt_device_find(name);
		if (usb_storage == RT_NULL)
        {
            rt_kprintf("%s can not find rt_device\n", __func__);
            return -1;

        }

        intf = usb_storage->user_data;
        /* dev=intf->usb_dev; */
        ss = (struct us_data *)intf->user_data;
    /* ss = (struct us_data *)usb_storage->user_data; */

/* usb_disable_asynch(1);  asynch transfer not allowed  */

    srb->lun = usb_dev_desc.lun;
    buf_addr = (unsigned long)buffer;
    start = blknr;
    blks = blkcnt;

    rt_kprintf("\nusb_write:  startblk %lx, blccnt %lx buffer %lx\n", start, blks, buf_addr);

    do
    {
        /* If write fails retry for max retry count else
         * return with number of blocks written successfully.
         */
        retry = 2;
        srb->pdata = (unsigned char *)buf_addr;
        if (blks > USB_MAX_XFER_BLK)
            smallblks = USB_MAX_XFER_BLK;
        else
            smallblks = (unsigned short) blks;
retry_it:
        if (smallblks == USB_MAX_XFER_BLK)
            usb_show_progress();
        srb->datalen = usb_dev_desc.blksz * smallblks;
        srb->pdata = (unsigned char *)buf_addr;
        if (usb_write_10(srb, ss, start, smallblks))
        {
            rt_kprintf("Write ERROR\n");
            usb_request_sense(srb, ss);
            if (retry--)
                goto retry_it;
            blkcnt -= blks;
            break;
        }
        start += smallblks;
        blks -= smallblks;
        buf_addr += srb->datalen;
    } while (blks != 0);
    ss->flags &= ~USB_READY;

    rt_kprintf("usb_write: end startblk %lx, blccnt %x buffer %lx\n",
          start, smallblks, buf_addr);

    /* usb_disable_asynch(0);  asynch transfer allowed  */
    if (blkcnt >= USB_MAX_XFER_BLK)
        rt_kprintf("\n");
    return blkcnt;
}
void usb_stor_disconnect(struct usb_interface *intf)
{
	RT_DEBUG_LOG(rt_debug_usb, (" USB Mass Storage driver disconnect start...\n"));
    struct us_data *us;

	us = intf->user_data;
    /* rt_free(us->iobuf); */
    /* rt_free(us->cr); */
    /* usb_put_urb(us->current_urb); */
    /* rt_thread_delete(us->ctl_thread); */
    rt_free(us);
    rt_device_unregister(&intf->dev);
	RT_DEBUG_LOG(rt_debug_usb, (" USB Mass Storage driver disconnect success...\n"));
}

static struct usb_driver usb_storage_driver = {
    .name =        "usb-storage",
    .probe =    storage_probe,
    .disconnect =    usb_stor_disconnect,
    .id_table =    usb_storage_usb_ids,
};

 int  usb_stor_init(void)
{
    int retval;

	RT_DEBUG_LOG(rt_debug_usb, ("Initializing USB Mass Storage driver...\n"));

    /* register the driver, return usb_register return code if error */
    retval = usb_register_driver(&usb_storage_driver);
    if (retval == 0)
    {
		RT_DEBUG_LOG(rt_debug_usb, ("USB Mass Storage support registered.\n"));
    }
    return retval;
}

