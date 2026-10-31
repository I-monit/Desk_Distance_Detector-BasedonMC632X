/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-08     wangyl307    zy&zy2 compatible support
 * 2019-01-14     wangyl307    remove pad fix funcs and defs
 */


#include "pinctrl.h"
#include "iopad.h"
#include "rtservice.h"
#include <fh_chip.h>
#include <fh_pmu.h>
#ifdef RT_USING_PM
#include "pm.h"
#endif


/* #define  FH_PINCTRL_DEBUG */
#ifdef FH_PINCTRL_DEBUG
#define PRINT_DBG(fmt, args...)  OS_PRINT(fmt, ##args)
#else
#define PRINT_DBG(fmt, args...)  do{} while (0)
#endif

static struct
{
    void *vbase;
    void *pbase;
    PinCtrl_Pin * pinlist[PAD_NUM];
} pinctrl_obj;
OS_LIST fh_pinctrl_devices = OS_LIST_INIT(fh_pinctrl_devices);

#ifdef FH_USING_AON_PIN
static struct
{
    void *vbase;
    void *pbase;
    PinCtrl_Pin *pinlist[AON_PAD_NUM];
} aon_pinctrl_obj;
OS_LIST fh_pinctrl_aon_devices = OS_LIST_INIT(fh_pinctrl_aon_devices);
#endif

static int fh_pinctrl_func_select(PinCtrl_Pin *pin, unsigned int flag)
{
    unsigned int reg;

    if (!pin)
    {
        OS_PRINT("ERROR: pin is null\n\n");
        return -1;
    }

    if (flag & NEED_CHECK_PINLIST)
    {
        if (pinctrl_obj.pinlist[pin->pad_id])
        {
            OS_PRINT("ERROR: pad %d has already been set\n\n", pin->pad_id);
            return -2;
        }
    }

    reg = GET_REG(pinctrl_obj.vbase + pin->reg_offset);

    pin->reg = (PinCtrl_Register *)&reg;

    pin->reg->bit.mfs = pin->func_sel;

    if (pin->pullup_pulldown == PUPD_DOWN)
    {
        pin->reg->bit.pdn = PUPD_ENABLE;
        pin->reg->bit.pun = PUPD_DISABLE;
    }
    else if (pin->pullup_pulldown == PUPD_UP)
    {
        pin->reg->bit.pdn = PUPD_DISABLE;
        pin->reg->bit.pun = PUPD_ENABLE;
    }
    else
    {
        pin->reg->bit.pdn = PUPD_ZERO;
        pin->reg->bit.pun = PUPD_ZERO;
    }

    pin->reg->bit.ds = pin->driving_curr;
    pin->reg->bit.st = 1;
    pin->reg->bit.ie = 1;
    pin->reg->bit.oe = pin->output_enable;

    SET_REG(pinctrl_obj.vbase + pin->reg_offset, pin->reg->dw);

    pinctrl_obj.pinlist[pin->pad_id] = pin;

    return 0;
}

#ifdef FH_USING_AON_PIN
static int fh_pinctrl_aon_func_select(PinCtrl_Pin *pin, unsigned int flag)
{
    unsigned int reg;

    if (!pin)
    {
        OS_PRINT("ERROR: pin is null\n\n");
        return -1;
    }

    if (flag & NEED_CHECK_PINLIST)
    {
        if (aon_pinctrl_obj.pinlist[pin->pad_id])
        {
            OS_PRINT("ERROR: pad %d has already been set\n\n", pin->pad_id);
            return -2;
        }
    }

    reg = GET_REG(aon_pinctrl_obj.vbase + pin->reg_offset);

    pin->reg = (PinCtrl_Register *)&reg;

    pin->reg->bit.mfs = pin->func_sel;

    if (pin->pullup_pulldown == PUPD_DOWN)
    {
        pin->reg->bit.pdn = PUPD_ENABLE;
        pin->reg->bit.pun = PUPD_DISABLE;
    }
    else if (pin->pullup_pulldown == PUPD_UP)
    {
        pin->reg->bit.pdn = PUPD_DISABLE;
        pin->reg->bit.pun = PUPD_ENABLE;
    }
    else
    {
        pin->reg->bit.pdn = PUPD_ZERO;
        pin->reg->bit.pun = PUPD_ZERO;
    }

    pin->reg->bit.ds = pin->driving_curr;
    pin->reg->bit.st = 1;
    pin->reg->bit.ie = 1;
    pin->reg->bit.oe = pin->output_enable;

    SET_REG(aon_pinctrl_obj.vbase + pin->reg_offset, pin->reg->dw);

    aon_pinctrl_obj.pinlist[pin->pad_id] = pin;

    return 0;
}
#endif

