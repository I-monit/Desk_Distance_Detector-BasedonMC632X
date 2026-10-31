/*
 * message.c - synchronous message handling
 */

#include <quirks.h>
#include <hcd.h>
#include <rtthread.h>
#include <rtdef.h>
#include <rtdevice.h>
#include <sys/types.h>
#include "usb_errno.h"

static void cancel_async_set_config(struct usb_device *udev)
{
}

struct api_context
{
    struct rt_completion    done;
    int            status;
};

static void usb_api_blocking_completion(struct urb *urb)
{
    struct api_context *ctx = urb->context;

    ctx->status = urb->status;
    rt_completion_done(&ctx->done);
}


/*
 * Starts urb and waits for completion or timeout. Note that this call
 * is NOT interruptible. Many device driver i/o requests should be
 * interruptible and therefore these drivers should implement their
 * own interruptible routines.
 */
static int usb_start_wait_urb(struct urb *urb, int timeout, int *actual_length)
{
    struct api_context ctx;
    int retval;

    rt_completion_init(&ctx.done);
    urb->context = &ctx;
    urb->actual_length = 0;
    retval = usb_submit_urb(urb, 0);
    if (retval)
        goto out;

    if (rt_completion_wait(&ctx.done, (timeout*RT_TICK_PER_SECOND/1000)))
    {
        usb_kill_urb(urb);
        retval = (ctx.status == -ENOENT ? -ETIMEDOUT : ctx.status);

        RT_DEBUG_LOG(rt_debug_usb,
            ("timed out on ep%d%s len=%u/%u\n",
            usb_endpoint_num(&urb->ep->desc),
            usb_urb_dir_in(urb) ? "in" : "out",
            urb->actual_length,
            urb->transfer_buffer_length));
    }
    else
        retval = ctx.status;
out:
    if (actual_length)
        *actual_length = urb->actual_length;
    usb_free_urb(urb);
    return retval;
}

/*-------------------------------------------------------------------*/
/* returns status (negative) or length (positive) */
static int usb_internal_control_msg(struct usb_device *usb_dev,
                    unsigned int pipe,
                    struct usb_ctrlrequest *cmd,
                    void *data, int len, int timeout)
{
    struct urb *urb;
    int retv;
    int length;

    urb = usb_alloc_urb(0, 0);
    if (!urb)
        return -ENOMEM;

    usb_fill_control_urb(urb, usb_dev, pipe, (unsigned char *)cmd, data,
                 len, usb_api_blocking_completion, NULL);

    retv = usb_start_wait_urb(urb, timeout, &length);
    if (retv < 0)
        return retv;
    else
        return length;
}

/**
 * usb_control_msg - Builds a control urb, sends it off and waits for completion
 * @dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @request: USB message request value
 * @requesttype: USB message request type value
 * @value: USB message value
 * @index: USB message index value
 * @data: pointer to the data to send
 * @size: length in bytes of the data to send
 * @timeout: time in msecs to wait for the message to complete before timing
 *    out (if 0 the wait is forever)
 *
 * Context: !in_interrupt ()
 *
 * This function sends a simple control message to a specified endpoint and
 * waits for the message to complete, or timeout.
 *
 * If successful, it returns the number of bytes transferred, otherwise a
 * negative error number.
 *
 * Don't use this function from within an interrupt context, like a bottom half
 * handler.  If you need an asynchronous message, or need to send a message
 * from within interrupt context, use usb_submit_urb().
 * If a thread in your driver uses this call, make sure your disconnect()
 * method can wait for it to complete.  Since you don't have a handle on the
 * URB used, you can't cancel the request.
 */
int usb_control_msg(struct usb_device *dev, unsigned int pipe, rt_uint8_t request,
            rt_uint8_t requesttype, rt_uint16_t value, rt_uint16_t index, void *data,
            rt_uint16_t size, int timeout)
{
    struct usb_ctrlrequest *dr;
    int ret;

    /* dr =rt_malloc(sizeof(struct usb_ctrlrequest)); */
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc usb_ctrlrequest :%d size:%d\n",g_mem_debug++,sizeof(struct usb_ctrlrequest))); */
    dr = usb_dma_buffer_alloc(sizeof(struct usb_ctrlrequest), dev->bus);
    if (!dr)
        return -ENOMEM;

    dr->bRequestType = requesttype;
    dr->bRequest = request;
    dr->wValue = value;
    dr->wIndex = index;
    dr->wLength = size;

    /* dbg("usb_control_msg"); */

    ret = usb_internal_control_msg(dev, pipe, dr, data, size, timeout);

    /* RT_DEBUG_LOG(rt_debug_usb,("rt_free message usb_ctrlrequest:%d\n",--g_mem_debug)); */
     /* rt_free(dr); */
    usb_dma_buffer_free(dr, dev->bus);
    return ret;
}

/**
 * usb_interrupt_msg - Builds an interrupt urb, sends it off and waits for completion
 * @usb_dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @data: pointer to the data to send
 * @len: length in bytes of the data to send
 * @actual_length: pointer to a location to put the actual length transferred
 *    in bytes
 * @timeout: time in msecs to wait for the message to complete before
 *    timing out (if 0 the wait is forever)
 *
 * Context: !in_interrupt ()
 *
 * This function sends a simple interrupt message to a specified endpoint and
 * waits for the message to complete, or timeout.
 *
 * If successful, it returns 0, otherwise a negative error number.  The number
 * of actual bytes transferred will be stored in the actual_length paramater.
 *
 * Don't use this function from within an interrupt context, like a bottom half
 * handler.  If you need an asynchronous message, or need to send a message
 * from within interrupt context, use usb_submit_urb() If a thread in your
 * driver uses this call, make sure your disconnect() method can wait for it to
 * complete.  Since you don't have a handle on the URB used, you can't cancel
 * the request.
 */
