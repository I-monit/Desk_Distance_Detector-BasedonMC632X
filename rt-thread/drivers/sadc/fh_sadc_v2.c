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
#include <rtthread.h>
#include <rthw.h>
#include <rtdbg.h>
#include <rtdef.h>



struct fh_sadc {
	struct wrap_sadc_obj *sadc;
	struct rt_mutex lock;
    struct rt_semaphore completion;
    rt_device_t rt_dev;
};


struct wrap_sadc_reg
{
    rt_uint32_t sadc_model;
    rt_uint32_t sadc_cmd;
    rt_uint32_t sadc_control;
    rt_uint32_t sadc_scan_en;
    rt_uint32_t sadc_chn_cfg;
    rt_uint32_t sadc_chn_cfg1;
    rt_uint32_t sadc_glitch_cfg;
    rt_uint32_t sadc_continuous_time;
    rt_uint32_t sadc_glitch_time;
    rt_uint32_t sadc_ier;
    rt_uint32_t sadc_int_status;
    rt_uint32_t sadc_dout0;
    rt_uint32_t sadc_dout1;
    rt_uint32_t sadc_dout2;
    rt_uint32_t sadc_dout3;
    rt_uint32_t sadc_dout0_all;
    rt_uint32_t sadc_dout1_all;
    rt_uint32_t sadc_dout2_all;
    rt_uint32_t sadc_dout3_all;
    rt_uint32_t sadc_debuge0;
    rt_uint32_t sadc_status;
    rt_uint32_t sadc_cnt;
    rt_uint32_t sadc_timeout;
    rt_uint32_t sadc_status2;
    rt_uint32_t sadc_hit_value0;
    rt_uint32_t sadc_hit_value1;
    rt_uint32_t sadc_hit_value2;
    rt_uint32_t sadc_hit_value3;
    rt_uint32_t sadc_hit_cfg;
    rt_uint32_t sadc_hit_data0;
    rt_uint32_t sadc_hit_data1;
    rt_uint32_t sadc_hit_data2;
    rt_uint32_t sadc_hit_data3;
};


#define __raw_writel(v, a) (*(volatile unsigned int *)(a) = (v))
#define __raw_readl(a) (*(volatile unsigned int *)(a))

#define wrap_readl(wrap, name) \
    __raw_readl(&(((struct wrap_sadc_reg *)wrap->regs)->name))

#define wrap_writel(wrap, name, val) \
    __raw_writel((val), &(((struct wrap_sadc_reg *)wrap->regs)->name))


rt_err_t fh_sadc_isr_read_data(struct fh_sadc *sadc_data, rt_uint32_t channel,
                               rt_uint16_t *buf)
{
    rt_uint32_t xainsel = 1 << channel;
    rt_uint32_t xversel = 0;
    rt_uint32_t xpwdb   = 1;
    /* cnt */
    rt_uint32_t sel2sam_pre_cnt = 2;
    rt_uint32_t sam_cnt         = 2;
    rt_uint32_t sam2sel_pos_cnt = 2;
    /* time out */
    rt_uint32_t eoc_tos  = 0xff;
    rt_uint32_t eoc_toe  = 0xff;
    rt_uint32_t time_out = 0xffff;
    /* set isr en.. */
    rt_uint32_t sadc_isr = 0x01;
    /* start */
    rt_uint32_t sadc_cmd = 0x01;
    /* get data */
    rt_uint32_t temp_data = 0;
    rt_err_t ret;
	struct wrap_sadc_obj *sadc = sadc_data->sadc;

    /* control... */
    wrap_writel(sadc, sadc_control, xainsel | (xversel << 8) | (xpwdb << 12));

    wrap_writel(sadc, sadc_cnt,
                sel2sam_pre_cnt | (sam_cnt << 8) | (sam2sel_pos_cnt << 16));

    wrap_writel(sadc, sadc_timeout,
                eoc_tos | (eoc_toe << 8) | (time_out << 16));

    wrap_writel(sadc, sadc_ier, sadc_isr);

    wrap_writel(sadc, sadc_cmd, sadc_cmd);

    /* ret = rt_completion_wait(&sadc->completion, RT_TICK_PER_SECOND / 2); */

    ret = rt_sem_take(&sadc_data->completion, 500);
    if (ret != RT_EOK)
        return ret;

    switch (channel)
    {
    case 0:
    case 1:
        /* read channel 0 1 */
        temp_data = wrap_readl(sadc, sadc_dout0);
        break;

    case 2:
    case 3:
        /* read channel 2 3 */
        temp_data = wrap_readl(sadc, sadc_dout1);
        break;

    case 4:
    case 5:
        /* read channel 4 5 */
        temp_data = wrap_readl(sadc, sadc_dout2);
        break;

    case 6:
    case 7:
        /* read channel 6 7 */
        temp_data = wrap_readl(sadc, sadc_dout3);
        break;
    default:
        break;
    }

    if (channel % 2)
    {
        /* read low 16bit */
        *buf = (rt_uint16_t)(temp_data & 0xffff);
    }
    else
    {
        /* read high 16bit */
        *buf = (rt_uint16_t)(temp_data >> 16);
    }

    return RT_EOK;
}