static int fh_pinctrl_mux_switch(PinCtrl_Mux *mux, unsigned int flag)
{
    if (mux->cur_pin >= MUX_NUM)
    {
        OS_PRINT("ERROR: selected function is not exist, sel_func=%d\n\n", mux->cur_pin);
        return -3;
    }

    if (!mux->mux_pin[mux->cur_pin])
    {
        OS_PRINT("ERROR: mux->mux_pin[%d] has no pin\n\n", mux->cur_pin);
        return -4;
    }

    PRINT_DBG("\t%s[%d]\n", mux->mux_pin[mux->cur_pin]->func_name, mux->cur_pin);
    return fh_pinctrl_func_select(mux->mux_pin[mux->cur_pin], flag);
}

#ifdef FH_USING_AON_PIN
static int fh_pinctrl_aon_mux_switch(PinCtrl_Mux *mux, unsigned int flag)
{
    if (mux->cur_pin >= MUX_NUM)
    {
        OS_PRINT("ERROR: aon selected function is not exist, sel_func=%d\n\n", mux->cur_pin);
        return -3;
    }

    if (!mux->mux_pin[mux->cur_pin])
    {
        OS_PRINT("ERROR: aon mux->mux_pin[%d] has no pin\n\n", mux->cur_pin);
        return -4;
    }

    PRINT_DBG("\taon %s[%d]\n", mux->mux_pin[mux->cur_pin]->func_name, mux->cur_pin);
    return fh_pinctrl_aon_func_select(mux->mux_pin[mux->cur_pin], flag);
}
#endif


static int fh_pinctrl_device_switch(PinCtrl_Device *dev, unsigned int flag)
{
    int i;

    for (i = 0; i < dev->mux_count; i++)
    {
        unsigned int *mux_addr = (unsigned int *)((unsigned int)dev
                + sizeof(*dev) - 4 + i*4);
        PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

        fh_pinctrl_mux_switch(mux, flag);
    }

#ifdef REG_PMU_SD1_FUNC_SEL
    if (rt_strncmp(dev->dev_name, "SD1", 3) == 0)
    {
        PinCtrl_Mux *mux = (PinCtrl_Mux *)dev->mux;

        /* for some platform, set SD1_FUNC_SEL after all mux switch */
        fh_pmu_set_sdc1_funcsel(mux->cur_pin);
    }
#endif
    return 0;
}

#ifdef FH_USING_AON_PIN
static int fh_pinctrl_aon_device_switch(PinCtrl_Device *dev, unsigned int flag)
{
    int i;

    for (i = 0; i < dev->mux_count; i++)
    {
        unsigned int *mux_addr = (unsigned int *)((unsigned int)dev
                + sizeof(*dev) - 4 + i*4);
        PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

        fh_pinctrl_aon_mux_switch(mux, flag);
    }

#ifdef REG_PMU_SD1_FUNC_SEL
    if (rt_strncmp(dev->dev_name, "SD1", 3) == 0)
    {
        PinCtrl_Mux *mux = (PinCtrl_Mux *)dev->mux;

        /* for some platform, set SD1_FUNC_SEL after all mux switch */
        fh_pmu_set_sdc1_funcsel(mux->cur_pin);
    }
#endif
    return 0;
}
#endif