int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
              void *data, int len, int *actual_length, int timeout)
{
    return usb_bulk_msg(usb_dev, pipe, data, len, actual_length, timeout);
}

/**
 * usb_bulk_msg - Builds a bulk urb, sends it off and waits for completion
 * @usb_dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @data: pointer to the data to send
 * @len: length in bytes of the data to send
 * @actual_length: pointer to a location to put the actual length transferred
 *    in bytes
 * @timeout: time in msecs to wait for the message to complete before
 *    timing out (if 0 the wait is forever)
 *
 * Context: !in_interrupt ()
 *
 * This function sends a simple bulk message to a specified endpoint
 * and waits for the message to complete, or timeout.
 *
 * If successful, it returns 0, otherwise a negative error number.  The number
 * of actual bytes transferred will be stored in the actual_length paramater.
 *
 * Don't use this function from within an interrupt context, like a bottom half
 * handler.  If you need an asynchronous message, or need to send a message
 * from within interrupt context, use usb_submit_urb() If a thread in your
 * driver uses this call, make sure your disconnect() method can wait for it to
 * complete.  Since you don't have a handle on the URB used, you can't cancel
 * the request.
 *
 * Because there is no usb_interrupt_msg() and no USBDEVFS_INTERRUPT ioctl,
 * users are forced to abuse this routine by using it to submit URBs for
 * interrupt endpoints.  We will take the liberty of creating an interrupt URB
 * (with the default interval) if the target is an interrupt endpoint.
 */
int usb_bulk_msg(struct usb_device *usb_dev, unsigned int pipe,
         void *data, int len, int *actual_length, int timeout)
{
    struct urb *urb;
    struct usb_host_endpoint *ep;

    ep = usb_pipe_endpoint(usb_dev, pipe);
    if (!ep || len < 0)
        return -EINVAL;

    urb = usb_alloc_urb(0, 0);
    if (!urb)
        return -ENOMEM;

    if ((ep->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
            USB_ENDPOINT_XFER_INT) {
        pipe = (pipe & ~(3 << 30)) | (PIPE_INTERRUPT << 30);
        usb_fill_int_urb(urb, usb_dev, pipe, data, len,
                usb_api_blocking_completion, NULL,
                ep->desc.bInterval);
    }
    else
        usb_fill_bulk_urb(urb, usb_dev, pipe, data, len,
                usb_api_blocking_completion, NULL);

    return usb_start_wait_urb(urb, timeout, actual_length);
}


/*-------------------------------------------------------------------*/

/**
 * usb_get_descriptor - issues a generic GET_DESCRIPTOR request
 * @dev: the device whose descriptor is being retrieved
 * @type: the descriptor type (USB_DT_*)
 * @index: the number of the descriptor
 * @buf: where to put the descriptor
 * @size: how big is "buf"?
 * Context: !in_interrupt ()
 *
 * Gets a USB descriptor.  Convenience functions exist to simplify
 * getting some types of descriptors.  Use
 * usb_get_string() or usb_string() for USB_DT_STRING.
 * Device (USB_DT_DEVICE) and configuration descriptors (USB_DT_CONFIG)
 * are part of the device structure.
 * In addition to a number of USB-standard descriptors, some
 * devices also use class-specific or vendor-specific descriptors.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_descriptor(struct usb_device *dev, unsigned char type,
               unsigned char index, void *buf, int size)
{
    int i;
    int result;

    rt_memset(buf, 0, size);    /* Make sure we parse really received data */

    for (i = 0; i < 3; ++i)
    {
        /* retry on length 0 or error; some devices are flakey */
        result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
                USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
                (type << 8) + index, 0, buf, size,
                USB_CTRL_GET_TIMEOUT);
        if (result <= 0 && result != -ETIMEDOUT)
            continue;
        if (result > 1 && ((rt_uint8_t *)buf)[1] != type)
        {
            result = -ENODATA;
            continue;
        }
        break;
    }
    return result;
}

int usb_get_string(struct usb_device *dev, unsigned short langid,
               unsigned char index, void *buf, int size)
{
    int i;
    int result;

    rt_memset(buf, 0, size);    /* Make sure we parse really received data */

    for (i = 0; i < 3; ++i)
    {
        /* retry on length 0 or error; some devices are flakey */
        result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
                USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
                (USB_DT_STRING << 8) + index, langid, buf, size,
                USB_CTRL_GET_TIMEOUT);
        if (result <= 0 && result != -ETIMEDOUT)
            continue;
        if (result > 1 && ((rt_uint8_t *)buf)[1] != USB_DT_STRING)
        {
            result = -ENODATA;
            continue;
        }
        break;
    }
    return result;
}

static void usb_try_string_workarounds(unsigned char *buf, int *length)
{
	int newlength, oldlength = *length;

	for (newlength = 2; newlength + 1 < oldlength; newlength += 2)
		if (buf[newlength] < 0x20 || buf[newlength] > 0x7e || buf[newlength + 1])
			break;

	if (newlength > 2) {
		buf[0] = newlength;
		*length = newlength;
	}
}

