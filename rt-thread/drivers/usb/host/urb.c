
/* #include <usb.h> */
#include <hcd.h>
#include "usb_errno.h"



#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static inline int fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u))
    {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u))
    {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u))
    {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u))
    {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u))
    {
        x <<= 1;
        r -= 1;
    }
    return r;
}


static void urb_destroy(struct urb *urb)
{

/* if (urb->transfer_flags & URB_FREE_BUFFER) */
/* { */
/* rt_free(urb->transfer_buffer); */
/* RT_DEBUG_LOG(rt_debug_usb,("rt_free urb transfer_buffer :%d\n",--g_mem_debug)); */
/* } */

    /* RT_DEBUG_LOG(rt_debug_usb,("rt_free urb :%d,addr:0x%x\n",--g_mem_debug,urb)); */
    rt_free(urb);
}

/**
 * usb_init_urb - initializes a urb so that it can be used by a USB driver
 * @urb: pointer to the urb to initialize
 *
 * Initializes a urb so that the USB subsystem can use it properly.
 *
 * If a urb is created with a call to usb_alloc_urb() it is not
 * necessary to call this function.  Only use this if you allocate the
 * space for a struct urb on your own.  If you call this function, be
 * careful when freeing the memory for your urb that it is no longer in
 * use by the USB core.
 *
 * Only use this function if you _really_ understand what you are doing.
 */
void usb_init_urb(struct urb *urb)
{
    if (urb)
    {
        rt_memset(urb, 0, sizeof(*urb));
        urb->kref = 1;
/* rt_list_init(&urb->anchor_list); */
    }
}

/**
 * usb_alloc_urb - creates a new urb for a USB driver to use
 * @iso_packets: number of iso packets for this urb
 * @mem_flags: the type of memory to allocate, see kmalloc() for a list of
 *    valid options for this.
 *
 * Creates an urb for the USB driver to use, initializes a few internal
 * structures, incrementes the usage counter, and returns a pointer to it.
 *
 * If no memory is available, NULL is returned.
 *
 * If the driver want to use this urb for interrupt, control, or bulk
 * endpoints, pass '0' as the number of iso packets.
 *
 * The driver must call usb_free_urb() when it is finished with the urb.
 */
struct urb *usb_alloc_urb(int iso_packets, int mem_flags)
{
    struct urb *urb;

    urb = rt_malloc(sizeof(struct urb) +
        iso_packets * sizeof(struct usb_iso_packet_descriptor));
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc urb:%d,size:%d,addr:0x%x\n",g_mem_debug++,(sizeof(struct urb) + */
    /* iso_packets * sizeof(struct usb_iso_packet_descriptor)),urb)); */
    if (!urb)
    {
        rt_kprintf("alloc_urb: kmalloc failed\n");
        return RT_NULL;
    }
    usb_init_urb(urb);
    return urb;
}

/**
 * usb_free_urb - frees the memory used by a urb when all users of it are finished
 * @urb: pointer to the urb to free, may be NULL
 *
 * Must be called when a user of a urb is finished with it.  When the last user
 * of the urb calls this function, the memory of the urb is freed.
 *
 * Note: The transfer buffer associated with the urb is not freed unless the
 * URB_FREE_BUFFER transfer flag is set.
 */
void usb_free_urb(struct urb *urb)
{
	if(!urb)
	{
		rt_kprintf("%s:failed urb is null",__func__);
        return;
	}
    urb->kref--;
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_free_urb kref:%d\n",urb->kref)); */
    if (urb && (urb->kref == 0))
    urb_destroy(urb);
}

/**
 * usb_get_urb - increments the reference count of the urb
 * @urb: pointer to the urb to modify, may be NULL
 *
 * This must be  called whenever a urb is transferred from a device driver to a
 * host controller driver.  This allows proper reference counting to happen
 * for urbs.
 *
 * A pointer to the urb with the incremented reference counter is returned.
 */
struct urb *usb_get_urb(struct urb *urb)
{
    if (urb)
        urb->kref++;
    return urb;
}


/*-------------------------------------------------------------------*/