extern void stage_log(unsigned int val);
static PinCtrl_Device *fh_pinctrl_get_device_by_name(char *name)
{
    PinCtrl_Device *dev = OS_NULL;

    rt_list_for_each_entry(dev, &fh_pinctrl_devices, list)
    {
        if (!rt_strcmp(name, dev->dev_name))
        {
            return dev;
        }
    }

    return 0;
}

#ifdef FH_USING_AON_PIN
static PinCtrl_Device *fh_pinctrl_aon_get_device_by_name(char *name)
{
    PinCtrl_Device *dev = OS_NULL;

    rt_list_for_each_entry(dev, &fh_pinctrl_aon_devices, list)
    {
        if (!rt_strcmp(name, dev->dev_name))
        {
            return dev;
        }
    }

    return 0;
}
#endif

static PinCtrl_Mux *fh_pinctrl_get_mux_by_name(char *mux_name)
{
    int i;
    PinCtrl_Device *dev;

    rt_list_for_each_entry(dev, &fh_pinctrl_devices, list)
    {
        for (i = 0; i < dev->mux_count; i++)
        {
            unsigned int *mux_addr = (unsigned int *)(
                (unsigned int)dev + sizeof(*dev) - 4 + i*4);
            PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

            if (!rt_strcmp(mux_name, mux->mux_pin[0]->func_name))
                return mux;
        }
    }

    return NULL;
}

#ifdef FH_USING_AON_PIN
static PinCtrl_Mux *fh_pinctrl_aon_get_mux_by_name(char *mux_name)
{
    int i;
    PinCtrl_Device *dev;

    rt_list_for_each_entry(dev, &fh_pinctrl_aon_devices, list)
    {
        for (i = 0; i < dev->mux_count; i++)
        {
            unsigned int *mux_addr = (unsigned int *)(
                (unsigned int)dev + sizeof(*dev) - 4 + i*4);
            PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

            if (!rt_strcmp(mux_name, mux->mux_pin[0]->func_name))
                return mux;
        }
    }

    return NULL;
}
#endif

int fh_pinctrl_check_pinlist(void)
{
    int i;

    for (i = 0; i < PAD_NUM; i++)
    {
        if (!pinctrl_obj.pinlist[i])
        {
            PRINT_DBG("ERROR: pad %d is still empty\n", i);
        }
    }

    return 0;
}

#ifdef FH_USING_AON_PIN
int fh_pinctrl_check_aon_pinlist(void)
{
    int i;

    for (i = 0; i < AON_PAD_NUM; i++)
    {
        if (!aon_pinctrl_obj.pinlist[i])
        {
            PRINT_DBG("ERROR: pad %d is still empty\n", i);
        }
    }

    return 0;
}
#endif

static int fh_pinctrl_init_devices(char **devlist, int listsize, unsigned int flag)
{
    int i, ret;
    PinCtrl_Device *dev;

    rt_memset(pinctrl_obj.pinlist, 0, sizeof(pinctrl_obj.pinlist));

    for (i = 0; i < listsize; i++)
    {
        if (!devlist[i])
            continue;
        dev = fh_pinctrl_get_device_by_name(devlist[i]);

        if (!dev)
        {
            OS_PRINT("[PINMUX] ERROR: cannot find device %s\n", devlist[i]);
            return -5;
        }

        PRINT_DBG("%s:\n", dev->dev_name);
        ret = fh_pinctrl_device_switch(dev, flag);
        PRINT_DBG("\n");
        if (ret)
        {
            return ret;
        }

    }

    if (flag & NEED_CHECK_PINLIST)
        fh_pinctrl_check_pinlist();

    return 0;

}