static int usb_string_sub(struct usb_device *dev, unsigned int langid,
			  unsigned int index, unsigned char *buf)
{
	int rc;

	/* Try to read the string descriptor by asking for the maximum
	 * possible number of bytes */
	if (dev->quirks & USB_QUIRK_STRING_FETCH_255)
		rc = -EIO;
	else
		rc = usb_get_string(dev, langid, index, buf, 255);

	/* If that failed try to read the descriptor length, then
	 * ask for just that many bytes */
	if (rc < 2) {
		rc = usb_get_string(dev, langid, index, buf, 2);
		if (rc == 2)
			rc = usb_get_string(dev, langid, index, buf, buf[0]);
	}

	if (rc >= 2) {
		if (!buf[0] && !buf[1])
			usb_try_string_workarounds(buf, &rc);

		/* There might be extra junk at the end of the descriptor */
		if (buf[0] < rc)
			rc = buf[0];

		rc = rc - (rc & 1); /* force a multiple of two */
	}

	if (rc < 2)
		rc = (rc < 0 ? rc : -EINVAL);

	return rc;
}

static int usb_get_langid(struct usb_device *dev, unsigned char *tbuf)
{
	int err;

	if (dev->have_langid)
		return 0;

	if (dev->string_langid < 0)
		return -EPIPE;

	err = usb_string_sub(dev, 0, 0, tbuf);

	/* If the string was reported but is malformed, default to english
	 * (0x0409) */
	if (err == -ENODATA || (err > 0 && err < 4)) {
		dev->string_langid = 0x0409;
		dev->have_langid = 1;
		rt_kprintf("language id specifier not provided by device, defaulting to English\n");
		return 0;
	}

	/* In case of all other errors, we assume the device is not able to
	 * deal with strings at all. Set string_langid to -1 in order to
	 * prevent any string to be retrieved from the device */
	if (err < 0) {
		rt_kprintf("string descriptor 0 read error: %d\n",
					err);
		dev->string_langid = -1;
		return -EPIPE;
	}

	/* always use the first langid listed */
	dev->string_langid = tbuf[2] | (tbuf[3] << 8);
	dev->have_langid = 1;
	rt_kprintf("default language 0x%04x\n",
				dev->string_langid);
	return 0;
}

int utf16s_to_utf8s(unsigned char *pwcs, int inlen, char *buf, int maxout)
{
    int i, size = 0;
    unsigned char u;

    for (i = 0; i < inlen; i++)
    {
        u = pwcs[i];
        if (!u)
            continue;
        if (size >= maxout)
            break;
        buf[size] = u;
        size ++;
    }
    return size;
}
/**
 * usb_string - returns UTF-8 version of a string descriptor
 * @dev: the device whose string descriptor is being retrieved
 * @index: the number of the descriptor
 * @buf: where to put the string
 * @size: how big is "buf"?
 * Context: !in_interrupt ()
 *
 * This converts the UTF-16LE encoded strings returned by devices, from
 * usb_get_string_descriptor(), to null-terminated UTF-8 encoded ones
 * that are more usable in most kernel contexts.  Note that this function
 * chooses strings in the first language supported by the device.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Return: length of the string (>= 0) or usb_control_msg status (< 0).
 */
int usb_string(struct usb_device *dev, int index, char *buf, size_t size)
{
	unsigned char *tbuf;
	int err;

	if (dev->state == USB_STATE_SUSPENDED)
		return -EHOSTUNREACH;
	if (size <= 0 || !buf || !index)
		return -EINVAL;
	buf[0] = 0;
	tbuf = rt_calloc(256, 1);
	if (!tbuf)
		return -ENOMEM;

	err = usb_get_langid(dev, tbuf);
	if (err < 0)
		goto errout;

	err = usb_string_sub(dev, dev->string_langid, index, tbuf);
	if (err < 0)
		goto errout;

	size--;		/* leave room for trailing NULL char in output buffer */
	err = utf16s_to_utf8s(&tbuf[2], err - 2, buf, size);

	buf[err] = 0;

	if (tbuf[1] != USB_DT_STRING)
		rt_kprintf("wrong descriptor type %02x for string %d (\"%s\")\n",
			tbuf[1], index, buf);

 errout:
	rt_free(tbuf);
	return err;
}

/*
 * usb_get_device_descriptor - (re)reads the device descriptor (usbcore)
 * @dev: the device whose device descriptor is being updated
 * @size: how much of the descriptor to read
 * Context: !in_interrupt ()
 *
 * Updates the copy of the device descriptor stored in the device structure,
 * which dedicates space for this purpose.
 *
 * Not exported, only for use by the core.  If drivers really want to read
 * the device descriptor directly, they can call usb_get_descriptor() with
 * type = USB_DT_DEVICE and index = 0.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_device_descriptor(struct usb_device *dev, unsigned int size)
{
    struct usb_device_descriptor *desc;
    int ret;

    if (size > sizeof(*desc))
        return -EINVAL;
    /* desc = rt_malloc(sizeof(*desc)); */
    desc = usb_dma_buffer_alloc(sizeof(*desc), dev->bus);
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc usb_device_descriptor :%d size:%d\n",g_mem_debug++,sizeof(*desc))); */
    if (!desc)
        return -ENOMEM;

    ret = usb_get_descriptor(dev, USB_DT_DEVICE, 0, desc, size);
    if (ret >= 0)
        rt_memcpy(&dev->descriptor, desc, size);

    /* RT_DEBUG_LOG(rt_debug_usb,("rt_free message desc:%d\n",--g_mem_debug)); */
    /* rt_free(desc); */
    usb_dma_buffer_free(desc, dev->bus);
    return ret;
}

