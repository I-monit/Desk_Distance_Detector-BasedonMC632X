/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/

#include "drv_cesa_if.h"
#include "fh_def.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

FH_CESA_DEV_P g_cesa = NULL;

static rt_err_t  rt_dev_cesa_init(rt_device_t dev)
{
    g_cesa = (FH_CESA_DEV_P)dev;

    return RT_EOK;
}

static rt_err_t rt_dev_cesa_open(struct rt_device *dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_dev_cesa_close(struct rt_device *dev)
{
    return RT_EOK;
}

static rt_err_t rt_dev_cesa_control(struct rt_device *dev,
        int cmd,
        void *args)
{
    FH_CESA_DEV_P p_cesa = (FH_CESA_DEV_P)dev;
    CMD_ADES_CREATE_S *cmd_ades_create = (CMD_ADES_CREATE_S *)args;
    CMD_ADES_DESTROY_S *cmd_ades_destroy = (CMD_ADES_DESTROY_S *)args;
    CMD_ADES_CTRL_S *cmd_ades_ctrl = (CMD_ADES_CTRL_S *)args;
    CMD_ADES_PROCESS_S *cmd_ades_process = (CMD_ADES_PROCESS_S *)args;
    CMD_ADES_INFO_S *cmd_ades_info = (CMD_ADES_INFO_S *)args;
    CMD_ADES_EFUSE_KEY_S *cmd_efuse_key = (CMD_ADES_EFUSE_KEY_S *)args;
    rt_int32_t ret = CESA_SUCCESS;


    if (dev->ref_count == 0)
        return CESA_IOCTL_DEV_NOT_OPEN;


    switch (cmd)
    {
    case CESA_IOC_ADES_CREATE:
        ret = drv_cesa_ades_create(&(cmd_ades_create->session), p_cesa->p_priv);
        break;
    case CESA_IOC_ADES_DESTROY:
        ret = drv_cesa_ades_destroy(cmd_ades_destroy->session);
        break;
    case CESA_IOC_ADES_CONFIG:
        ret = drv_cesa_ades_config(cmd_ades_ctrl->session, &(cmd_ades_ctrl->ctrl));
        break;
    case CESA_IOC_ADES_PROCESS:
        ret = drv_cesa_ades_process(cmd_ades_process->session,
                                    cmd_ades_process->p_src_addr,
                                    cmd_ades_process->p_dst_addr,
                                    cmd_ades_process->length);
        break;
    case CESA_IOC_ADES_ENCRYPT:
        ret = drv_cesa_ades_config_oper(
                    cmd_ades_process->session,
                    FH_CESA_ADES_OPER_MODE_ENCRYPT);
        if (ret == CESA_SUCCESS)
        {
            ret = drv_cesa_ades_process(
                    cmd_ades_process->session,
                    cmd_ades_process->p_src_addr,
                    cmd_ades_process->p_dst_addr,
                    cmd_ades_process->length);
        }
        break;
    case CESA_IOC_ADES_DECRYPT:
        ret = drv_cesa_ades_config_oper(
                    cmd_ades_process->session,
                    FH_CESA_ADES_OPER_MODE_DECRYPT);
        if (ret == CESA_SUCCESS)
        {
            ret = drv_cesa_ades_process(cmd_ades_process->session,
                    cmd_ades_process->p_src_addr,
                    cmd_ades_process->p_dst_addr,
                    cmd_ades_process->length);
        }
        break;
    case CESA_IOC_ADES_GET_INFO:
        ret = drv_cesa_ades_get_infor(cmd_ades_info->session, &(cmd_ades_info->ctrl));
        break;
    case CESA_IOC_ADES_EFUSE_KEY:
        ret = drv_cesa_ades_efuse_key(cmd_efuse_key->session, &(cmd_efuse_key->efuse_para));
        cmd_efuse_key->ret = ret;
        break;
    default:
        return CESA_IOCTL_CMD_NOT_SUPPORT;
    }

    return ret;
}


static void fh_cesa_ades_intr(int irq, void *param)
{
    FH_CESA_DEV_P p_cesa = (FH_CESA_DEV_P)param;

    drv_cesa_ades_intr(p_cesa->p_priv);
}

#ifdef RT_USING_PM
static void fh_cesa_reset_assert(const struct rt_device *device)
{
    FH_CESA_DEV_P p_cesa = (FH_CESA_DEV_P)device;

    if (!p_cesa || !p_cesa->reset_base || !p_cesa->reset_offset || !p_cesa->reset_bit)
        return;

#ifdef CONFIG_ARCH_MC632X
    SET_REG(p_cesa->reset_base + p_cesa->reset_offset, BIT(p_cesa->reset_bit));
#else
    SET_REG(p_cesa->reset_base + p_cesa->reset_offset, ~BIT(p_cesa->reset_bit));
#endif
}

static void fh_cesa_reset_deassert(const struct rt_device *device)
{
    FH_CESA_DEV_P p_cesa = (FH_CESA_DEV_P)device;

    if (!p_cesa || !p_cesa->reset_base || !p_cesa->reset_offset || !p_cesa->reset_bit)
        return;

#ifdef CONFIG_ARCH_MC632X
    SET_REG(p_cesa->reset_base + p_cesa->reset_offset, 0x0);
#else
    SET_REG(p_cesa->reset_base + p_cesa->reset_offset, 0xffffffff);
#endif
}

static int fh_cesa_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    FH_CESA_DEV_P p_cesa = (FH_CESA_DEV_P)device;

    fh_cesa_reset_assert(p_cesa);
    clk_disable(p_cesa->clk);
    return 0;
}