#ifdef FH_USING_AON_PIN
static int fh_pinctrl_init_aon_devices(char **devlist, int listsize, unsigned int flag)
{
    int i, ret;
    PinCtrl_Device *dev;

    rt_memset(aon_pinctrl_obj.pinlist, 0, sizeof(aon_pinctrl_obj.pinlist));

    for (i = 0; i < listsize; i++)
    {
        if (!devlist[i])
            continue;
        dev = fh_pinctrl_aon_get_device_by_name(devlist[i]);

        if (!dev)
        {
            OS_PRINT("[PINMUX] ERROR: aon cannot find device %s\n", devlist[i]);
            return -5;
        }

        PRINT_DBG("aon %s:\n", dev->dev_name);
        ret = fh_pinctrl_aon_device_switch(dev, flag);
        PRINT_DBG("\n");
        if (ret)
        {
            return ret;
        }

    }

    if (flag & NEED_CHECK_PINLIST)
        fh_pinctrl_check_aon_pinlist();

    return 0;

}
#endif

static void fh_pinctrl_init_pin(void)
{
    int i;

    for (i = 0; i < PAD_NUM; i++)
    {
        PinCtrl_Pin *pin = pinctrl_obj.pinlist[i];

        if (!pin)
        {
            PinCtrl_Register reg;

            PRINT_DBG("ERROR: pad %d is empty\n", i);
            reg.dw = GET_REG(pinctrl_obj.vbase + i * 4);
            reg.bit.oe = 0;
            SET_REG(pinctrl_obj.vbase + i * 4, reg.dw);
            continue;
        }
        pin->reg->dw = GET_REG(pinctrl_obj.vbase +
                pin->reg_offset);

        pin->input_enable = pin->reg->bit.ie;
        pin->output_enable = pin->reg->bit.oe;
    }
}

#ifdef FH_USING_AON_PIN
static void fh_pinctrl_init_aon_pin(void)
{
    int i;

    for (i = 0; i < AON_PAD_NUM; i++)
    {
        PinCtrl_Pin *aon_pin = aon_pinctrl_obj.pinlist[i];

        if (!aon_pin)
        {
            PinCtrl_Register aon_reg;

            PRINT_DBG("ERROR: aon pad %d is empty\n", i);
            aon_reg.dw = GET_REG(aon_pinctrl_obj.vbase + i * 4);
            aon_reg.bit.oe = 0;
            SET_REG(aon_pinctrl_obj.vbase + i * 4, aon_reg.dw);
            continue;
        }
        aon_pin->reg->dw = GET_REG(aon_pinctrl_obj.vbase +
                aon_pin->reg_offset);

        aon_pin->input_enable = aon_pin->reg->bit.ie;
        aon_pin->output_enable = aon_pin->reg->bit.oe;
    }
}
#endif
#ifdef RT_USING_PM
static unsigned int *pinmux_reg_val = RT_NULL;

int pinctrl_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    unsigned int i;

    for (i = 0; i < PAD_NUM; i++)
        pinmux_reg_val[i] = GET_REG(pinctrl_obj.vbase + i * 4);

    return RT_EOK;
}

void pinctrl_resume(const struct rt_device *device, rt_uint8_t mode)
{
    unsigned int i;

    for (i = 0; i < PAD_NUM; i++)
        SET_REG(pinctrl_obj.vbase + i * 4, pinmux_reg_val[i]);
}

struct rt_device_pm_ops pinctrl_pm_ops = {
    .suspend_late = pinctrl_suspend,
    .resume_late = pinctrl_resume
};
#endif

void fh_pinctrl_init(unsigned int base)
{
    pinctrl_obj.vbase = pinctrl_obj.pbase = (void *)base;

    fh_pinctrl_init_devicelist(&fh_pinctrl_devices);
    fh_pinctrl_init_devices(fh_pinctrl_selected_devices,
            ARRAY_SIZE(fh_pinctrl_selected_devices),
            NEED_CHECK_PINLIST);
    fh_pinctrl_init_pin();
#ifdef RT_USING_PM
    pinmux_reg_val = rt_malloc(sizeof(unsigned int) * PAD_NUM);
    if (pinmux_reg_val == RT_NULL)
    {
        rt_kprintf("pinmux malloc pinmux_reg_val failed\n");
        return;
    }
    rt_pm_device_register((struct rt_device *)&pinctrl_obj, &pinctrl_pm_ops);
#endif
}

