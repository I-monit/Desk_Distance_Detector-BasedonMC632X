#include <rtdevice.h>
#include <rthw.h>
#include "fh_def.h"
#include "board_info.h"
#include "stepmotor.h"
#include "fh_stepmotor.h"
#include "fh_clock.h"
#include "fh_pmu.h"

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/

#include "fh_stepmotor.h"

#define FHSM_DEF_PERIOD (128)

struct fh_sm_obj_t
{
    int irq_no;
    void *regs;
    struct clk *clk;
    struct rt_completion    startsync_complete;
    struct rt_completion    stop_complete;
    char isrname[24];
    int id;

};
struct fh_sm_obj_t *fh_sm_obj[MAX_FHSM_NR];



void  fh_stepmotor_isr(int vector, void *param)
{
    struct fh_sm_obj_t *obj = (struct fh_sm_obj_t *)param;

    SET_REG(obj->regs+MOTOR_INT_EN, 0);
    SET_REG(obj->regs+MOTOR_INT_STATUS, 0);
    SET_REG(obj->regs+MOTOR_INT_EN, 1);

    rt_completion_done(&obj->startsync_complete);

    rt_completion_done(&obj->stop_complete);

    return;
}


static int fh_stepmotor_is_busy(struct fh_sm_obj_t *obj)
{
    int busy = GET_REG(obj->regs+MOTOR_STATUS0)&0x01;
    return busy;
}


static void fh_stepmotor_set_hw_param(struct fh_sm_obj_t *obj, struct fh_sm_param *param)
{
    unsigned int reg;

    reg = GET_REG(obj->regs+MOTOR_MODE);
    reg &= (~0x3);
    reg |= param->mode&0x3;

    if (param->direction)
        reg |= 0x1<<4;
    else
        reg &= (~(0x1<<4));


    if (param->output_invert_A)
        reg |= 0x1<<5;
    else
        reg &= (~(0x1<<5));

    if (param->output_invert_B)
        reg |= 0x1<<6;
    else
        reg &= (~(0x1<<6));


    reg &= (~(0xf<<8));
    reg |= ((param->timingparam.microstep & 0xf)<<8);
#ifdef FH_STEPMOTOR_V1_1
    if (param->keep)
        reg |= 0x1<<7;
    else
        reg &= (~(0x1<<7));

    if (param->initparam.NeedInit)
        reg |= 0x1<<12;
    else
        reg &= (~(0x1<<12));

    reg &= (~(0xff<<16));
    reg |= ((param->initparam.InitRunNum & 0xff)<<16);

    if (param->autostopparam.AutoStopEn)
    {
        reg |= 0x1<<2;
        if (param->autostopparam.AutoStopSigPol)
            reg |= 0x1<<3;
        else
            reg &= (~(0x1<<3));

        if (param->autostopparam.AutoStopSigDeb)
            SET_REG(
                obj->regs+MOTOR_DEBOUNCE,
                1|(1<<16)|((param->autostopparam.AutoStopSigDebSet&0x3fff)<<2));
        else
            SET_REG(obj->regs+MOTOR_DEBOUNCE, 0);

    } else
        reg &= (~(0x1<<2));

    fh_pmu_set_stmautostopgpio(obj->id, param->autostopparam.AutoStopChechGPIO);

#endif
#ifdef FH_STEPMOTOR_V1_2
    if (param->dampen)
    {
        reg |= ((param->dampen & 0x1)<<13);
        reg &= (~(0xf<<24));
        reg |= ((param->damp & 0xf)<<24);
    } else
        reg &= (~(0x1<<13));

    if (param->stoplevel)
        reg |= ((param->stoplevel & 0x1)<<14);
    else
        reg &= (~(0x1<<14));
#endif
    SET_REG(obj->regs+MOTOR_MODE, reg);

    reg = GET_REG(obj->regs+MOTOR_TIMING0);
    reg = (param->timingparam.period<<16)|param->timingparam.counter;
    SET_REG(obj->regs+MOTOR_TIMING0, reg);


    reg = GET_REG(obj->regs+MOTOR_TIMING1);
    reg &= (~0xff);
    reg |= (param->timingparam.copy & 0xff);
    SET_REG(obj->regs+MOTOR_TIMING1, reg);



    if (fh_sm_manual_4 == param->mode || fh_sm_manual_8 == param->mode)
    {
        SET_REG(obj->regs+MOTOR_MANUAL_CONFIG0, param->manual_pwm_choosenA);
        SET_REG(obj->regs+MOTOR_MANUAL_CONFIG1, param->manual_pwm_choosenB);
    }

#ifdef FH_STEPMOTOR_V1_1

    if (param->initparam.NeedUpdatePos)
    {
        reg = GET_REG(obj->regs + MOTOR_INIT);
        reg |= (param->initparam.InitStep)&0x7;
        reg |= ((param->initparam.InitMicroStep&0x3f)<<8);
        SET_REG(obj->regs + MOTOR_INIT, reg);
    }

#endif

    /* keep param */
#ifdef FH_STEPMOTOR_V1_2
    reg = GET_REG(obj->regs+MOTOR_KEEP);
    if (param->keepparam.KeepNumEn)
    {
        reg |= (param->keepparam.KeepNumEn)&0x1;
        reg &= (~(0xffff<<16));
        reg |= ((param->keepparam.KeepNum&0xffff)<<16);
    } else
        reg &= (~0x1);

    if (param->keepparam.KeepDampEn)
    {
        reg |= ((param->keepparam.KeepDampEn&0x1)<<1);
        reg &= (~(0xf<<4));
        reg |= ((param->keepparam.KeepDamp&0xf)<<4);
    } else
        reg &= (~(0x1<<1));

    SET_REG(obj->regs + MOTOR_KEEP, reg);
#endif
}