/**
 * usb_get_status - issues a GET_STATUS call
 * @dev: the device whose status is being checked
 * @type: USB_RECIP_*; for device, interface, or endpoint
 * @target: zero (for device), else interface or endpoint number
 * @data: pointer to two bytes of bitmap data
 * Context: !in_interrupt ()
 *
 * Returns device, interface, or endpoint status.  Normally only of
 * interest to see if the device is self powered, or has enabled the
 * remote wakeup facility; or whether a bulk or interrupt endpoint
 * is halted ("stalled").
 *
 * Bits in these status bitmaps are set using the SET_FEATURE request,
 * and cleared using the CLEAR_FEATURE request.  The usb_clear_halt()
 * function should be used to clear halt ("stall") status.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_status(struct usb_device *dev, int type, int target, void *data)
{
    int ret;
    rt_uint16_t *status = usb_dma_buffer_alloc(sizeof(*status), dev->bus);
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc status :%d size:%d\n",g_mem_debug++,sizeof(*status))); */

    if (!status)
        return -ENOMEM;

    ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
        USB_REQ_GET_STATUS, USB_DIR_IN | type, 0, target, status,
        sizeof(*status), USB_CTRL_GET_TIMEOUT);

    *(rt_uint16_t *)data = *status;
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_free message status:%d\n",--g_mem_debug)); */
    usb_dma_buffer_free(status, dev->bus);
    return ret;
}

/**
 * usb_clear_halt - tells device to clear endpoint halt/stall condition
 * @dev: device whose endpoint is halted
 * @pipe: endpoint "pipe" being cleared
 * Context: !in_interrupt ()
 *
 * This is used to clear halt conditions for bulk and interrupt endpoints,
 * as reported by URB completion status.  Endpoints that are halted are
 * sometimes referred to as being "stalled".  Such endpoints are unable
 * to transmit or receive data until the halt status is cleared.  Any URBs
 * queued for such an endpoint should normally be unlinked by the driver
 * before clearing the halt condition, as described in sections 5.7.5
 * and 5.8.5 of the USB 2.0 spec.
 *
 * Note that control and isochronous endpoints don't halt, although control
 * endpoints report "protocol stall" (for unsupported requests) using the
 * same status code used to report a true stall.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns zero on success, or else the status code returned by the
 * underlying usb_control_msg() call.
 */
int usb_clear_halt(struct usb_device *dev, int pipe)
{
    int result;
    int endp = usb_pipeendpoint(pipe);

    if (usb_pipein(pipe))
        endp |= USB_DIR_IN;

    /* we don't care if it wasn't halted first. in fact some devices
     * (like some ibmcam model 1 units) seem to expect hosts to make
     * this request for iso endpoints, which can't halt!
     */
    result = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
        USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
        USB_ENDPOINT_HALT, endp, NULL, 0,
        USB_CTRL_SET_TIMEOUT);

    /* don't un-halt or force to DATA0 except on success */
    if (result < 0)
        return result;

    /* NOTE:  seems like Microsoft and Apple don't bother verifying
     * the clear "took", so some devices could lock up if you check...
     * such as the Hagiwara FlashGate DUAL.  So we won't bother.
     *
     * NOTE:  make sure the logic here doesn't diverge much from
     * the copy in usb-storage, for as long as we need two copies.
     */

    usb_reset_endpoint(dev, endp);

    return 0;
}


/**
 * usb_disable_endpoint -- Disable an endpoint by address
 * @dev: the device whose endpoint is being disabled
 * @epaddr: the endpoint's address.  Endpoint number for output,
 *    endpoint number + USB_DIR_IN for input
 * @reset_hardware: flag to erase any endpoint state stored in the
 *    controller hardware
 *
 * Disables the endpoint for URB submission and nukes all pending URBs.
 * If @reset_hardware is set then also deallocates hcd/hardware state
 * for the endpoint.
 */
void usb_disable_endpoint(struct usb_device *dev, unsigned int epaddr,
        rt_bool_t reset_hardware)
{
    unsigned int epnum = epaddr & USB_ENDPOINT_NUMBER_MASK;
    struct usb_host_endpoint *ep;

    if (!dev)
        return;

    if (usb_endpoint_out(epaddr))
    {
        ep = dev->ep_out[epnum];
        if (reset_hardware)
            dev->ep_out[epnum] = NULL;
    } else
    {
        ep = dev->ep_in[epnum];
        if (reset_hardware)
            dev->ep_in[epnum] = NULL;
    }
    if (ep)
    {
        ep->enabled = 0;
        usb_hcd_flush_endpoint(dev, ep);
        if (reset_hardware)
            usb_hcd_disable_endpoint(dev, ep);
    }
}

/**
 * usb_reset_endpoint - Reset an endpoint's state.
 * @dev: the device whose endpoint is to be reset
 * @epaddr: the endpoint's address.  Endpoint number for output,
 *    endpoint number + USB_DIR_IN for input
 *
 * Resets any host-side endpoint state such as the toggle bit,
 * sequence number or current window.
 */