#ifdef FH_USING_AON_PIN
void fh_aon_pinctrl_init(unsigned int base)
{
    aon_pinctrl_obj.vbase = aon_pinctrl_obj.pbase = (void *)base;

    fh_pinctrl_init_aon_devicelist(&fh_pinctrl_aon_devices);
    fh_pinctrl_init_aon_devices(fh_pinctrl_selected_aon_devices,
            ARRAY_SIZE(fh_pinctrl_selected_aon_devices),
            NEED_CHECK_PINLIST);
    fh_pinctrl_init_aon_pin();
}
#endif

static char *pupdstrs[3] = {
    [PUPD_NONE] = "none",
    [PUPD_UP] = "up",
    [PUPD_DOWN] = "down",
};

void fh_pinctrl_prt(void)
{
    int i;
    int reg_start = (int)pinctrl_obj.pbase & 0xffff;
#ifdef FH_USING_AON_PIN
    int aon_reg_start = (int)aon_pinctrl_obj.pbase & 0xffff;
#endif

    OS_PRINT(" id name             addr        val    sel  ie  oe  pupd  ds\n");

    for (i = 0; i < PAD_NUM; i++)
    {
        if (!pinctrl_obj.pinlist[i])
        {
            continue;
        }

        OS_PRINT("%3d %-16s 0x%04x  0x%08x %3d %3d %3d  %4s %3d\n",
                pinctrl_obj.pinlist[i]->pad_id,
                pinctrl_obj.pinlist[i]->func_name,
                pinctrl_obj.pinlist[i]->reg_offset + reg_start,
                GET_REG(pinctrl_obj.vbase + pinctrl_obj.pinlist[i]->reg_offset),
                pinctrl_obj.pinlist[i]->func_sel,
                pinctrl_obj.pinlist[i]->input_enable,
                pinctrl_obj.pinlist[i]->output_enable,
                pupdstrs[pinctrl_obj.pinlist[i]->pullup_pulldown],
                pinctrl_obj.pinlist[i]->driving_curr);
    }
#ifdef FH_USING_AON_PIN
    OS_PRINT("AON:\n");
    for (i = 0; i < AON_PAD_NUM; i++)
    {
        if (!aon_pinctrl_obj.pinlist[i])
        {
            continue;
        }

        OS_PRINT("%3d %-16s 0x%04x  0x%08x %3d %3d %3d  %4s %3d\n",
                aon_pinctrl_obj.pinlist[i]->pad_id,
                aon_pinctrl_obj.pinlist[i]->func_name,
                aon_pinctrl_obj.pinlist[i]->reg_offset + aon_reg_start,
                GET_REG(aon_pinctrl_obj.vbase + aon_pinctrl_obj.pinlist[i]->reg_offset),
                aon_pinctrl_obj.pinlist[i]->func_sel,
                aon_pinctrl_obj.pinlist[i]->input_enable,
                aon_pinctrl_obj.pinlist[i]->output_enable,
                pupdstrs[aon_pinctrl_obj.pinlist[i]->pullup_pulldown],
                aon_pinctrl_obj.pinlist[i]->driving_curr);
    }
#endif
}