/**
 * usb_submit_urb - issue an asynchronous transfer request for an endpoint
 * @urb: pointer to the urb describing the request
 * @mem_flags: the type of memory to allocate, see kmalloc() for a list
 *    of valid options for this.
 *
 * This submits a transfer request, and transfers control of the URB
 * describing that request to the USB subsystem.  Request completion will
 * be indicated later, asynchronously, by calling the completion handler.
 * The three types of completion are success, error, and unlink
 * (a software-induced fault, also called "request cancellation").
 *
 * URBs may be submitted in interrupt context.
 *
 * The caller must have correctly initialized the URB before submitting
 * it.  Functions such as usb_fill_bulk_urb() and usb_fill_control_urb() are
 * available to ensure that most fields are correctly initialized, for
 * the particular kind of transfer, although they will not initialize
 * any transfer flags.
 *
 * Successful submissions return 0; otherwise this routine returns a
 * negative error number.  If the submission is successful, the complete()
 * callback from the URB will be called exactly once, when the USB core and
 * Host Controller Driver (HCD) are finished with the URB.  When the completion
 * function is called, control of the URB is returned to the device
 * driver which issued the request.  The completion handler may then
 * immediately free or reuse that URB.
 *
 * With few exceptions, USB device drivers should never access URB fields
 * provided by usbcore or the HCD until its complete() is called.
 * The exceptions relate to periodic transfer scheduling.  For both
 * interrupt and isochronous urbs, as part of successful URB submission
 * urb->interval is modified to reflect the actual transfer period used
 * (normally some power of two units).  And for isochronous urbs,
 * urb->start_frame is modified to reflect when the URB's transfers were
 * scheduled to start.  Not all isochronous transfer scheduling policies
 * will work, but most host controller drivers should easily handle ISO
 * queues going from now until 10-200 msec into the future.
 *
 * For control endpoints, the synchronous usb_control_msg() call is
 * often used (in non-interrupt context) instead of this call.
 * That is often used through convenience wrappers, for the requests
 * that are standardized in the USB 2.0 specification.  For bulk
 * endpoints, a synchronous usb_bulk_msg() call is available.
 *
 * Request Queuing:
 *
 * URBs may be submitted to endpoints before previous ones complete, to
 * minimize the impact of interrupt latencies and system overhead on data
 * throughput.  With that queuing policy, an endpoint's queue would never
 * be empty.  This is required for continuous isochronous data streams,
 * and may also be required for some kinds of interrupt transfers. Such
 * queuing also maximizes bandwidth utilization by letting USB controllers
 * start work on later requests before driver software has finished the
 * completion processing for earlier (successful) requests.
 *
 * As of Linux 2.6, all USB endpoint transfer queues support depths greater
 * than one.  This was previously a HCD-specific behavior, except for ISO
 * transfers.  Non-isochronous endpoint queues are inactive during cleanup
 * after faults (transfer errors or cancellation).
 *
 * Reserved Bandwidth Transfers:
 *
 * Periodic transfers (interrupt or isochronous) are performed repeatedly,
 * using the interval specified in the urb.  Submitting the first urb to
 * the endpoint reserves the bandwidth necessary to make those transfers.
 * If the USB subsystem can't allocate sufficient bandwidth to perform
 * the periodic request, submitting such a periodic request should fail.
 *
 * For devices under xHCI, the bandwidth is reserved at configuration time, or
 * when the alt setting is selected.  If there is not enough bus bandwidth, the
 * configuration/alt setting request will fail.  Therefore, submissions to
 * periodic endpoints on devices under xHCI should never fail due to bandwidth
 * constraints.
 *
 * Device drivers must explicitly request that repetition, by ensuring that
 * some URB is always on the endpoint's queue (except possibly for short
 * periods during completion callacks).  When there is no longer an urb
 * queued, the endpoint's bandwidth reservation is canceled.  This means
 * drivers can use their completion handlers to ensure they keep bandwidth
 * they need, by reinitializing and resubmitting the just-completed urb
 * until the driver longer needs that periodic bandwidth.
 *
 * Memory Flags:
 *
 * The general rules for how to decide which mem_flags to use
 * are the same as for kmalloc.  There are four
 * different possible values; GFP_KERNEL, GFP_NOFS, GFP_NOIO and
 * GFP_ATOMIC.
 *
 * GFP_NOFS is not ever used, as it has not been implemented yet.
 *
 * GFP_ATOMIC is used when
 *   (a) you are inside a completion handler, an interrupt, bottom half,
 *       tasklet or timer, or
 *   (b) you are holding a spinlock or rwlock (does not apply to
 *       semaphores), or
 *   (c) current->state != TASK_RUNNING, this is the case only after
 *       you've changed it.
 *
 * GFP_NOIO is used in the block io path and error handling of storage
 * devices.
 *
 * All other situations use GFP_KERNEL.
 *
 * Some more specific rules for mem_flags can be inferred, such as
 *  (1) start_xmit, timeout, and receive methods of network drivers must
 *      use GFP_ATOMIC (they are called with a spinlock held);
 *  (2) queuecommand methods of scsi drivers must use GFP_ATOMIC (also
 *      called with a spinlock held);
 *  (3) If you use a kernel thread with a network driver you must use
 *      GFP_NOIO, unless (b) or (c) apply;
 *  (4) after you have done a down() you can use GFP_KERNEL, unless (b) or (c)
 *      apply or your are in a storage driver's block io path;
 *  (5) USB probe and disconnect can use GFP_KERNEL unless (b) or (c) apply; and
 *  (6) changing firmware on a running storage or net device uses
 *      GFP_NOIO, unless b) or c) apply
 *
 */