static void fh_stepmotor_get_hw_param(struct fh_sm_obj_t *obj, struct fh_sm_param *param)
{
    unsigned int reg;

    reg = GET_REG(obj->regs + MOTOR_MODE);

    param->mode = reg&0x3;
    param->direction = (reg>>4)&0x1;
    param->output_invert_A = (reg>>5)&0x1;
    param->output_invert_B = (reg>>6)&0x1;

#ifdef FH_STEPMOTOR_V1_2
    param->dampen = (reg>>13)&0x1;
    param->damp = (reg>>24)&0xf;
#endif
    param->timingparam.microstep = (reg>>8)&0xf;
#ifdef FH_STEPMOTOR_V1_1
    param->keep = (reg>>7)&0x1;
    param->initparam.NeedInit = (reg>>12)&0x1;
    param->initparam.InitRunNum = (reg>>16)&0xff;
    param->autostopparam.AutoStopEn = (reg>>2)&0x1;
    param->autostopparam.AutoStopSigPol = (reg>>3)&0x1;
#endif

    reg = GET_REG(obj->regs+MOTOR_TIMING0);
    param->timingparam.period = (reg>>16);
    param->timingparam.counter = reg&0xffff;

    reg = GET_REG(obj->regs + MOTOR_TIMING1);
    param->timingparam.copy = reg & 0xff;

    if (fh_sm_manual_4 == param->mode || fh_sm_manual_8 == param->mode)
    {
        param->manual_pwm_choosenA = GET_REG(obj->regs+MOTOR_MANUAL_CONFIG0);
        param->manual_pwm_choosenB = GET_REG(obj->regs+MOTOR_MANUAL_CONFIG1);
    }
#ifdef FH_STEPMOTOR_V1_1
    reg = GET_REG(obj->regs+MOTOR_INIT);
    param->initparam.InitStep = (reg)&0x7;
    param->initparam.InitMicroStep = (reg>>8)&0x3f;
    reg = GET_REG(obj->regs+MOTOR_DEBOUNCE);
    param->autostopparam.AutoStopSigDeb = (reg)&0x1;
    param->autostopparam.AutoStopSigDebSet = (reg>>2)&0xFFFFFF;
#endif
#ifdef FH_STEPMOTOR_V1_2
    reg = GET_REG(obj->regs+MOTOR_KEEP);
    param->keepparam.KeepNumEn = (reg)&0x1;
    param->keepparam.KeepDampEn = (reg>>1)&0x1;
    param->keepparam.KeepDamp = (reg>>4)&0xf;
    param->keepparam.KeepNum = (reg>>16)&0xffff;
#endif
}



