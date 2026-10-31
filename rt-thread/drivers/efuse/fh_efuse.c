/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#include "fh_efuse.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

#ifdef EFUSE_DEBUG
#define EFUSE_DBG(fmt, args...)     \
        do                                    \
        {                                    \
            rt_kprintf("[EFUSE]: ");     \
            rt_kprintf(fmt, ## args);        \
        }                                    \
        while (0)
#else
#define EFUSE_DBG(fmt, args...)  do { } while (0)
#endif

#define EFUSE_ERR(fmt, args...)     \
        do                                    \
        {                                    \
            rt_kprintf("[EFUSE ERROR]: ");   \
            rt_kprintf(fmt, ## args);        \
        }                                    \
        while (0)

#define EFUSE_WARN(fmt, args...)     \
        do                                    \
        {                                    \
            rt_kprintf("[EFUSE WARNING]: ");     \
            rt_kprintf(fmt, ## args);        \
        }                                    \
        while (0)

#define EFUSE_ASSERT(expr)    \
        do                    \
        {                     \
            if (!(expr))      \
            {                 \
                rt_kprintf("Assertion failed! %s:line %d\n", \
                __func__, __LINE__); \
                while (1)     \
                {}            \
            }                 \
        } while (0)


#define efuse_cmd              (0x0)
#define efuse_config           (0x4)
#define efuse_match_key        (0x8)
#define efuse_timing0          (0xc)
#define efuse_timing1          (0x10)
#define efuse_timing2          (0x14)
#define efuse_timing3          (0x18)
#define efuse_timing4          (0x1c)
#define efuse_timing5          (0x20)
#define efuse_timing6          (0x24)
#define efuse_dout             (0x28)
#define efuse_status0          (0x2c)
#define efuse_status1          (0x30)
#define efuse_status2          (0x34)
#define efuse_status3          (0x38)
#define efuse_status4          (0x3c)
#define efuse_mem_info         (0x40)

#define EFUSE_TIMEOUT           100000

FH_EFUSE_DEV_P g_efuse = NULL;
#define EFUSE_REG32(addr) (*(volatile unsigned int *)(g_efuse->efuse_reg + addr))

enum {
    CMD_LOAD_USERCMD = 1,
    CMD_TRANS_AESKEY = 4,
    CMD_WFLAG_AUTO = 8,
};









void efuse_biglittle_swap(rt_uint8_t *buf, rt_uint32_t size)
{
    rt_uint8_t tmp, tmp1;
    rt_uint32_t i, pos;

    for (i = 0; i < size >> 2; i++)
    {
        pos = i << 2;

        tmp = buf[pos + 0];
        tmp1 = buf[pos + 1];
        buf[pos + 0] = buf[pos + 3];
        buf[pos + 1] = buf[pos + 2];
        buf[pos + 2] = tmp1;
        buf[pos + 3] = tmp;
    }
}

rt_int32_t efuse_power_on(void)
{
    rt_uint32_t data;

    data = EFUSE_REG32(efuse_config);
    data |= 1 << 27;
    EFUSE_REG32(efuse_config) = data;

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_power_off(void)
{
    rt_uint32_t data;

    data = EFUSE_REG32(efuse_config);
    data &= ~(1 << 27);
    EFUSE_REG32(efuse_config) = data;

    return EFUSE_SUCCESS;
}

static rt_int32_t efuse_detect_complete(rt_uint32_t pos)
{
    rt_uint32_t rdata;
    rt_uint32_t time = 0;

    do
    {
        time++;
        rdata = EFUSE_REG32(efuse_status0);
        if (time > EFUSE_TIMEOUT)
        {
            EFUSE_ERR("detect time out...pos: 0x%x\n", pos);
            return -1;
        }
    } while ((rdata & (1 << pos)) != 1 << pos);

    return 0;
}

rt_int32_t efuse_refresh(void)
{
    EFUSE_REG32(efuse_cmd) = CMD_WFLAG_AUTO;
    if (efuse_detect_complete(8))
        return EFUSE_WAIT_CMD_WFLAG_AUTO_ERROR;

    EFUSE_REG32(efuse_cmd) = CMD_LOAD_USERCMD;
    if (efuse_detect_complete(1))
        return EFUSE_WAIT_CMD_LOAD_USERCMD_ERROR;

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_check_pro_bits(rt_uint32_t *buff)
{
    buff[0] = EFUSE_REG32(efuse_status1);
    buff[1] = EFUSE_REG32(efuse_status2);

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_get_lock_status(struct efuse_status *status)
{
    status->efuse_apb_lock = (EFUSE_REG32(efuse_status0) >> 20) & 0x0f;
    status->aes_ahb_lock = (EFUSE_REG32(efuse_status0) >> 24) & 0x0f;

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_check_err_bits(rt_uint32_t *buff)
{
    /* first set auto check cmd */
    rt_uint32_t data;
    /* efuse_load_usrcmd(obj); */
    data = EFUSE_REG32(efuse_status3);
    *buff = data;

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_read_entry(rt_uint32_t key, rt_uint32_t start_entry, rt_uint8_t *buff,
        rt_uint32_t size)
{
    rt_uint32_t data, i;

    for (i = 0; i < size; i++)
    {
        EFUSE_REG32(efuse_match_key) = key;
        EFUSE_REG32(efuse_cmd) = ((start_entry + i) << 4) + 0x03;
        if (efuse_detect_complete(3))
            return EFUSE_WAIT_CMD_READ_ERROR;
        data = EFUSE_REG32(efuse_dout);
        *buff++ = (rt_uint8_t) data;
    }

    return EFUSE_SUCCESS;
}

static rt_int32_t efuse_write_key_byte(rt_uint32_t entry, rt_uint8_t data)
{
    rt_uint32_t temp = 0;

    temp = (rt_uint32_t) data;
    /* 0~255 */
    temp &= ~0xffffff00;
    temp <<= 12;
    /* 0~63 */
    entry &= 0x3f;
    temp |= (entry << 4) | (0x02);

    EFUSE_REG32(efuse_cmd) = temp;
    if (efuse_detect_complete(2))
        return EFUSE_WAIT_CMD_WRITE_ERROR;

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_write_entry(rt_uint32_t start_entry, rt_uint8_t *buff, rt_uint32_t size)
{
    rt_uint32_t i;
    rt_uint32_t temp;
    rt_uint32_t entry;

    for (i = 0; i < size; i++)
    {
        temp = (rt_uint32_t) buff[i];
        entry = start_entry + i;
        /* 0~255 */
        temp &= ~0xffffff00;
        temp <<= 12;
        /* 0~63 */
        entry &= 0x3f;
        temp |= (entry << 4) | (0x02);

        EFUSE_REG32(efuse_cmd) = temp;
        if (efuse_detect_complete(2))
            return EFUSE_WAIT_CMD_WRITE_ERROR;
    }

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_set_lock_data(EFUSE_INFO *efuse_info)
{
    rt_uint8_t temp_data;

    /* parse lock data... */
    temp_data = efuse_info->status.aes_ahb_lock;
    temp_data <<= 4;
    temp_data &= 0xf0;
    efuse_info->status.efuse_apb_lock &= 0x0f;
    temp_data |= efuse_info->status.efuse_apb_lock;

    return efuse_write_key_byte(63, temp_data);
}

#include "fh_cesa.h"
typedef struct _EFUSE_TRANS_PARA_S
{
    rt_uint32_t mode;
    rt_uint32_t map_size;
    struct {
        rt_uint32_t crypto_key_no;
        rt_uint32_t ex_mem_entry;
    } map[MAX_EX_KEY_MAP_SIZE];
} EFUSE_TRANS_PARA_S;

void efuse_check_map_para(rt_uint32_t size, rt_uint32_t map[][2])
{
    rt_uint32_t loop;

    EFUSE_ASSERT(size <= MAX_EX_KEY_MAP_SIZE);
    for (loop = 0; loop < size; loop++)
    {
        if ((map[loop][1] % 4 != 0)
                || (map[loop][0] != loop))
        {
            EFUSE_ERR("map[%d]:entry[0x%x]:aes key[0x%x] para error..\n", loop,
                    map[loop][1],
                    map[loop][0]);
            EFUSE_ASSERT(0);
        }
    }
}

void efuse_trans_key(void *efuse_para)
{
    int i;
    rt_uint32_t key_map_size;
    rt_uint32_t temp_reg;

    EFUSE_TRANS_PARA_S efuse_trans_para;

    rt_memcpy(&efuse_trans_para, efuse_para, sizeof(efuse_trans_para));

    /* chip need set the map...and usr set ~~~ */
    if (efuse_trans_para.mode & CRYPTO_EX_MEM_SWITCH_KEY)
    {
        /* parse efuse map.. */
        if (efuse_trans_para.mode & CRYPTO_EX_MEM_4_ENTRY_1_KEY)
        {
            EFUSE_DBG("parse efuse map...map size: %d\n",efuse_trans_para.map_size);
            key_map_size = efuse_trans_para.map_size;
            /* check map buf data... */
            efuse_check_map_para(key_map_size, (rt_uint32_t (*)[2])efuse_trans_para.map);
            for (i = 0; i < key_map_size; i++)
            {
                efuse_refresh();
                temp_reg = EFUSE_REG32(efuse_config);
                temp_reg &= ~(0xf <<28);
                temp_reg |= (efuse_trans_para.map[i].ex_mem_entry / 4) << 28;
                EFUSE_REG32(efuse_config) = temp_reg;
                EFUSE_REG32(efuse_cmd) = (i << 20) + 0x04;
                efuse_detect_complete(4);
            }
        }
    }
    else
    {
        /* chip need set the map...and usr not set ~~~ */
        EFUSE_DBG("usr not set efuse map...key size:%d\n", efuse_trans_para.map_size);
        key_map_size = efuse_trans_para.map_size;
        for (i = 0; i < key_map_size / 4; i++)
        {
            efuse_refresh();
            temp_reg = EFUSE_REG32(efuse_config);
            temp_reg &= ~(0xf <<28);
            temp_reg |= i << 28;
            EFUSE_REG32(efuse_config) = temp_reg;
            EFUSE_REG32(efuse_cmd) = (i << 20) + 0x04;
            efuse_detect_complete(4);
        }
    }
}



































static rt_err_t  rt_dev_efuse_init(rt_device_t dev)
{
    struct clk *clk_efuse, *pclk_efuse;

    g_efuse = (FH_EFUSE_DEV_P)dev;

    clk_efuse = (struct clk *)clk_get(NULL, "efuse_clk");
    if (clk_efuse)
    {
        clk_set_rate(clk_efuse, 25000000);
        clk_enable(clk_efuse);
    }
    pclk_efuse = (struct clk *)clk_get(NULL, "efuse_pclk");
    if (pclk_efuse)
    {
        clk_enable(pclk_efuse);
    }

    efuse_power_on();
    g_efuse->clk = clk_efuse;
    g_efuse->pclk = pclk_efuse;

    return RT_EOK;
}

static rt_err_t rt_dev_efuse_open(struct rt_device *dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_dev_efuse_close(struct rt_device *dev)
{
    return RT_EOK;
}

static rt_err_t rt_dev_efuse_control(struct rt_device *dev,
        int cmd,
        void *args)
{
    EFUSE_INFO *efuse_info = (EFUSE_INFO *)args;
    rt_uint8_t temp_buff[32] = { 0 };
    rt_int32_t ret = EFUSE_SUCCESS;


    ret = efuse_refresh();
    if (ret)
        return ret;


    switch (cmd)
    {
    /* efuse ioctl */
    case IOCTL_EFUSE_CHECK_PRO:
        ret = efuse_check_pro_bits(efuse_info->status.protect_bits);
        break;
    case IOCTL_EFUSE_WRITE_KEY:
        rt_memcpy(temp_buff, efuse_info->key_buff, efuse_info->key_size);
        efuse_biglittle_swap(temp_buff, efuse_info->key_size);
        ret = efuse_write_entry(efuse_info->efuse_entry_no,
                                temp_buff,
                                efuse_info->key_size);
        break;
    case IOCTL_EFUSE_CHECK_LOCK:
        ret = efuse_get_lock_status(&efuse_info->status);
        break;
    case IOCTL_EFUSE_TRANS_KEY:
        EFUSE_DBG("please use efuse transkey with cesa...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_CHECK_ERROR:
        ret = efuse_check_err_bits(&efuse_info->status.error);
        break;
    case IOCTL_EFUSE_READ_KEY:
        ret = efuse_read_entry(efuse_info->status.error,
                               efuse_info->efuse_entry_no,
                               temp_buff,
                               efuse_info->key_size);
        if (ret == EFUSE_SUCCESS)
        {
            efuse_biglittle_swap(temp_buff, efuse_info->key_size);
            rt_memcpy(efuse_info->key_buff, temp_buff, efuse_info->key_size);
        }
        break;
    case IOCTL_EFUSE_SET_LOCK_DATA:
        ret = efuse_set_lock_data(efuse_info);
        break;
    case IOCTL_EFUSE_GET_LOCK_DATA:
        EFUSE_DBG("not support get lock data ...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_SET_MAP_PARA_4_TO_1:
        EFUSE_DBG("please use with aes...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_SET_MAP_PARA_1_TO_1:
        EFUSE_DBG("not support this func now..\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_CLR_MAP_PARA:
        EFUSE_DBG("not support here...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_DUMP_REGISTER:
        EFUSE_DBG("not support here...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_DEBUG:
        EFUSE_DBG("not support here...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
    case IOCTL_EFUSE_WRITE_ENTRY:
        ret = efuse_write_entry(efuse_info->efuse_entry_no,
                                efuse_info->key_buff,
                                efuse_info->key_size);
        break;
    case IOCTL_EFUSE_READ_ENTRY:
        ret = efuse_read_entry(efuse_info->status.error,
                               efuse_info->efuse_entry_no,
                               efuse_info->key_buff,
                               efuse_info->key_size);
        break;
    default:
        return EFUSE_IOCTL_CMD_NOT_SUPPORT;
    }

    return ret;
}

#ifdef RT_USING_PM
int fh_efuse_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    efuse_power_off();
    clk_disable(g_efuse->clk);
    clk_disable(g_efuse->pclk);

    return 0;
}

void fh_efuse_resume(const struct rt_device *device, rt_uint8_t mode)
{
    rt_dev_efuse_init(device);
}

struct rt_device_pm_ops fh_efuse_pm_ops = {
    .suspend_prepare = fh_efuse_suspend,
    .resume_prepare = fh_efuse_resume,
};
#endif

int fh_efuse_probe(void *priv_data)
{
    struct fh_efuse_platform_data *plat_data =
        (struct fh_efuse_platform_data *) priv_data;
    FH_EFUSE_DEV_P p_efuse;

    p_efuse = rt_malloc(sizeof(FH_EFUSE_DEV_S));
    if (p_efuse)
        p_efuse->efuse_reg = plat_data->base;
    else
        return RT_ERROR;

    p_efuse->parent.type = RT_Device_Class_Miscellaneous;
    p_efuse->parent.init = rt_dev_efuse_init;
    p_efuse->parent.open = rt_dev_efuse_open;
    p_efuse->parent.close = rt_dev_efuse_close;
    p_efuse->parent.control = rt_dev_efuse_control;
    rt_device_register((rt_device_t)p_efuse, "efuse", 0);

    rt_device_init((rt_device_t)p_efuse);

#ifdef RT_USING_PM
    rt_pm_device_register((rt_device_t)p_efuse, &fh_efuse_pm_ops);
#endif

    return RT_EOK;
}

int fh_efuse_exit(void *priv_data)
{
    if (g_efuse == NULL)
        return RT_ERROR;

    rt_device_unregister((rt_device_t)g_efuse);
    rt_free(g_efuse);
    g_efuse = NULL;

    return RT_EOK;
}


struct fh_board_ops efuse_driver_ops = {
    .probe = fh_efuse_probe,
    .exit = fh_efuse_exit,
};

void rt_efuse_init(void)
{
    fh_board_driver_register("fh_efuse", &efuse_driver_ops);
}