int fh_sadc_mode_set(rt_uint32_t mode, struct wrap_sadc_obj *sadc)
{
    rt_uint32_t reg;

    reg = wrap_readl(sadc, sadc_model);
    if (mode)
        reg |= 1;
    else
        reg &= (~1);
    wrap_writel(sadc, sadc_model, reg);

    return 0;
}

int fh_sadc_set_act_bit(rt_uint32_t act_bit, struct wrap_sadc_obj *sadc)
{
    rt_uint32_t reg;
    rt_uint32_t value;

    if (act_bit > 12)
        value = 12;
    else
        value = act_bit;

    value = (~(1 << value)) & 0xfff;
    reg = wrap_readl(sadc, sadc_glitch_cfg);
    reg &= (~(0xfff << 8));
    reg |= (value << 8);
    wrap_writel(sadc, sadc_glitch_cfg, reg);

    return 0;
}

static void fh_sadc_interrupt(int irq, void *param)
{
    rt_uint32_t isr_status;
    rt_uint32_t temp_data =  0;
    rt_uint32_t channel = 0;
    rt_uint32_t data = 0;
	struct fh_sadc *sadc_data = (struct fh_sadc *)param;
    struct wrap_sadc_obj *sadc = sadc_data->sadc;
    
    isr_status = wrap_readl(sadc, sadc_int_status);
    /*single intc*/
    if (isr_status & 0x01)
    {
        rt_uint32_t sadc_isr = 0x00;

        wrap_writel(sadc, sadc_ier, sadc_isr);
        wrap_writel(sadc, sadc_int_status, isr_status);
        rt_sem_release(&(sadc_data->completion));
        return;
    }

    if (isr_status & 0x10000)
    {
        channel = wrap_readl(sadc, sadc_status2);
        channel = ((channel >> 12) & 0x7);
        switch (channel/2)
        {
        case 0:
            /*read channel 0 1*/
            temp_data = wrap_readl(sadc, sadc_dout0);
            break;

        case 1:
            /*read channel 2 3*/
            temp_data = wrap_readl(sadc, sadc_dout1);
            break;

        case 2:
            /*read channel 4 5*/
            temp_data = wrap_readl(sadc, sadc_dout2);
            break;

        case 3:
            /*read channel 6 7*/
            temp_data = wrap_readl(sadc, sadc_dout3);
            break;
        default:
            break;
        }

        if (channel % 2)
        {
            /*read low 16bit*/
            data = (temp_data & 0xfff);
        }
        else
        {
            /*read high 16bit*/
            data = (temp_data >> 16) & 0xfff;
        }

        wrap_writel(sadc, sadc_int_status, 0x10000);
    }
    
    

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
    rt_uint32_t control_reg;
    rt_uint16_t ad_raw_data;
    rt_uint32_t value;
    rt_int32_t  status;
    SADC_INFO *sadc_info;
	struct fh_sadc  *sadc_data = (struct fh_sadc  *)dev->user_data;
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

        if ((wrap_readl(sadc, sadc_status2) & 0x10))
        {
            rt_mutex_release(&sadc_data->lock);
            return -EBUSY;
        }

        fh_sadc_mode_set(0, sadc);
		fh_sadc_set_act_bit(12, sadc);

        fh_sadc_isr_read_data(sadc_data, sadc_info->channel, &ad_raw_data);
        sadc_info->sadc_data = ad_raw_data;
        break;
    case SADC_CMD_READ_VOLT:
        sadc_info = (SADC_INFO *)arg;

        if ((wrap_readl(sadc, sadc_status2) & 0x10))
        {
            rt_mutex_release(&sadc_data->lock);
            return -EBUSY;
        }

        fh_sadc_mode_set(0, sadc);
		fh_sadc_set_act_bit(12, sadc);

        fh_sadc_isr_read_data(sadc_data, sadc_info->channel, &ad_raw_data);
        ad_data = ad_raw_data * sadc->ref_vol;
        ad_data /= sadc->max_value;
        sadc_info->sadc_data = ad_data;
        break;
    case SADC_CMD_DISABLE:
        control_reg = wrap_readl(sadc, sadc_control);
        control_reg &= ~(1 << 12);
        wrap_writel(sadc, sadc_control, control_reg);
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
	struct fh_sadc *sadc_data =RT_NULL;
    struct wrap_sadc_obj *sadc_obj = RT_NULL;

    
	sadc_data = rt_malloc(sizeof(struct fh_sadc ));
	if (!sadc_data )
		return -RT_ENOMEM;
	rt_memset(sadc_data , 0, sizeof(struct fh_sadc ));
    sadc_data->sadc = (struct wrap_sadc_obj *)priv_data;
	sadc_obj = sadc_data->sadc;
    sadc_dev = rt_malloc(sizeof(struct rt_device));
    if (!sadc_dev)
        return -RT_ENOMEM;
    rt_memset(sadc_dev, 0, sizeof(struct rt_device));

    sadc_clk = clk_get(NULL, "sadc_clk");
    if (sadc_clk == RT_NULL)
    {
        LOG_E("sadc clk get fail\n");
        return -RT_ERROR;
    }
    clk_set_rate(sadc_clk, sadc_obj->frequency);
    clk_enable(sadc_clk);
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
        (void *)sadc_data, "sadc_isr_0");
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