static int fh_stepmotor_start_sync(struct fh_sm_obj_t *obj, int cycles)
{
    int fin = 0;
    unsigned int regcycle = 0;

    if (cycles <= 0)
        return 0;

    rt_completion_init(&obj->startsync_complete);

    cycles -= 1;
    regcycle = GET_REG(obj->regs + MOTOR_TIMING1);

#ifdef FH_STEPMOTOR_V1_1
    regcycle = regcycle & (0x000000ff);
    regcycle = regcycle|((cycles<<8)&0xffffff00);
#else
    regcycle = regcycle & (0x0000ffff);
    regcycle = regcycle|((cycles<<16)&0xffff0000);
#endif
    SET_REG(obj->regs+MOTOR_TIMING1, regcycle);

    SET_REG(obj->regs+MOTOR_CTRL, 1);
    rt_completion_wait(&obj->startsync_complete, RT_WAITING_FOREVER);
#ifdef FH_STEPMOTOR_V1_1
    fin = (GET_REG(obj->regs+MOTOR_STATUS0)>>4)&0x1ffffff;
#else
    fin = (GET_REG(obj->regs+MOTOR_STATUS0)>>8)&0x1ffff;
#endif
    return fin;

}

static void fh_stepmotor_start_async(struct fh_sm_obj_t *obj, int cycles)
{
    unsigned int regcycle = 0;

    if (cycles <= 0)
        return;

    cycles -= 1;
    regcycle = GET_REG(obj->regs+MOTOR_TIMING1);
#ifdef FH_STEPMOTOR_V1_1
    regcycle = regcycle & (0x000000ff);
    regcycle = regcycle|((cycles<<8)&0xffffff00);
#else
    regcycle = regcycle & (0x0000ffff);
    regcycle = regcycle|((cycles<<16)&0xffff0000);
#endif
    SET_REG(obj->regs+MOTOR_TIMING1, regcycle);

    SET_REG(obj->regs+MOTOR_CTRL, 1);

}

void fh_stepmotor_update_cycle(struct fh_sm_obj_t *obj, int cycles)
{
    unsigned int regcycle = 0;

    if (cycles <= 0)
        return;
    cycles -= 1;
    regcycle = GET_REG(obj->regs+MOTOR_TIMING1);
    regcycle = regcycle & (0x000000ff);
    regcycle = regcycle|((cycles<<8)&0xffffff00);
    SET_REG(obj->regs+MOTOR_TIMING1, regcycle);
}

static int fh_stepmotor_stop(struct fh_sm_obj_t *obj)
{
    int fin = 0;

    rt_completion_init(&obj->stop_complete);
    if (fh_stepmotor_is_busy(obj))
    {
        SET_REG(obj->regs+MOTOR_CTRL, 0);
        rt_completion_wait(&obj->stop_complete, RT_WAITING_FOREVER);
    }
#ifdef FH_STEPMOTOR_V1_1
    fin = (GET_REG(obj->regs+MOTOR_STATUS0)>>4)&0x1ffffff;
#else
    fin = (GET_REG(obj->regs+MOTOR_STATUS0)>>8)&0x1ffff;
#endif
    return fin;
}
static int fh_stepmotor_get_current_cycle(struct fh_sm_obj_t *obj)
{
#ifdef FH_STEPMOTOR_V1_1
    int fin = (GET_REG(obj->regs+MOTOR_STATUS0)>>4)&0x1ffffff;
#else
    int fin = (GET_REG(obj->regs+MOTOR_STATUS0)>>8)&0x1ffff;
#endif
    return fin;
}


static int fh_stepmotor_set_lut(struct fh_sm_obj_t *obj, struct fh_sm_lut *lut)
{
    int i = 0;

    for (i = 0; i < lut->lutsize/sizeof(rt_uint32_t); i++)
        SET_REG(obj->regs+MOTOR_MEM+i*4, lut->lut[i]);

    return 0;
}
#if 0
static int fh_stepmotor_get_lutsize(struct fh_sm_obj_t *obj)
{
    return 256;
}
#endif
static int fh_stepmotor_get_lut(struct fh_sm_obj_t *obj, struct fh_sm_lut *lut)
{
    int i;

    for (i = 0; i < lut->lutsize/sizeof(rt_uint32_t); i++)
        lut->lut[i] = GET_REG(obj->regs+MOTOR_MEM+i*4);


    return 0;
}