void usb_reset_endpoint(struct usb_device *dev, unsigned int epaddr)
{
    unsigned int epnum = epaddr & USB_ENDPOINT_NUMBER_MASK;
    struct usb_host_endpoint *ep;

    if (usb_endpoint_out(epaddr))
        ep = dev->ep_out[epnum];
    else
        ep = dev->ep_in[epnum];
    if (ep)
        usb_hcd_reset_endpoint(dev, ep);
}


/**
 * usb_disable_interface -- Disable all endpoints for an interface
 * @dev: the device whose interface is being disabled
 * @intf: pointer to the interface descriptor
 * @reset_hardware: flag to erase any endpoint state stored in the
 *    controller hardware
 *
 * Disables all the endpoints for the interface's current altsetting.
 */
void usb_disable_interface(struct usb_device *dev, struct usb_interface *intf,
        rt_bool_t reset_hardware)
{
    struct usb_host_interface *alt = intf->cur_altsetting;
    int i;

    for (i = 0; i < alt->desc.bNumEndpoints; ++i)
    {
        usb_disable_endpoint(dev,
                alt->endpoint[i].desc.bEndpointAddress,
                reset_hardware);
    }
}

void usb_release_interface(struct usb_interface *intf)
{
    struct usb_interface_cache *intfc =
            altsetting_to_usb_interface_cache(intf->altsetting);

    usb_release_interface_cache(intfc);
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free message intf:%d,addr:%x\n", --g_mem_debug, intf));
    rt_free(intf);
}

/**
 * usb_disable_device - Disable all the endpoints for a USB device
 * @dev: the device whose endpoints are being disabled
 * @skip_ep0: 0 to disable endpoint 0, 1 to skip it.
 *
 * Disables all the device's endpoints, potentially including endpoint 0.
 * Deallocates hcd/hardware state for the endpoints (nuking all or most
 * pending urbs) and usbcore state for the interfaces, so that usbcore
 * must usb_set_configuration() before any interfaces could be used.
 *
 * Must be called with hcd->bandwidth_mutex held.
 */
void usb_disable_device(struct usb_device *dev, int skip_ep0)
{
    int i;
    struct usb_hcd *hcd = bus_to_hcd(dev->bus);

    /* getting rid of interfaces will disconnect
     * any drivers bound to them (a key side effect)
     */
    if (dev->actconfig)
    {
        /*
         * FIXME: In order to avoid self-deadlock involving the
         * bandwidth_mutex, we have to mark all the interfaces
         * before unregistering any of them.
         */
        for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++)
            dev->actconfig->interface[i]->unregistering = 1;

        for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++)
        {
            struct usb_interface    *interface;

            /* remove this interface if it has been registered */
            interface = dev->actconfig->interface[i];
/* if (!interface->dev) */
/* continue; */
            RT_DEBUG_LOG(rt_debug_usb, ("unregistering interface\n"));
            if (interface->driver)
                interface->driver->disconnect(interface);
            usb_release_interface(interface);
            /* rt_device_unregister(&(interface->dev)); */

        }

        /* Now that the interfaces are unbound, nobody should
         * try to access them.
         */
        for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++)
        {
            dev->actconfig->interface[i] = NULL;
        }
        dev->actconfig = NULL;
        if (dev->state == USB_STATE_CONFIGURED)
            usb_set_device_state(dev, USB_STATE_ADDRESS);
    }

    RT_DEBUG_LOG(rt_debug_usb, ("%s nuking %s URBs\n", __func__,
        skip_ep0 ? "non-ep0" : "all"));
    if (hcd->driver->check_bandwidth)
    {
        /* First pass: Cancel URBs, leave endpoint pointers intact. */
        for (i = skip_ep0; i < 16; ++i)
        {
            usb_disable_endpoint(dev, i, RT_FALSE);
            usb_disable_endpoint(dev, i + USB_DIR_IN, RT_FALSE);
        }
        /* Remove endpoints from the host controller internal state */
        usb_hcd_alloc_bandwidth(dev, NULL, NULL, NULL);
        /* Second pass: remove endpoint pointers */
    }
    for (i = skip_ep0; i < 16; ++i)
    {
        usb_disable_endpoint(dev, i, RT_TRUE);
        usb_disable_endpoint(dev, i + USB_DIR_IN, RT_TRUE);
    }
}

/**
 * usb_enable_endpoint - Enable an endpoint for USB communications
 * @dev: the device whose interface is being enabled
 * @ep: the endpoint
 * @reset_ep: flag to reset the endpoint state
 *
 * Resets the endpoint state if asked, and sets dev->ep_{in,out} pointers.
 * For control endpoints, both the input and output sides are handled.
 */
void usb_enable_endpoint(struct usb_device *dev, struct usb_host_endpoint *ep,
        rt_bool_t reset_ep)
{
    int epnum = usb_endpoint_num(&ep->desc);
    int is_out = usb_endpoint_dir_out(&ep->desc);
    int is_control = usb_endpoint_xfer_control(&ep->desc);

    if (reset_ep)
        usb_hcd_reset_endpoint(dev, ep);
    if (is_out || is_control)
        dev->ep_out[epnum] = ep;
    if (!is_out || is_control)
        dev->ep_in[epnum] = ep;
    ep->enabled = 1;
}

/**
 * usb_enable_interface - Enable all the endpoints for an interface
 * @dev: the device whose interface is being enabled
 * @intf: pointer to the interface descriptor
 * @reset_eps: flag to reset the endpoints' state
 *
 * Enables all the endpoints for the interface's current altsetting.
 */