extern int fls(int x);

int usb_submit_urb(struct urb *urb, int mem_flags)
{
    int                xfertype, max;
    struct usb_device        *dev;
    struct usb_host_endpoint    *ep;
    int                is_out;

    if (!urb || urb->hcpriv || !urb->complete)
        return -EINVAL;
    dev = urb->dev;
    if ((!dev) || (dev->state < USB_STATE_UNAUTHENTICATED))
        return -ENODEV;

    /* For now, get the endpoint from the pipe.  Eventually drivers
     * will be required to set urb->ep directly and we will eliminate
     * urb->pipe.
     */
    ep = usb_pipe_endpoint(dev, urb->pipe);
    if (!ep)
        return -ENOENT;

    urb->ep = ep;
    urb->status = -EINPROGRESS;
    urb->actual_length = 0;

    /* Lots of sanity checks, so HCDs can rely on clean data
     * and don't need to duplicate tests
     */
    xfertype = usb_endpoint_type(&ep->desc);
    if (xfertype == USB_ENDPOINT_XFER_CONTROL)
    {
        struct usb_ctrlrequest *setup =
                (struct usb_ctrlrequest *) urb->setup_packet;

        if (!setup)
            return -ENOEXEC;
        is_out = !(setup->bRequestType & USB_DIR_IN) ||
                !setup->wLength;
    } else
    {
        is_out = usb_endpoint_dir_out(&ep->desc);
    }

    /* Clear the internal flags and cache the direction for later use */
    urb->transfer_flags &= ~(URB_DIR_MASK | URB_DMA_MAP_SINGLE |
            URB_DMA_MAP_PAGE | URB_DMA_MAP_SG | URB_MAP_LOCAL |
            URB_SETUP_MAP_SINGLE | URB_SETUP_MAP_LOCAL |
            URB_DMA_SG_COMBINED);
    urb->transfer_flags |= (is_out ? URB_DIR_OUT : URB_DIR_IN);

    if (xfertype != USB_ENDPOINT_XFER_CONTROL &&
            dev->state < USB_STATE_CONFIGURED)
        return -ENODEV;

    max = (ep->desc.wMaxPacketSize);
    if (max <= 0)
    {
        RT_DEBUG_LOG(rt_debug_usb,
            ("bogus endpoint ep%d%s in (bad maxpacket %d)\n",
            usb_endpoint_num(&ep->desc), is_out ? "out" : "in",
            max));
        return -EMSGSIZE;
    }

    /* periodic transfers limit size per frame/uframe,
     * but drivers only control those sizes for ISO.
     * while we're checking, initialize return status.
     */
    if (xfertype == USB_ENDPOINT_XFER_ISOC)
    {
        int    n, len;

        /* SuperSpeed isoc endpoints have up to 16 bursts of up to
         * 3 packets each
         */
        if (dev->speed == USB_SPEED_SUPER)
        {
            int     burst = 1 + ep->ss_ep_comp.bMaxBurst;
            int     mult = USB_SS_MULT(ep->ss_ep_comp.bmAttributes);

            max *= burst;
            max *= mult;
        }

        /* "high bandwidth" mode, 1-3 packets/uframe? */
        if (dev->speed == USB_SPEED_HIGH)
        {
            int    mult = 1 + ((max >> 11) & 0x03);

            max &= 0x07ff;
            max *= mult;
        }

        if (urb->number_of_packets <= 0)
            return -EINVAL;
        for (n = 0; n < urb->number_of_packets; n++)
        {
            len = urb->iso_frame_desc[n].length;
            if (len < 0 || len > max)
                return -EMSGSIZE;
            urb->iso_frame_desc[n].status = -EXDEV;
            urb->iso_frame_desc[n].actual_length = 0;
        }
    }

    /* the I/O buffer must be mapped/unmapped, except when length=0 */
    if (urb->transfer_buffer_length > 0xfffffff)
        return -EMSGSIZE;

#if 0
    /* stuff that drivers shouldn't do, but which shouldn't
     * cause problems in HCDs if they get it wrong.
     */
    {
    unsigned int    orig_flags = urb->transfer_flags;
    unsigned int    allowed;
    static int pipetypes[4] = {
        PIPE_CONTROL, PIPE_ISOCHRONOUS, PIPE_BULK, PIPE_INTERRUPT
    };

    /* Check that the pipe's type matches the endpoint's type */
    if (usb_pipetype(urb->pipe) != pipetypes[xfertype])
    {
        RT_DEBUG_LOG(rt_debug_usb, ("BOGUS urb xfer, pipe %x != type %x\n",
            usb_pipetype(urb->pipe), pipetypes[xfertype]));
        return -EPIPE;        /* The most suitable error code :-) */
    }

    /* enforce simple/standard policy */
    allowed = (URB_NO_TRANSFER_DMA_MAP | URB_NO_INTERRUPT | URB_DIR_MASK |
            URB_FREE_BUFFER);
    switch (xfertype)
    {
    case USB_ENDPOINT_XFER_BULK:
        if (is_out)
            allowed |= URB_ZERO_PACKET;
        /* FALLTHROUGH */
    case USB_ENDPOINT_XFER_CONTROL:
        allowed |= URB_NO_FSBR;    /* only affects UHCI */
        /* FALLTHROUGH */
    default:            /* all non-iso endpoints */
        if (!is_out)
            allowed |= URB_SHORT_NOT_OK;
        break;
    case USB_ENDPOINT_XFER_ISOC:
        allowed |= URB_ISO_ASAP;
        break;
    }
    urb->transfer_flags &= allowed;

    /* fail if submitter gave bogus flags */
    if (urb->transfer_flags != orig_flags)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("BOGUS urb flags, %x --> %x\n",
            orig_flags, urb->transfer_flags));
        return -EINVAL;
    }
    }