static long fh_stepmotor_ioctl(rt_device_t dev, int cmd, void *arg)
{
    int ret = 0;
    struct fh_sm_param *param;
    struct fh_sm_lut *lut;
    struct fh_sm_obj_t *curobj;

    if (arg == NULL)
        return -RT_EINVAL;

    curobj = (struct fh_sm_obj_t *)dev->user_data;
    if (curobj == NULL)
        return -RT_EINVAL;

    switch (cmd)
    {
    case FH_SM_SET_PARAM:
    {
        if (fh_stepmotor_is_busy(curobj))
        {
            ret = -RT_EINVAL;
            break;
        }
        param = (struct fh_sm_param *)arg;
        fh_stepmotor_set_hw_param(curobj, param);
        ret  = 0;
        break;
    }
    case FH_SM_GET_PARAM:
    {
        param = rt_malloc(sizeof(struct fh_sm_param));
        if (param == NULL)
        {
            ret = -RT_EINVAL;
            break;
        }
        fh_stepmotor_get_hw_param(curobj, param);
        rt_memcpy(arg, (void *)param, sizeof(struct fh_sm_param));
        rt_free(param);
        ret  = 0;
        break;
    }

    case FH_SM_START_ASYNC:
    {
        int cycles = *(int *)arg;

        if (fh_stepmotor_is_busy(curobj))
        {
            ret = -RT_EINVAL;
            break;
        }
        fh_stepmotor_start_async(curobj, cycles);
        ret  = 0;
        break;
    }
    case FH_SM_START_SYNC:
    {
        int cycles = *(int *)arg;
        int fin = 0;

        if (fh_stepmotor_is_busy(curobj))
        {
            ret = -RT_EINVAL;
            break;
        }

        fin = fh_stepmotor_start_sync(curobj, cycles);
        rt_memcpy((void *)arg, (void *)&fin, sizeof(fin));
        ret  = 0;
        break;
    }
    case FH_SM_UPDATE_CYCLE:
    {
#ifdef FH_STEPMOTOR_V1_2
        int cycles = *(int *)arg;

        fh_stepmotor_update_cycle(curobj, cycles);
        ret  = 0;
#else
        rt_kprintf("Ip not support this function!!!\n");
        ret = -ENOENT;
#endif
        break;
    }
    case FH_SM_STOP:
    {
        int fincnt = 0;

        fincnt = fh_stepmotor_stop(curobj);
        rt_memcpy((void  *)arg, (void *)&fincnt, sizeof(fincnt));
        ret  = 0;
        break;
    }
    case FH_SM_CHECKISTOP:
    {
       int stop = fh_stepmotor_is_busy(curobj) ? 0 : 1;
       rt_memcpy((void  *)arg, (void *)&stop, sizeof(stop));
       ret  = 0;
       break;
    }
    case FH_SM_GET_CUR_CYCLE:
    {
        int fincnt = 0;

        fincnt = fh_stepmotor_get_current_cycle(curobj);
        rt_memcpy((void *)arg, (void *)&fincnt, sizeof(fincnt));
        ret  = 0;
        break;
    }

    case FH_SM_GET_LUT:
    {

        fh_stepmotor_get_lut(curobj, arg);
        ret  = 0;
        break;
    }
    case FH_SM_SET_LUT:
    {
        if (fh_stepmotor_is_busy(curobj))
        {
            ret = -RT_EINVAL;
            break;
        }
        lut = (struct fh_sm_lut *)arg;
        if (lut == NULL)
        {
            ret = -RT_EINVAL;
            break;
        }
        ret = fh_stepmotor_set_lut(curobj, lut);
        ret  = 0;
        break;
    }
    default:
      ret = -RT_EINVAL;
      break;
    }

    return ret;
}





rt_device_t fh_stepmotor_device[MAX_FHSM_NR];