void usb_enable_interface(struct usb_device *dev,
        struct usb_interface *intf, rt_bool_t reset_eps)
{
    struct usb_host_interface *alt = intf->cur_altsetting;
    int i;

    for (i = 0; i < alt->desc.bNumEndpoints; ++i)
        usb_enable_endpoint(dev, &alt->endpoint[i], reset_eps);
}

/**
 * usb_set_interface - Makes a particular alternate setting be current
 * @dev: the device whose interface is being updated
 * @interface: the interface being updated
 * @alternate: the setting being chosen.
 * Context: !in_interrupt ()
 *
 * This is used to enable data transfers on interfaces that may not
 * be enabled by default.  Not all devices support such configurability.
 * Only the driver bound to an interface may change its setting.
 *
 * Within any given configuration, each interface may have several
 * alternative settings.  These are often used to control levels of
 * bandwidth consumption.  For example, the default setting for a high
 * speed interrupt endpoint may not send more than 64 bytes per microframe,
 * while interrupt transfers of up to 3KBytes per microframe are legal.
 * Also, isochronous endpoints may never be part of an
 * interface's default setting.  To access such bandwidth, alternate
 * interface settings must be made current.
 *
 * Note that in the Linux USB subsystem, bandwidth associated with
 * an endpoint in a given alternate setting is not reserved until an URB
 * is submitted that needs that bandwidth.  Some other operating systems
 * allocate bandwidth early, when a configuration is chosen.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 * Also, drivers must not change altsettings while urbs are scheduled for
 * endpoints in that interface; all such urbs must first be completed
 * (perhaps forced by unlinking).
 *
 * Returns zero on success, or else the status code returned by the
 * underlying usb_control_msg() call.
 */
int usb_set_interface(struct usb_device *dev, int interface, int alternate)
{
    struct usb_interface *iface;
    struct usb_host_interface *alt;
    struct usb_hcd *hcd = bus_to_hcd(dev->bus);
    int ret;
    int manual = 0;
    unsigned int epaddr;
    unsigned int pipe;

    if (dev->state == USB_STATE_SUSPENDED)
        return -EHOSTUNREACH;

    iface = usb_ifnum_to_if(dev, interface);
    if (!iface)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("selecting invalid interface %d\n",
            interface));
        return -EINVAL;
    }
    if (iface->unregistering)
        return -ENODEV;

    alt = usb_altnum_to_altsetting(iface, alternate);
    if (!alt)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("selecting invalid altsetting %d\n",
             alternate));
        return -EINVAL;
    }

    /* Make sure we have enough bandwidth for this alternate interface.
     * Remove the current alt setting and add the new alt setting.
     */
    rt_mutex_take(hcd->bandwidth_mutex, RT_WAITING_FOREVER);
    ret = usb_hcd_alloc_bandwidth(dev, NULL, iface->cur_altsetting, alt);
    if (ret < 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("Not enough bandwidth for altsetting %d\n",
                alternate));
        rt_mutex_release(hcd->bandwidth_mutex);
        return ret;
    }

    if (dev->quirks & USB_QUIRK_NO_SET_INTF)
        ret = -EPIPE;
    else
        ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                   USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
                   alternate, interface, NULL, 0, 5000);

    /* 9.4.10 says devices don't need this and are free to STALL the
     * request if the interface only has one alternate setting.
     */
    if (ret == -EPIPE && iface->num_altsetting == 1)
    {
        RT_DEBUG_LOG(rt_debug_usb,
            ("manual set_interface for iface %d, alt %d\n",
            interface, alternate));
        manual = 1;
    } else if (ret < 0)
    {
        /* Re-instate the old alt setting */
        usb_hcd_alloc_bandwidth(dev, NULL, alt, iface->cur_altsetting);
        rt_mutex_release(hcd->bandwidth_mutex);
        return ret;
    }
    rt_mutex_release(hcd->bandwidth_mutex);

    /* FIXME drivers shouldn't need to replicate/bugfix the logic here
     * when they implement async or easily-killable versions of this or
     * other "should-be-internal" functions (like clear_halt).
     * should hcd+usbcore postprocess control requests?
     */

    usb_disable_interface(dev, iface, RT_TRUE);

    iface->cur_altsetting = alt;

    /* If the interface only has one altsetting and the device didn't
     * accept the request, we attempt to carry out the equivalent action
     * by manually clearing the HALT feature for each endpoint in the
     * new altsetting.
     */
    if (manual)
    {
        int i;

        for (i = 0; i < alt->desc.bNumEndpoints; i++)
        {
            epaddr = alt->endpoint[i].desc.bEndpointAddress;
            pipe = __create_pipe(dev,
                    USB_ENDPOINT_NUMBER_MASK & epaddr) |
                    (usb_endpoint_out(epaddr) ?
                    USB_DIR_OUT : USB_DIR_IN);

            usb_clear_halt(dev, pipe);
        }
    }

    /* 9.1.1.5: reset toggles for all endpoints in the new altsetting
     *
     * Note:
     * Despite EP0 is always present in all interfaces/AS, the list of
     * endpoints from the descriptor does not contain EP0. Due to its
     * omnipresence one might expect EP0 being considered "affected" by
     * any SetInterface request and hence assume toggles need to be reset.
     * However, EP0 toggles are re-synced for every individual transfer
     * during the SETUP stage - hence EP0 toggles are "don't care" here.
     * (Likewise, EP0 never "halts" on well designed devices.)
     */
    usb_enable_interface(dev, iface, RT_TRUE);
    return 0;
}



