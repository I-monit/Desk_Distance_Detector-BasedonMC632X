/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-03     tangyh    the first version
 *
 */

#include "sadc.h"
#include "fh_sadc.h"
#include "fh_clock.h"
#include "board_info.h"
#include "fh_def.h"
#include <rtdef.h>
#include <rtthread.h>
#include <rthw.h>

//for fpga
#define SADC_CLK 1500000

#define wrap_readl(wrap, offset) \
    GET_REG(((struct wrap_sadc_obj *)wrap)->regs + offset)
#define wrap_writel(wrap, offset, val) \
    SET_REG(((struct wrap_sadc_obj *)wrap)->regs + offset, val)

#define MAX_CHANNEL_NO                  (8)

#define SADC_CTRL0			0x0
#define SADC_CTRL1			0x4
#define SADC_CTRL2			0x8
#define SADC_CONFIG0		0x10
#define SADC_CONFIG1		0x14
#define SADC_CONFIG2		0x18
#define SADC_CONFIG3		0x1c
#define SADC_CONFIG4		0x20
#define SADC_CONFIG5		0x24
#define SADC_CONFIG6		0x28
#define SADC_CONFIG7		0x2c
#define SADC_INT_EN			0x40
#define SADC_INT_STA		0x44
#define SADC_DOUT0			0x50
#define SADC_DOUT1			0x54
#define SADC_DOUT2			0x58
#define SADC_DOUT3			0x5c
#define SADC_BUTTON_DOUT0	0x60
#define SADC_BUTTON_DOUT1	0x64
#define SADC_BUTTON_DOUT2	0x68
#define SADC_BUTTON_DOUT3	0x6c
#define SADC_DEBUG0			0x100
#define SADC_DEBUG1			0x104
#define SADC_DEBUG2			0x108
#define SADC_DEBUG3			0x10c
#define SADC_DEBUG4			0x110
#define SADC_DEBUG5			0x114
#define SADC_DEBUG6			0x118
#define SADC_ECO			0x120

typedef unsigned int u32;

struct fh_sadc {
	struct wrap_sadc_obj *sadc;
	struct rt_mutex lock;
    struct rt_semaphore completion;
    rt_device_t rt_dev;
	rt_int32_t active_chan_no;
	rt_uint32_t chan_data[MAX_CHANNEL_NO];
};

int fh_sadc_enable(struct wrap_sadc_obj *sadc)
{
	u32 control_reg;

	control_reg = wrap_readl(sadc, SADC_CTRL0);
	control_reg |= 1 << 0;
	wrap_writel(sadc, SADC_CTRL0, control_reg);
	return 0;
}


int fh_sadc_disable(struct wrap_sadc_obj *sadc)
{
	u32 control_reg;

	control_reg = wrap_readl(sadc, SADC_CTRL0);
	control_reg &= ~(1 << 0);
	wrap_writel(sadc, SADC_CTRL0, control_reg);
	return 0;
}


int fh_sadc_update(struct wrap_sadc_obj *sadc)
{
	u32 reg;

	reg = wrap_readl(sadc, SADC_CTRL1);
	reg |= 1;
	wrap_writel(sadc, SADC_CTRL1, reg);
	return 0;
}

int fh_sadc_lowpwr_enable(struct wrap_sadc_obj *sadc)
{
	u32 reg;

	reg = wrap_readl(sadc, SADC_CTRL2);
	reg |= (1 << 0);
	wrap_writel(sadc, SADC_CTRL2, reg);
	return 0;
}

u32 fh_sadc_lowpwr_disable(struct wrap_sadc_obj *sadc)
{
	u32 reg;

	reg = wrap_readl(sadc, SADC_CTRL2);
	reg &= (~(1 << 0));
	wrap_writel(sadc, SADC_CTRL2, reg);
	return 0;
}

int fh_sadc_continue_enable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG1);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG1, reg);
	return 0;
}

int fh_sadc_continue_disable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG1);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG1, reg);
	return 0;
}

int fh_sadc_single_enable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG0);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG0, reg);
	return 0;
}

int fh_sadc_single_disable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG0);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG0, reg);
	return 0;
}


int fh_sadc_eq_enable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG2);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG2, reg);
	return 0;
}


int fh_sadc_eq_disable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG2);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG2, reg);
	return 0;
}


int fh_sadc_hit_enable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG3);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG3, reg);
	return 0;
}