const rt_uint32_t fhsm_deflut[] = {
0x00000080, 0x0003007f, 0x0006007f, 0x0009007f, 0x000c007f, 0x000f007f, 0x0012007e, 0x0015007e,
0x0018007d, 0x001c007c, 0x001f007c, 0x0022007b, 0x0025007a, 0x00280079, 0x002b0078, 0x002e0077,
0x00300076, 0x00330075, 0x00360073, 0x00390072, 0x003c0070, 0x003f006f, 0x0041006d, 0x0044006c,
0x0047006a, 0x00490068, 0x004c0066, 0x004e0064, 0x00510062, 0x00530060, 0x0055005e, 0x0058005c,
0x005a005a, 0x005c0058, 0x005e0055, 0x00600053, 0x00620051, 0x0064004e, 0x0066004c, 0x00680049,
0x006a0047, 0x006c0044, 0x006d0041, 0x006f003f, 0x0070003c, 0x00720039, 0x00730036, 0x00750033,
0x00760030, 0x0077002e, 0x0078002b, 0x00790028, 0x007a0025, 0x007b0022, 0x007c001f, 0x007c001c,
0x007d0018, 0x007e0015, 0x007e0012, 0x007f000f, 0x007f000c, 0x007f0009, 0x007f0006, 0x007f0003,
};


static void fh_stepmotor_init_hw_param(struct fh_sm_obj_t *obj)
{
    int i = 0;

    SET_REG(obj->regs+MOTOR_MODE, 0);
    SET_REG(obj->regs+MOTOR_TIMING0, 0x800000); /* clk div 128*1 */
    SET_REG(obj->regs+MOTOR_TIMING1, 0x10010);
    SET_REG(obj->regs+MOTOR_MANUAL_CONFIG0, 0);
    SET_REG(obj->regs+MOTOR_MANUAL_CONFIG1, 0);
    SET_REG(obj->regs+MOTOR_INT_EN, 1);

    for (i = 0; i < 64; i++)
        SET_REG(obj->regs+MOTOR_MEM+i*4, fhsm_deflut[i]);

}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops smt_ops = {
    .control = fh_stepmotor_ioctl
};
#endif

static int fh_stepmotor_probe(void *priv_data)
{
    struct clk *sm_clk;
    int id = 0;
    static char devstr[MAX_FHSM_NR][40];
    char clkname[24] = {0};

    struct fh_smt_obj *pdev = (struct fh_smt_obj *)priv_data;

    id = pdev->id;
    fh_sm_obj[id] = rt_malloc(sizeof(struct fh_sm_obj_t));
    if (fh_sm_obj[id] == NULL)
        return -RT_EINVAL;

    fh_sm_obj[id]->irq_no = pdev->irq;
    fh_sm_obj[id]->id = id;
    fh_sm_obj[id]->regs = (void *)pdev->base;

    rt_sprintf(clkname,"stm%d_clk", id);
    rt_sprintf(fh_sm_obj[id]->isrname, "stm-%d", id);

    sm_clk = clk_get(NULL, clkname);
    if (sm_clk != NULL)
        clk_enable(sm_clk);

    rt_completion_init(&fh_sm_obj[id]->startsync_complete);
    rt_completion_init(&fh_sm_obj[id]->stop_complete);

    fh_stepmotor_init_hw_param(fh_sm_obj[id]);

    fh_stepmotor_device[id] = rt_malloc(sizeof(struct rt_device));
    rt_memset(fh_stepmotor_device[id], 0, sizeof(struct rt_device));
#ifdef RT_USING_DEVICE_OPS
    fh_stepmotor_device[id]->ops       = &smt_ops;
#else
    fh_stepmotor_device[id]->control   = fh_stepmotor_ioctl;
#endif
    fh_stepmotor_device[id]->type      = RT_Device_Class_Miscellaneous;
    fh_stepmotor_device[id]->user_data = fh_sm_obj[id];

    rt_memset(devstr[id], 0, 40);
    rt_sprintf(devstr[id], FH_SM_MISC_DEVICE_NAME, id);

    rt_hw_interrupt_install(fh_sm_obj[id]->irq_no, fh_stepmotor_isr,
                                (void *)fh_sm_obj[id], fh_sm_obj[id]->isrname);
    rt_hw_interrupt_umask(fh_sm_obj[id]->irq_no);


    rt_device_register(fh_stepmotor_device[id], devstr[id], RT_DEVICE_FLAG_RDWR);


    return 0;

}


int fh_stepmotor_exit(void *priv_data) { return 0; }
struct fh_board_ops fh_smt_driver_ops = {
    .probe = fh_stepmotor_probe, .exit = fh_stepmotor_exit,
};

void rt_hw_stepmotor_init(void)
{
    fh_board_driver_register(FH_SM_PLAT_DEVICE_NAME, &fh_smt_driver_ops);
}