static struct usb_interface_assoc_descriptor *find_iad(struct usb_device *dev,
                        struct usb_host_config *config,
                        rt_uint8_t inum)
{
    struct usb_interface_assoc_descriptor *retval = RT_NULL;
    struct usb_interface_assoc_descriptor *intf_assoc;
    int first_intf;
    int last_intf;
    int i;

    for (i = 0; (i < USB_MAXIADS && config->intf_assoc[i]); i++)
    {
        intf_assoc = config->intf_assoc[i];
        if (intf_assoc->bInterfaceCount == 0)
            continue;

        first_intf = intf_assoc->bFirstInterface;
        last_intf = first_intf + (intf_assoc->bInterfaceCount - 1);
        if (inum >= first_intf && inum <= last_intf)
        {
            if (!retval)
                retval = intf_assoc;
            else
                RT_DEBUG_LOG(rt_debug_usb, ("Interface #%d referenced by multiple IADs\n",
                    inum));
        }
    }

    return retval;
}



/*
 * usb_set_configuration - Makes a particular device setting be current
 * @dev: the device whose configuration is being updated
 * @configuration: the configuration being chosen.
 * Context: !in_interrupt(), caller owns the device lock
 *
 * This is used to enable non-default device modes.  Not all devices
 * use this kind of configurability; many devices only have one
 * configuration.
 *
 * @configuration is the value of the configuration to be installed.
 * According to the USB spec (e.g. section 9.1.1.5), configuration values
 * must be non-zero; a value of zero indicates that the device in
 * unconfigured.  However some devices erroneously use 0 as one of their
 * configuration values.  To help manage such devices, this routine will
 * accept @configuration = -1 as indicating the device should be put in
 * an unconfigured state.
 *
 * USB device configurations may affect Linux interoperability,
 * power consumption and the functionality available.  For example,
 * the default configuration is limited to using 100mA of bus power,
 * so that when certain device functionality requires more power,
 * and the device is bus powered, that functionality should be in some
 * non-default device configuration.  Other device modes may also be
 * reflected as configuration options, such as whether two ISDN
 * channels are available independently; and choosing between open
 * standard device protocols (like CDC) or proprietary ones.
 *
 * Note that a non-authorized device (dev->authorized == 0) will only
 * be put in unconfigured mode.
 *
 * Note that USB has an additional level of device configurability,
 * associated with interfaces.  That configurability is accessed using
 * usb_set_interface().
 *
 * This call is synchronous. The calling context must be able to sleep,
 * must own the device lock, and must not hold the driver model's USB
 * bus mutex; usb interface driver probe() methods cannot use this routine.
 *
 * Returns zero on success, or else the status code returned by the
 * underlying call that failed.  On successful completion, each interface
 * in the original device configuration has been destroyed, and each one
 * in the new configuration has been probed by all relevant usb device
 * drivers currently known to the kernel.
 */