int fh_sadc_hit_disable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel >= MAX_CHANNEL_NO) {
		rt_kprintf("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG3);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG3, reg);
	return 0;
}


int fh_sadc_scan_time(struct wrap_sadc_obj *sadc, u32 scan_time)
{
	u32 value;

	value = scan_time*(sadc->frequency/1000);
	wrap_writel(sadc, SADC_CONFIG4, value);
	return 0;
}


int fh_sadc_glitch_time(struct wrap_sadc_obj *sadc, u32 glitch_time)
{
	u32 value;

	value = glitch_time*(sadc->frequency/1000);
	wrap_writel(sadc, SADC_CONFIG5, value);
	return 0;
}


int fh_sadc_noise_range(struct wrap_sadc_obj *sadc, u32 noise_range)
{
	u32 reg;
	u32 value;

	value = (noise_range*sadc->max_value)/sadc->ref_vol;
	value &= 0xfff;
	if(value >= 0xff) {
		rt_kprintf("sadc noise range error %d\n", noise_range);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG6);
	reg &= (~(0xff << 0));
	reg |= (value << 0);
	wrap_writel(sadc, SADC_CONFIG6, reg);
	return 0;
}


int fh_sadc_zero_value(struct wrap_sadc_obj *sadc, u32 zero_value)
{
	u32 reg;
	u32 value;

	value = (zero_value*sadc->max_value)/sadc->ref_vol;
	value &= 0xfff;

	reg = wrap_readl(sadc, SADC_CONFIG6);
	reg &= (~(0xfff << 16));
	reg |= (value << 16);
	wrap_writel(sadc, SADC_CONFIG6, reg);
	return 0;
}


int fh_sadc_active_bit(struct wrap_sadc_obj *sadc, u32 active_bit)
{
	u32 reg;

	if(active_bit > 0x12) {
		rt_kprintf("sadc active bit error %d\n", active_bit);
		return -1;
	}
	reg = 0xfff >> (12-active_bit);
	reg = reg << (12-active_bit);
	wrap_writel(sadc, SADC_CONFIG7, reg);
	return 0;
}


int fh_sadc_int_enable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel > MAX_CHANNEL_NO) {
		rt_kprintf("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}


int fh_sadc_int_disable(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel > MAX_CHANNEL_NO) {
		rt_kprintf("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}


int fh_sadc_press_int_en(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel > MAX_CHANNEL_NO) {
		rt_kprintf("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg |= (1 << (channel + 8));
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}


int fh_sadc_press_int_dis(struct wrap_sadc_obj *sadc, u32 channel)
{
	u32 reg;

	if(channel > MAX_CHANNEL_NO) {
		rt_kprintf("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg &= (~(1 << (channel + 8)));
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}


u32 fh_sadc_isr_read_data(struct fh_sadc *sadc_data, u32 channel,
		uint16 *buf) {

	u32 ret;
	struct wrap_sadc_obj *sadc = sadc_data->sadc;

	sadc_data->completion.value = 0;
	sadc_data->active_chan_no= -1;
	fh_sadc_eq_enable(sadc, channel);
	fh_sadc_active_bit(sadc, 12);
	fh_sadc_continue_enable(sadc, channel);
	fh_sadc_int_enable(sadc, channel);
	/*10ms*/
	fh_sadc_scan_time(sadc, 10);
	fh_sadc_enable(sadc);
	fh_sadc_update(sadc);
	ret = rt_sem_take(&sadc_data->completion, 500);
	if (ret != RT_EOK) {
		rt_kprintf("sadc timeout..\n");
		goto release;
	}
	if(channel == sadc_data->active_chan_no)
		*buf = sadc_data->chan_data[channel];
	else
		rt_kprintf("sadc channel :%d,return channel:%d error\n", channel,
		sadc_data->active_chan_no);
	release:
	fh_sadc_eq_disable(sadc, channel);
	fh_sadc_continue_disable(sadc, channel);
	fh_sadc_int_disable(sadc, channel);
	fh_sadc_update(sadc);
	return ret;
}

u32 fh_sadc_isr_read_single_data(struct fh_sadc *sadc_data, u32 channel,
		uint16 *buf) {
	u32 ret;
    struct wrap_sadc_obj *sadc = sadc_data->sadc;
	sadc_data->active_chan_no= -1;
	fh_sadc_active_bit(sadc, 12);
	fh_sadc_single_enable(sadc, channel);
	fh_sadc_int_enable(sadc, channel);
	/*10ms*/
	fh_sadc_scan_time(sadc, 10);
	fh_sadc_enable(sadc);
	fh_sadc_update(sadc);
	ret = rt_sem_take(&sadc_data->completion, 500);
	if (ret != RT_EOK) {
		rt_kprintf("sadc timeout..\n");
		goto release;
	}
	if(channel == sadc_data->active_chan_no)
		*buf = sadc_data->chan_data[channel];
	else
		rt_kprintf("sadc channel :%d,return channel:%d error\n", channel,
		sadc_data->active_chan_no);
	release:
	fh_sadc_single_disable(sadc, channel);
	fh_sadc_int_disable(sadc, channel);
	fh_sadc_update(sadc);
	return ret;
}
u32 fh_sadc_isr_read_hit_data(struct fh_sadc *sadc_data, u32 channel,
		uint16 *buf) {

    struct wrap_sadc_obj *sadc = sadc_data->sadc;
	sadc_data->active_chan_no= -1;
	fh_sadc_active_bit(sadc, 12);
	fh_sadc_continue_enable(sadc, channel);
	fh_sadc_hit_enable(sadc, channel);
	fh_sadc_press_int_en(sadc, channel);
	/*10ms*/
	fh_sadc_scan_time(sadc, 10);
	fh_sadc_enable(sadc);
	fh_sadc_update(sadc);
	do {
	rt_sem_take(&sadc_data->completion, 500);
	}while(channel != sadc_data->active_chan_no);

	*buf = (sadc_data->chan_data[channel]*sadc->ref_vol)/sadc->max_value;
	fh_sadc_continue_disable(sadc, channel);
	fh_sadc_hit_disable(sadc, channel);
	fh_sadc_press_int_dis(sadc, channel);
	fh_sadc_update(sadc);
	return 0;
}
static void fh_sadc_interrupt(int irq, void *param)
{

	u32 isr_status;
	u32 isr_active;
	u32 isr_en;
	u32 temp_data =  0;
	u32 channel = 0;
	u32 data = 0;
    struct fh_sadc *sadc_data = (struct fh_sadc *)param;
	struct wrap_sadc_obj *sadc = sadc_data->sadc;

	isr_status = wrap_readl(sadc, SADC_INT_STA);
	isr_en = wrap_readl(sadc, SADC_INT_EN);
	isr_active = isr_status & isr_en;
	if (isr_active & 0xff) {
		channel = __rt_ffs(isr_active);
		channel = channel - 1;
		switch (channel/2) {
		case 0:
			/*read channel 0 1*/
			temp_data = wrap_readl(sadc, SADC_DOUT0);
			break;
		case 1:
			/*read channel 2 3*/
			temp_data = wrap_readl(sadc, SADC_DOUT1);
			break;
		case 2:
			/*read channel 4 5*/
			temp_data = wrap_readl(sadc, SADC_DOUT2);
			break;
		case 3:
			/*read channel 6 7*/
			temp_data = wrap_readl(sadc, SADC_DOUT3);
			break;
		default:
			break;
		}
		rt_sem_release(&(sadc_data->completion));
	} else if (isr_active & 0xffff00) {
		if(isr_active & 0xff00)
			channel = __rt_ffs(isr_active >> 8);
		else
			channel = __rt_ffs(isr_active >> 16);
		channel = channel - 1;
		switch (channel/2) {
		case 0:
			/*read channel 0 1*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT0);
			break;
		case 1:
			/*read channel 2 3*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT1);
			break;
		case 2:
			/*read channel 4 5*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT2);
			break;
		case 3:
			/*read channel 6 7*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT3);
			break;
		default:
			break;
		}
		rt_sem_release(&(sadc_data->completion));
	}

	if (channel % 2)
		/*read high 16bit*/
		data = (temp_data >> 16) & 0xfff;
	else
		/*read low 16bit*/
		data = (temp_data & 0xfff);
	sadc_data->chan_data[channel] = data;
	sadc_data->active_chan_no= channel;
	wrap_writel(sadc, SADC_INT_STA, (~isr_status));
	return;
}

static rt_err_t fh_sadc_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t fh_sadc_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t fh_sadc_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t fh_sadc_ioctl(rt_device_t dev, int cmd, void *arg)
{
    rt_uint32_t ad_data;
    rt_uint16_t ad_raw_data;
    rt_int32_t  status;
    SADC_INFO *sadc_info;
	struct fh_sadc *sadc_data = (struct fh_sadc *)dev->user_data;
    struct wrap_sadc_obj *sadc = sadc_data->sadc;

    status = rt_mutex_take(&sadc_data->lock, RT_WAITING_FOREVER);
    if (status < 0)
    {
        rt_kprintf("[SADC] sadc ioctl busy!\n");
        return -RT_EBUSY;
    }

    switch (cmd)
    {
    case SADC_CMD_READ_RAW_DATA:
        sadc_info = (SADC_INFO *)arg;
        fh_sadc_isr_read_single_data(sadc_data, sadc_info->channel, &ad_raw_data);
        sadc_info->sadc_data = ad_raw_data;
        break;
    case SADC_CMD_READ_VOLT:
        sadc_info = (SADC_INFO *)arg;
        fh_sadc_isr_read_single_data(sadc_data, sadc_info->channel, &ad_raw_data);
        ad_data = ad_raw_data * sadc->ref_vol;
        ad_data /= sadc->max_value;
        sadc_info->sadc_data = ad_data;
        break;
    case SADC_CMD_READ_CONTINUE_DATA:
        sadc_info = (SADC_INFO *)arg;
        fh_sadc_isr_read_data(sadc_data, sadc_info->channel, &ad_raw_data);
        ad_data = ad_raw_data * sadc->ref_vol;
        ad_data /= sadc->max_value;
        sadc_info->sadc_data = ad_data;
        break;
    case IOCTL_GET_HIT_DATA:
        sadc_info = (SADC_INFO *)arg;
        fh_sadc_isr_read_hit_data(sadc_data, sadc_info->channel,\
			&ad_raw_data);
		ad_data = ad_raw_data * sadc->ref_vol;
        ad_data /= sadc->max_value;
        sadc_info->sadc_data = ad_data;
        break;
    default:
        rt_kprintf("SADC CMD error:%x\n", cmd);
    }

    rt_mutex_release(&sadc_data->lock);

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops fh_sadc_ops = {
    fh_sadc_init,
    fh_sadc_open,
    fh_sadc_close,
    NULL,
    NULL,
    fh_sadc_ioctl
};
#endif

int fh_sadc_probe(void *priv_data)
{
    struct clk *sadc_clk = RT_NULL;
    rt_device_t sadc_dev = RT_NULL;
    struct fh_sadc *sadc_data = RT_NULL;
    struct wrap_sadc_obj *sadc_obj = (struct wrap_sadc_obj *)priv_data;

    sadc_data = rt_malloc(sizeof(struct fh_sadc));
    if (!sadc_data)
        return -RT_ENOMEM;
    rt_memset(sadc_data, 0, sizeof(struct fh_sadc));
	sadc_data->sadc = sadc_obj;

    sadc_dev = rt_malloc(sizeof(struct rt_device));
    if (!sadc_dev)
        return -RT_ENOMEM;
    rt_memset(sadc_dev, 0, sizeof(struct rt_device));

    sadc_data->rt_dev = sadc_dev;
    sadc_data->active_chan_no = -1;

    sadc_clk = clk_get(NULL, "sadc_clk");
    if (sadc_clk == RT_NULL)
    {
        rt_kprintf("sadc clk get fail\n");
        return -RT_ERROR;
    }
    clk_set_rate(sadc_clk, sadc_obj->frequency);
    clk_enable(sadc_clk);
    /*sadc_obj->frequency = SADC_CLK;*/


    rt_sem_init(&sadc_data->completion, "sadc_sem", 0, RT_IPC_FLAG_FIFO);
    rt_mutex_init(&sadc_data->lock, "sadc_lock", RT_IPC_FLAG_FIFO);

    sadc_dev->user_data = (void *)sadc_data;
#ifdef RT_USING_DEVICE_OPS
    sadc_dev->ops       = &fh_sadc_ops;
#else
    sadc_dev->open      = fh_sadc_open;
    sadc_dev->close     = fh_sadc_close;
    sadc_dev->control   = fh_sadc_ioctl;
    sadc_dev->init      = fh_sadc_init;
#endif
    sadc_dev->type      = RT_Device_Class_Miscellaneous;

    rt_device_register(sadc_dev, "sadc", RT_DEVICE_FLAG_RDWR);
    rt_hw_interrupt_install(sadc_obj->irq_no, fh_sadc_interrupt,
        (void*)sadc_data, "sadc_isr_0");
    rt_hw_interrupt_umask(sadc_obj->irq_no);

    return RT_EOK;
}

struct fh_board_ops sdac_driver_ops = {
    .probe = fh_sadc_probe,
};

void rt_hw_sadc_init(void)
{
    fh_board_driver_register("sadc", &sdac_driver_ops);
}