#endif
    /*
     * Force periodic transfer intervals to be legal values that are
     * a power of two (so HCDs don't need to).
     *
     * FIXME want bus->{intr,iso}_sched_horizon values here.  Each HC
     * supports different values... this uses EHCI/UHCI defaults (and
     * EHCI can use smaller non-default values).
     */
    switch (xfertype)
    {
    case USB_ENDPOINT_XFER_ISOC:
    case USB_ENDPOINT_XFER_INT:
        /* too small? */
        switch (dev->speed)
        {
        case USB_SPEED_WIRELESS:
            if (urb->interval < 6)
                return -EINVAL;
            break;
        default:
            if (urb->interval <= 0)
                return -EINVAL;
            break;
        }
        /* too big? */
        switch (dev->speed)
        {
        case USB_SPEED_SUPER:    /* units are 125us */
            /* Handle up to 2^(16-1) microframes */
            if (urb->interval > (1 << 15))
                return -EINVAL;
            max = 1 << 15;
            break;
        case USB_SPEED_WIRELESS:
            if (urb->interval > 16)
                return -EINVAL;
            break;
        case USB_SPEED_HIGH:    /* units are microframes */
            /* NOTE usb handles 2^15 */
            if (urb->interval > (1024 * 8))
                urb->interval = 1024 * 8;
            max = 1024 * 8;
            break;
        case USB_SPEED_FULL:    /* units are frames/msec */
        case USB_SPEED_LOW:
            if (xfertype == USB_ENDPOINT_XFER_INT)
            {
                if (urb->interval > 255)
                    return -EINVAL;
                /* NOTE ohci only handles up to 32 */
                max = 128;
            } else
            {
                if (urb->interval > 1024)
                    urb->interval = 1024;
                /* NOTE usb and ohci handle up to 2^15 */
                max = 1024;
            }
            break;
        default:
            return -EINVAL;
        }
        if (dev->speed != USB_SPEED_WIRELESS)
        {
            /* Round down to a power of 2, no more than max */
            urb->interval = min(max, 1 << fls(urb->interval));
            /* rt_kprintf("interval:%x\n",urb->interval); */
        }
    }

    return usb_hcd_submit_urb(urb);
}

/*-------------------------------------------------------------------*/