int usb_set_configuration(struct usb_device *dev, int configuration)
{
    int i, ret;
    struct usb_host_config *cp = RT_NULL;
    struct usb_interface **new_interfaces = RT_NULL;
    struct usb_hcd *hcd = bus_to_hcd(dev->bus);
    int n, nintf;

    if (configuration == -1)
        configuration = 0;
    else
    {
        for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
        {
            if (dev->config[i].desc.bConfigurationValue ==
                    configuration) {
                cp = &dev->config[i];
                break;
            }
        }
    }
    if ((!cp && configuration != 0))
        return -EINVAL;

    /* The USB spec says configuration 0 means unconfigured.
     * But if a device includes a configuration numbered 0,
     * we will accept it as a correctly configured state.
     * Use -1 if you really want to unconfigure the device.
     */
    if (cp && configuration == 0)
        RT_DEBUG_LOG(rt_debug_usb, ("config 0 descriptor??\n"));

    /* Allocate memory for new interfaces before doing anything else,
     * so that if we run out then nothing will have changed. */
    n = nintf = 0;
    if (cp)
    {
        nintf = cp->desc.bNumInterfaces;
        new_interfaces = rt_malloc(nintf * sizeof(*new_interfaces));
        RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc new_interfaces point:%d size:%d,addr:%x\n", g_mem_debug++, sizeof(*new_interfaces), new_interfaces));
        if (!new_interfaces)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("Out of memory\n"));
            return -ENOMEM;
        }
        rt_memset(new_interfaces, 0, nintf * sizeof(*new_interfaces));

        for (; n < nintf; ++n)
        {
            new_interfaces[n] = rt_malloc(
                    sizeof(struct usb_interface));
            RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc usb_interface:%d size:%d,addr:%x\n", g_mem_debug++, sizeof(struct usb_interface), new_interfaces[n]));
            if (!new_interfaces[n])
            {
                RT_DEBUG_LOG(rt_debug_usb, ("Out of memory\n"));
                ret = -ENOMEM;
free_interfaces:
                while (--n >= 0)
                {
                    rt_free(new_interfaces[n]);
                    RT_DEBUG_LOG(rt_debug_usb, ("rt_free message new_interfaces n:%d,addr:%x\n", --g_mem_debug, new_interfaces[n]));
                }
                RT_DEBUG_LOG(rt_debug_usb, ("rt_free message new_interfaces:%d\n", --g_mem_debug, new_interfaces));
                rt_free(new_interfaces);
                return ret;
            }
        rt_memset(new_interfaces[n], 0, sizeof(struct usb_interface));
        }

        i = dev->bus_mA - cp->desc.bMaxPower * 2;
        if (i < 0)
            RT_DEBUG_LOG(rt_debug_usb,("new config #%d exceeds power limit by %dmA\n",
                    configuration, -i));
    }

    /* if it's already configured, clear out old state first.
     * getting rid of old interfaces means unbinding their drivers.
     */
    rt_mutex_take(hcd->bandwidth_mutex, RT_WAITING_FOREVER);
    if (dev->state != USB_STATE_ADDRESS)
        usb_disable_device(dev, 1);    /* Skip ep0 */

    /* Get rid of pending async Set-Config requests for this device */
    cancel_async_set_config(dev);

    /* Make sure we have bandwidth (and available HCD resources) for this
     * configuration.  Remove endpoints from the schedule if we're dropping
     * this configuration to set configuration 0.  After this point, the
     * host controller will not allow submissions to dropped endpoints.  If
     * this call fails, the device state is unchanged.
     */
    ret = usb_hcd_alloc_bandwidth(dev, cp, RT_NULL, RT_NULL);
    if (ret < 0)
    {
        rt_mutex_release(hcd->bandwidth_mutex);
        goto free_interfaces;
    }

    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                  USB_REQ_SET_CONFIGURATION, 0, configuration, 0,
                  RT_NULL, 0, USB_CTRL_SET_TIMEOUT);
    if (ret < 0)
    {
        /* All the old state is gone, so what else can we do?
         * The device is probably useless now anyway.
         */
        cp = RT_NULL;
    }

    dev->actconfig = cp;
    if (!cp)
    {
        usb_set_device_state(dev, USB_STATE_ADDRESS);
        usb_hcd_alloc_bandwidth(dev, RT_NULL, RT_NULL, RT_NULL);
        rt_mutex_release(hcd->bandwidth_mutex);
        goto free_interfaces;
    }
    rt_mutex_release(hcd->bandwidth_mutex);
    usb_set_device_state(dev, USB_STATE_CONFIGURED);

    /* Initialize the new interface structures and the
     * hc/hcd/usbcore interface/endpoint state.
     */
    for (i = 0; i < nintf; ++i)
    {
        struct usb_interface_cache *intfc;
        struct usb_interface *intf;
        struct usb_host_interface *alt;

        cp->interface[i] = intf = new_interfaces[i];
        intfc = cp->intf_cache[i];
        intf->altsetting = intfc->altsetting;
        intf->num_altsetting = intfc->num_altsetting;
        intf->intf_assoc = find_iad(dev, cp, i);
        intfc->ref++;

        alt = usb_altnum_to_altsetting(intf, 0);

        /* No altsetting 0?  We'll assume the first altsetting.
         * We could use a GetInterface call, but if a device is
         * so non-compliant that it doesn't have altsetting 0
         * then I wouldn't trust its reply anyway.
         */
        if (!alt)
            alt = &intf->altsetting[0];

        intf->cur_altsetting = alt;
        usb_enable_interface(dev, intf, RT_TRUE);
        intf->usb_dev = dev;
        intf->dev.type = RT_Device_Class_USBDevice;
/* intf->dev=rt_malloc(sizeof(struct rt_device)); */
/* if (!(intf->dev)) { */
/* RT_DEBUG_LOG(rt_debug_usb, ("Out of memory\n")); */
/* return -ENOMEM; */
/* } */
/* rt_memset(intf->dev,0,sizeof(struct rt_device)); */

        rt_dev_set_name(&(intf->dev), "%d-%s:%d.%d",
            dev->bus->busnum, dev->devpath,
            configuration, alt->desc.bInterfaceNumber);
    }
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free message new_interfaces:%d addr:0x%x\n", --g_mem_debug, new_interfaces));
    rt_free(new_interfaces);
#if 0
    if (cp->string == NULL &&
            !(dev->quirks & USB_QUIRK_CONFIG_INTF_STRINGS))
        cp->string = usb_cache_string(dev, cp->desc.iConfiguration);
#endif

    /* Now that all the interfaces are set up, register them
     * to trigger binding of drivers to interfaces.  probe()
     * routines may install different altsettings and may
     * claim() any interfaces not yet bound.  Many class drivers
     * need that: CDC, audio, video, etc.
     */
    for (i = 0; i < nintf; ++i)
    {
        struct usb_interface *intf = cp->interface[i];

        RT_DEBUG_LOG(rt_debug_usb,
            ("adding %s (config #%d, interface %d)\n",
            intf->dev.parent.name, configuration,
            intf->cur_altsetting->desc.bInterfaceNumber));
        ret = usb_device_add(intf);
        if (ret != 0)
        {
            rt_kprintf("device_add fail(%s) --> %d\n",
                intf->dev.parent.name, ret);
            /* return ret; */
        }

        /* ret = rt_device_register(&(intf->dev),intf->dev.parent.name,RT_DEVICE_FLAG_DMA_RX|RT_DEVICE_FLAG_DMA_TX); */
        /* if (ret != 0) { */
        /* rt_kprintf("device_register fail(%s) --> %d\n", */
        /* intf->dev.parent.name, ret); */
        /* return ret; */
        /* } */
    }
    return 0;
}