static void fh_cesa_resume(const struct rt_device *device, rt_uint8_t mode)
{
    FH_CESA_DEV_P p_cesa = (FH_CESA_DEV_P)device;

    clk_enable(p_cesa->clk);
    fh_cesa_reset_deassert(p_cesa);
}

struct rt_device_pm_ops fh_cesa_pm_ops = {
    .suspend_prepare = fh_cesa_suspend,
    .resume_prepare = fh_cesa_resume,
};
#endif

int fh_cesa_probe(void *priv_data)
{
    struct fh_cesa_platform_data *plat_data =
        (struct fh_cesa_platform_data *) priv_data;
    FH_CESA_DEV_P p_cesa;
    FH_CESA_AUX_P p_aux;
    struct clk *clk_rate = RT_NULL;

    p_aux = rt_malloc(sizeof(FH_CESA_AUX_S));
    if (p_aux)
        drv_cesa_aux_init(p_aux);
    else
        return RT_ERROR;

    p_cesa = rt_malloc(sizeof(FH_CESA_DEV_S));
    if (p_cesa)
    {
        p_cesa->ades_irq = plat_data->irq;
        p_cesa->ades_reg = plat_data->base;
        p_cesa->p_priv = p_aux;
        p_cesa->reset_base = plat_data->reset_base;
        p_cesa->reset_offset = plat_data->reset_offset;
        p_cesa->reset_bit = plat_data->reset_bit;
    }
    else
    {
        rt_free(p_aux);
        return RT_ERROR;
    }

    clk_rate = clk_get(NULL, "aes_clk");
    if (clk_rate != RT_NULL)
    {
        p_cesa->clk = clk_rate;
        clk_enable(p_cesa->clk);
    }

    drv_cesa_aux_intr_install(p_cesa->ades_irq,
                              fh_cesa_ades_intr,
                              (void *)p_cesa);

    p_cesa->parent.type = RT_Device_Class_Miscellaneous;
    p_cesa->parent.init = rt_dev_cesa_init;
    p_cesa->parent.open = rt_dev_cesa_open;
    p_cesa->parent.close = rt_dev_cesa_close;
    p_cesa->parent.control = rt_dev_cesa_control;
    rt_device_register((rt_device_t)p_cesa, "cesad", 0);

    rt_device_init((rt_device_t)p_cesa);

#ifdef RT_USING_PM
    rt_pm_device_register((rt_device_t)p_cesa, &fh_cesa_pm_ops);
#endif

    return RT_EOK;
}

int fh_cesa_exit(void *priv_data)
{
    if (g_cesa == NULL)
        return RT_ERROR;

    rt_device_unregister((rt_device_t)g_cesa);
    drv_cesa_aux_deinit(g_cesa->p_priv);
    rt_free(g_cesa->p_priv);
    rt_free(g_cesa);
    g_cesa = NULL;

    return RT_EOK;
}


struct fh_board_ops cesa_driver_ops = {
    .probe = fh_cesa_probe,
    .exit = fh_cesa_exit,
};

void rt_cesa_init(void)
{
    fh_board_driver_register("fh_cesa", &cesa_driver_ops);
}