int fh_pinctrl_smux(char *devname, char *muxname, int muxsel, unsigned int flag)
{
    PinCtrl_Device *dev;
    int i, ret;
    PinCtrl_Mux *foundmux = NULL;
    char *oldfunc = NULL;
    PinCtrl_Pin *pin = NULL;
#ifdef FH_USING_AON_PIN
    int found_in_aon = 0;
#endif

    if (flag & PIN_RESTORE)
    {
        foundmux = fh_pinctrl_get_mux_by_name(muxname);
        if (foundmux == NULL)
        {
#ifdef FH_USING_AON_PIN
            foundmux = fh_pinctrl_aon_get_mux_by_name(muxname);
            if (foundmux == NULL)
            {
                OS_PRINT("ERROR: PIN_RESTORE, cannot found mux: %s\n",
                muxname);
                return -10;
            }
            found_in_aon = 1;
#else
            OS_PRINT("ERROR: PIN_RESTORE, cannot found mux: %s\n",
                muxname);
            return -10;
#endif
        }
        goto mux_switch;
    }
    dev = fh_pinctrl_get_device_by_name(devname);

    if (!dev)
    {
#ifdef FH_USING_AON_PIN
        dev = fh_pinctrl_aon_get_device_by_name(devname);
        if (!dev)
        {
            OS_PRINT("ERROR: cannot find device %s\n", devname);
            return -4;
        }
        found_in_aon = 1;
#else
        OS_PRINT("ERROR: cannot find device %s\n", devname);
        return -4;
#endif
    }

    for (i = 0; i < dev->mux_count; i++)
    {
        unsigned int *mux_addr = (unsigned int *)((unsigned int)dev
                + sizeof(*dev) - 4 + i*4);
        PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

        if (!rt_strcmp(muxname, mux->mux_pin[0]->func_name))
        {
            foundmux = mux;
            mux->cur_pin = muxsel;
            goto mux_switch;
        }
    }

    if (i == dev->mux_count)
    {
        OS_PRINT("ERROR: cannot find mux %s of device %s\n",
            muxname, devname);
        return -6;
    }

mux_switch:
    if (flag & PIN_BACKUP)
    {
        int id = foundmux->mux_pin[foundmux->cur_pin]->pad_id;
#ifdef FH_USING_AON_PIN
        if (found_in_aon)
            pin = aon_pinctrl_obj.pinlist[id];
        else
            pin = pinctrl_obj.pinlist[id];
#else
        pin = pinctrl_obj.pinlist[id];
#endif

        if (pin == NULL)
        {
            OS_PRINT("ERROR: PIN_BACKUP, oldpin is null\n");
            return 0;
        }
        oldfunc = pin->func_name;
    }
#ifdef FH_USING_AON_PIN
    if (found_in_aon)
    {
        ret = fh_pinctrl_aon_mux_switch(foundmux, flag);
        if (flag & PIN_BACKUP)
            ret = (int)oldfunc;

        if (flag & NEED_CHECK_PINLIST)
            fh_pinctrl_check_aon_pinlist();

        return ret;
    }
    else
    {
        ret = fh_pinctrl_mux_switch(foundmux, flag);
        if (flag & PIN_BACKUP)
            ret = (int)oldfunc;

        if (flag & NEED_CHECK_PINLIST)
            fh_pinctrl_check_pinlist();

        return ret;
    }
#else
    ret = fh_pinctrl_mux_switch(foundmux, flag);
    if (flag & PIN_BACKUP)
        ret = (int)oldfunc;

    if (flag & NEED_CHECK_PINLIST)
        fh_pinctrl_check_pinlist();

    return ret;
#endif
}

char *fh_pinctrl_smux_backup(char *devname, char *muxname, int muxsel)
{
    return (char *)fh_pinctrl_smux(devname, muxname, muxsel, PIN_BACKUP);
}

char *fh_pinctrl_smux_restore(char *devname, char *muxname, int muxsel)
{
    return (char *)fh_pinctrl_smux(devname, muxname, muxsel, PIN_RESTORE);
}

int fh_pinctrl_sdev(char *devname, unsigned int flag)
{
    PinCtrl_Device *dev;
    int ret;
    int print = !(flag & NO_PRINT);
#ifdef FH_USING_AON_PIN
    int found_in_aon = 0;
#endif

    dev = fh_pinctrl_get_device_by_name(devname);
    if (!dev)
    {
#ifdef FH_USING_AON_PIN
        dev = fh_pinctrl_aon_get_device_by_name(devname);
        if (!dev)
        {
            OS_PRINT("ERROR: cannot find device %s\n", devname);
            return -7;
        }
        found_in_aon = 1;
#else
        OS_PRINT("ERROR: cannot find device %s\n", devname);
        return -7;
#endif
    }

    if (print)
        OS_PRINT("%s:\n\n", dev->dev_name);
#ifdef FH_USING_AON_PIN
    if (found_in_aon)
        ret = fh_pinctrl_aon_device_switch(dev, flag);
    else
        ret = fh_pinctrl_device_switch(dev, flag);
#else
    ret = fh_pinctrl_device_switch(dev, flag);
#endif

    if (ret)
    {
        return ret;
    }

    if (flag & NEED_CHECK_PINLIST)
    {
#ifdef FH_USING_AON_PIN
        if (found_in_aon)
            fh_pinctrl_check_aon_pinlist();
        else
            fh_pinctrl_check_pinlist();
#else
        fh_pinctrl_check_pinlist();
#endif
    }

    return 0;
}