/**
 * usb_unlink_urb - abort/cancel a transfer request for an endpoint
 * @urb: pointer to urb describing a previously submitted request,
 *    may be NULL
 *
 * This routine cancels an in-progress request.  URBs complete only once
 * per submission, and may be canceled only once per submission.
 * Successful cancellation means termination of @urb will be expedited
 * and the completion handler will be called with a status code
 * indicating that the request has been canceled (rather than any other
 * code).
 *
 * Drivers should not call this routine or related routines, such as
 * usb_kill_urb() or usb_unlink_anchored_urbs(), after their disconnect
 * method has returned.  The disconnect function should synchronize with
 * a driver's I/O routines to insure that all URB-related activity has
 * completed before it returns.
 *
 * This request is always asynchronous.  Success is indicated by
 * returning -EINPROGRESS, at which time the URB will probably not yet
 * have been given back to the device driver.  When it is eventually
 * called, the completion function will see @urb->status == -ECONNRESET.
 * Failure is indicated by usb_unlink_urb() returning any other value.
 * Unlinking will fail when @urb is not currently "linked" (i.e., it was
 * never submitted, or it was unlinked before, or the hardware is already
 * finished with it), even if the completion handler has not yet run.
 *
 * Unlinking and Endpoint Queues:
 *
 * [The behaviors and guarantees described below do not apply to virtual
 * root hubs but only to endpoint queues for physical USB devices.]
 *
 * Host Controller Drivers (HCDs) place all the URBs for a particular
 * endpoint in a queue.  Normally the queue advances as the controller
 * hardware processes each request.  But when an URB terminates with an
 * error its queue generally stops (see below), at least until that URB's
 * completion routine returns.  It is guaranteed that a stopped queue
 * will not restart until all its unlinked URBs have been fully retired,
 * with their completion routines run, even if that's not until some time
 * after the original completion handler returns.  The same behavior and
 * guarantee apply when an URB terminates because it was unlinked.
 *
 * Bulk and interrupt endpoint queues are guaranteed to stop whenever an
 * URB terminates with any sort of error, including -ECONNRESET, -ENOENT,
 * and -EREMOTEIO.  Control endpoint queues behave the same way except
 * that they are not guaranteed to stop for -EREMOTEIO errors.  Queues
 * for isochronous endpoints are treated differently, because they must
 * advance at fixed rates.  Such queues do not stop when an URB
 * encounters an error or is unlinked.  An unlinked isochronous URB may
 * leave a gap in the stream of packets; it is undefined whether such
 * gaps can be filled in.
 *
 * Note that early termination of an URB because a short packet was
 * received will generate a -EREMOTEIO error if and only if the
 * URB_SHORT_NOT_OK flag is set.  By setting this flag, USB device
 * drivers can build deep queues for large or complex bulk transfers
 * and clean them up reliably after any sort of aborted transfer by
 * unlinking all pending URBs at the first fault.
 *
 * When a control URB terminates with an error other than -EREMOTEIO, it
 * is quite likely that the status stage of the transfer will not take
 * place.
 */
int usb_unlink_urb(struct urb *urb)
{
    if (!urb)
        return -EINVAL;
    if (!urb->dev)
        return -ENODEV;
    if (!urb->ep)
        return -EIDRM;
    return usb_hcd_unlink_urb(urb, -ECONNRESET);
}

/**
 * usb_kill_urb - cancel a transfer request and wait for it to finish
 * @urb: pointer to URB describing a previously submitted request,
 *    may be NULL
 *
 * This routine cancels an in-progress request.  It is guaranteed that
 * upon return all completion handlers will have finished and the URB
 * will be totally idle and available for reuse.  These features make
 * this an ideal way to stop I/O in a disconnect() callback or close()
 * function.  If the request has not already finished or been unlinked
 * the completion handler will see urb->status == -ENOENT.
 *
 * While the routine is running, attempts to resubmit the URB will fail
 * with error -EPERM.  Thus even if the URB's completion handler always
 * tries to resubmit, it will not succeed and the URB will become idle.
 *
 * This routine may not be used in an interrupt context (such as a bottom
 * half or a completion handler), or when holding a spinlock, or in other
 * situations where the caller can't schedule().
 *
 * This routine should not be called by a driver after its disconnect
 * method has returned.
 */
void usb_kill_urb(struct urb *urb)
{
    if (!(urb && urb->dev && urb->ep))
        return;


    usb_hcd_unlink_urb(urb, -ENOENT);
}