static PinCtrl_Pin *fh_pinctrl_get_pin_by_name(char *pin_name)
{
    int i;
    PinCtrl_Pin *pin = NULL;

    for (i = 0; i < PAD_NUM; i++)
    {
        pin = pinctrl_obj.pinlist[i];
        if (!pin || !pin->func_name)
        {
            continue;
        }
        if (!rt_strncmp(pin->func_name, pin_name, RT_NAME_MAX))
        {
            return pin;
        }
    }

    return NULL;
}

#ifdef FH_USING_AON_PIN
static PinCtrl_Pin *fh_pinctrl_get_aon_pin_by_name(char *pin_name)
{
    int i;
    PinCtrl_Pin *pin = NULL;

    for (i = 0; i < AON_PAD_NUM; i++)
    {
        pin = aon_pinctrl_obj.pinlist[i];
        if (!pin || !pin->func_name)
        {
            continue;
        }
        if (!rt_strncmp(pin->func_name, pin_name, RT_NAME_MAX))
        {
            return pin;
        }
    }

    return NULL;
}
#endif

int fh_pinctrl_pupd(char *pin_name, char *pupd)
{
    PinCtrl_Pin *pin = NULL;
    int pupd_val = -1;
    int i = 0;
#ifdef FH_USING_AON_PIN
    int found_in_aon = 0;
#endif

    for (i = 0; i < ARRAY_SIZE(pupdstrs); i++)
    {
        if (pupdstrs[i] && rt_strncmp(pupd, pupdstrs[i], RT_NAME_MAX) == 0)
        {
            pupd_val = i;
            break;
        }
    }
    if (pupd_val < 0)
    {
        OS_PRINT("ERROR: pupd %s\n", pupd);
        return -EINVAL;
    }

    pin = fh_pinctrl_get_pin_by_name(pin_name);

    if (!pin)
    {
#ifdef FH_USING_AON_PIN
        pin = fh_pinctrl_get_aon_pin_by_name(pin_name);
        if (!pin)
        {
            OS_PRINT("ERROR: cannot find pin %s\n", pin_name);
            return -ENXIO;
        }
        found_in_aon = 1;
#else
        OS_PRINT("ERROR: cannot find pin %s\n", pin_name);
        return -ENXIO;
#endif
    }

    pin->pullup_pulldown = pupd_val;
#ifdef FH_USING_AON_PIN
    if (found_in_aon)
        return fh_pinctrl_aon_func_select(pin, 0);
    else
        return fh_pinctrl_func_select(pin, 0);
#else
    return fh_pinctrl_func_select(pin, 0);
#endif
}

int fh_pinctrl_ds(char *pin_name, unsigned int ds)
{
    PinCtrl_Pin *pin = NULL;
#ifdef FH_USING_AON_PIN
    int found_in_aon = 0;
#endif

    if (ds > 7)
    {
        OS_PRINT("ds val is in [0-7]\n");
        return -EINVAL;
    }

    pin = fh_pinctrl_get_pin_by_name(pin_name);

    if (!pin)
    {
#ifdef FH_USING_AON_PIN
        pin = fh_pinctrl_get_aon_pin_by_name(pin_name);
        if (!pin)
        {
            OS_PRINT("ERROR: cannot find pin %s\n", pin_name);
            return -ENXIO;
        }
        found_in_aon = 1;
#else
        OS_PRINT("ERROR: cannot find pin %s\n", pin_name);
        return -ENXIO;
#endif
    }

    pin->driving_curr = ds;
#ifdef FH_USING_AON_PIN
    if (found_in_aon)
        return fh_pinctrl_aon_func_select(pin, 0);
    else
        return fh_pinctrl_func_select(pin, 0);
#else
    return fh_pinctrl_func_select(pin, 0);
#endif
}

int fh_pinctrl_set_oe(char *pin_name, unsigned int oe)
{
    PinCtrl_Pin *pin = NULL;
#ifdef FH_USING_AON_PIN
    int found_in_aon = 0;
#endif

    if (oe > 1)
    {
        OS_PRINT("oe is 0 or 1\n");
        return -1;
    }

    pin = fh_pinctrl_get_pin_by_name(pin_name);

    if (!pin)
    {
#ifdef FH_USING_AON_PIN
        pin = fh_pinctrl_get_aon_pin_by_name(pin_name);
        if (!pin)
        {
            OS_PRINT("ERROR: cannot find pin %s\n", pin_name);
            return -ENXIO;
        }
        found_in_aon = 1;
#else
        OS_PRINT("ERROR: cannot find pin %s\n", pin_name);
        return -ENXIO;
#endif
    }

    pin->output_enable = oe;
#ifdef FH_USING_AON_PIN
    if (found_in_aon)
        return fh_pinctrl_aon_func_select(pin, 0);
    else
        return fh_pinctrl_func_select(pin, 0);
#else
    return fh_pinctrl_func_select(pin, 0);
#endif
}

int fh_select_gpio(int gpio)
{
    return 0;
}

#ifdef FINSH_USING_MSH
#include <stdlib.h>
#include <stdio.h>

static void pinctrl_usage(void)
{
    rt_kprintf("pin mux utilities.\n\n");
    rt_kprintf("Usage:\n");
    rt_kprintf("  pinctrl -l                     show pinctrl table\n");
    rt_kprintf("  pinctrl [-h]                   show this help\n");
    rt_kprintf("  pinctrl -m dev mux sel         device mux selection\n");
    rt_kprintf("  pinctrl -d device              mux device\n");
    rt_kprintf("  pinctrl -p pin [up|down|none]  pupd pad output\n");
    rt_kprintf("  pinctrl -f pin (0-7)           device ds func.\n");
}

static int pinctrl(int argc, char *argv[])
{
    if (argc < 2)
        goto usage;

    if (rt_strcmp(argv[1], "-h") == 0)
        goto usage;

    if (rt_strcmp(argv[1], "-l") == 0)
    {
        fh_pinctrl_prt();
        return 0;
    }
    if (rt_strcmp(argv[1], "-m") == 0)
    {
        if (argc < 5)
            goto usage;
        return fh_pinctrl_smux(argv[2], argv[3], rt_atoi(argv[4]), 0);
    }
    if (rt_strcmp(argv[1], "-d") == 0)
    {
        if (argc < 3)
            goto usage;

        return fh_pinctrl_sdev(argv[2], 0);
    }
    if (rt_strcmp(argv[1], "-f") == 0)
    {
        if (argc < 4)
            goto usage;

        return fh_pinctrl_ds(argv[2], rt_atoi(argv[3]));
    }
    if (rt_strcmp(argv[1], "-p") == 0)
    {
        if (argc < 4)
            goto usage;

        return fh_pinctrl_pupd(argv[2], argv[3]);
    }

usage:
    pinctrl_usage();
    return 0;
}

MSH_CMD_EXPORT(pinctrl, device pin mux control);
#elif defined(RT_USING_FINSH)
#include <finsh.h>
FINSH_FUNCTION_EXPORT(fh_pinctrl_prt, print pinlist);
FINSH_FUNCTION_EXPORT(fh_pinctrl_smux, mux switch by name);
FINSH_FUNCTION_EXPORT(fh_pinctrl_sdev, dev switch by name);
FINSH_FUNCTION_EXPORT(fh_pinctrl_pupd, (pin, up|down|none) change pupd by name);
FINSH_FUNCTION_EXPORT(fh_pinctrl_ds, (pin, val) change ds by name);
#endif

