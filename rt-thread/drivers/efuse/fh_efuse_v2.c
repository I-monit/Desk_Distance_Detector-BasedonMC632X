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
#define efuse_write_val        (0x40)
#define efuse_status6          (0x44)
#define efuse_status7          (0x48)
#define efuse_status8          (0x4c)



FH_EFUSE_DEV_P g_efuse = NULL;
#define EFUSE_REG32(addr) (*(volatile unsigned int *)(g_efuse->efuse_reg + addr))

enum {
    CMD_WFLGA = 0,
    CMD_LOAD_USERCMD = 1,
    CMD_WRITE = 2,
    CMD_READ = 3,
    CMD_TRANS_AESKEY = 4,
    CMD_WFLAG_AUTO = 8,
    CMD_CMP = 15,
};

/* cmd bit mask*/
#define EFUSE_CMD_OPC_POS       0
#define EFUSE_CMD_ADDR_POS      4
#define EFUSE_CMD_BIT_WRFLAG_POS     12
#define EFUSE_CMD_AES_KEYID_POS      20

#define EFUSE_VALIDITY_OK   0
#define EFUSE_VALIDITY_ERR   1

#define ENTRY_ALLIGN        1
#define ENTRY_NOT_ALLIGN        0





/* Molchip efuse ctl entry map (16 block version)*/
/****************************************/
/*NAME       ENTREY         ENTRY_NO_2_BLK      COMMENT

/*NORMAL     (00 ~ 47)      BLK00 ~ BLK11       USR AREA    total 12 BLK*/
/*TEST       (48 ~ 58)      BLK12 ~ BLK14       CAN'T LK    BLK12-ENTRY51-BONDING AREA*/
/*RESERVE    (59)           BLK14                                                                                  */
/*LK_R_APB   (60)           BLK15               BIT[0] : BLK[0-3] ..BIT[2] : BLK[8-11], {else :: BIT[3 - 7] : REV}*/
/*LK_W_EFUSE (61 ~ 62)      BLK15               BIT[0] : BLK[0] ... BIT[11] : BLK[11], {else :: BIT[12 - 15] : REV }*/
/*LK_R_AHB   (63)           BLK15               BIT[0] : KEY[0-1] ..BIT[3] : KEY[6 : 7], {else :: BIT[4 - 7] : REV}*/
/****************************************/







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
        if (time > 100000)
        {
            EFUSE_ERR("detect time out...pos: 0x%x\n", pos);
            return -1;
        }
    } while ((rdata & (1 << pos)) != 1 << pos);

    return 0;
}

static rt_int32_t efuse_write_pre_check(rt_uint32_t entry)
{
    rt_uint32_t blk_no;
    rt_uint32_t data;

    blk_no = entry / 4;
    data =  (CMD_WFLGA << EFUSE_CMD_OPC_POS) | (blk_no << EFUSE_CMD_ADDR_POS);
    EFUSE_REG32(efuse_cmd) = data;
    return efuse_detect_complete(CMD_WFLGA);
}

static void efuse_write_key_bit_to_1(rt_uint32_t entry, rt_uint8_t pos)
{
    rt_uint32_t blk_no;
    rt_uint32_t data;
 
    blk_no = entry / 4;
    efuse_write_pre_check(entry);

    EFUSE_REG32(efuse_write_val) = pos + (8 * (entry % 4));
    data = (CMD_WRITE << EFUSE_CMD_OPC_POS) |
    	(blk_no << EFUSE_CMD_ADDR_POS) |
    	(0xff << EFUSE_CMD_BIT_WRFLAG_POS);
    EFUSE_REG32(efuse_cmd) = data;
    efuse_detect_complete(CMD_WRITE);
}

static void efuse_write_key_byte(rt_uint32_t entry, rt_uint8_t data)
{
	rt_uint32_t i;

    for(i = 0; i < 8; i++){
        if(data & (1 << i))
            efuse_write_key_bit_to_1(entry, i);
    }
}

static void efuse_write_key_word(rt_uint32_t entry, rt_uint32_t wdata)
{
    rt_uint32_t blk_no;
    rt_uint32_t data;

    blk_no = entry / 4;
    efuse_write_pre_check(entry);

    EFUSE_REG32(efuse_write_val) = wdata;
    data = (CMD_WRITE << EFUSE_CMD_OPC_POS) |
        (blk_no << EFUSE_CMD_ADDR_POS);
    EFUSE_REG32(efuse_cmd) = data;
    efuse_detect_complete(CMD_WRITE);
}

static rt_uint32_t efuse_read_entry_word(rt_uint32_t entry_no)
{
    rt_uint32_t blk_no;
    rt_uint32_t data;

    blk_no = entry_no / 4;
    data = (CMD_READ << EFUSE_CMD_OPC_POS) | (blk_no << EFUSE_CMD_ADDR_POS);
    EFUSE_REG32(efuse_cmd) = data;
    efuse_detect_complete(CMD_READ);

    return EFUSE_REG32(efuse_status4);
}

static rt_uint8_t efuse_read_entry_byte(rt_uint32_t entry_no)
{
    rt_uint32_t blk_no;
    rt_uint32_t data;

    blk_no = entry_no / 4;
    data = (CMD_READ << EFUSE_CMD_OPC_POS) | (blk_no << EFUSE_CMD_ADDR_POS);
    EFUSE_REG32(efuse_cmd) = data;
    efuse_detect_complete(CMD_READ);
    data = EFUSE_REG32(efuse_status4);
    data = data >> (8 * (entry_no % 4));

    return (rt_uint8_t)data;
}

static rt_int32_t efuse_check_write_validity(void)
{
    rt_uint32_t ret; 

    ret = EFUSE_REG32(efuse_status3);
    if(ret & (1 << 7))
        return EFUSE_VALIDITY_ERR;

    return  EFUSE_VALIDITY_OK;
}

static rt_int32_t efuse_check_read_validity(void)
{
    rt_uint32_t ret; 

    ret = EFUSE_REG32(efuse_status3);
    if(ret & (1 << 8))
        return EFUSE_VALIDITY_ERR;

    return  EFUSE_VALIDITY_OK;
}

static rt_int32_t check_entry_4byte_allign(rt_uint32_t start_entry, rt_uint32_t size)
{
    if((start_entry % 4 == 0) && (size % 4 == 0))
        return ENTRY_ALLIGN;
    else
        return ENTRY_NOT_ALLIGN;
}

rt_int32_t efuse_write_entry(rt_uint32_t start_entry, rt_uint8_t *buff, rt_uint32_t size)
{
	rt_uint32_t data, i;

    if (efuse_check_write_validity() != EFUSE_VALIDITY_OK) {
        EFUSE_ERR("efuse hw not allow to write efuse.\n");
        return -1001;
    }

    if (check_entry_4byte_allign(start_entry, size)) {
        /* word process */
        for (i = 0; i < size; i += 4) {
            memcpy(&data, buff, 4);
            efuse_write_key_word(start_entry + i, data);
            buff += 4;
        }
    } else {
        /* byte process */
        for (i = 0; i < size; i++) {
            efuse_write_key_byte(start_entry + i, *buff++);
        }
    }

	return EFUSE_SUCCESS;
}

rt_int32_t efuse_read_entry(rt_uint32_t key, rt_uint32_t start_entry, rt_uint8_t *buff,
        rt_uint32_t size)
{
	rt_uint32_t data, i;

    if (efuse_check_read_validity() != EFUSE_VALIDITY_OK) {
        EFUSE_ERR("efuse hw not allow to read efuse.\n");
        return -1002;
    }

    EFUSE_REG32(efuse_match_key) = key;
    if (check_entry_4byte_allign(start_entry, size)) {
        /* word process */
        for (i = 0; i < size; i += 4) {
            data = efuse_read_entry_word(start_entry + i);
            memcpy(buff, &data, 4);
            buff += 4;
        }

    } else {
        /* byte process */
        for (i = 0; i < size; i++) {
            data = efuse_read_entry_byte(start_entry + i);
            *buff++ = (rt_uint8_t) data;
        }
    }

	return EFUSE_SUCCESS;
}

rt_int32_t efuse_check_pro_bits(rt_uint32_t *buff)
{
	/*
     * [bit0 : bit15]: block0 ~ block15
     */
    buff[0] = EFUSE_REG32(efuse_status1);
    buff[0] &= 0xFFFF;
    buff[1] = 0;

    return EFUSE_SUCCESS;
}

rt_int32_t efuse_get_lock_status(struct efuse_status *status)
{
    /*
     * bit0: block0 ~ block3 locked
     * bit1: block4 ~ block7 locked
     * bit2: block8 ~ block11 locked
     */
    status->efuse_apb_lock = EFUSE_REG32(efuse_status6) & 0x7;
    /*
     * bit24: aes key0 & key1 locked
     * bit25: aes key2 & key3 locked
     * bit26: aes key4 & key5 locked
     * bit27: aes key6 & key7 locked
     */
    status->aes_ahb_lock = (EFUSE_REG32(efuse_status0) >> 24) & 0xf;

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

rt_int32_t efuse_set_read_lock(EFUSE_INFO *efuse_info)
{
    rt_uint32_t data;

    /*
     * bit0: lock aes key0 & key1
     * bit1: lock aes key2 & key3
     * bit2: lock aes key4 & key5
     * bit3: lock aes key6 & key7
     */
    data = efuse_info->status.aes_ahb_lock;
    data &= 0x0f;
    efuse_write_key_byte(63, (rt_uint8_t)data);

    /*
     * bit0: lock block0 ~ block3
     * bit1: lock block4 ~ block7
     * bit2: lock block8 ~ block11
     */
    data = efuse_info->status.efuse_apb_lock;
    data &= 0x7;
    efuse_write_key_byte(60, (rt_uint8_t)data);

	return EFUSE_SUCCESS;
}

rt_int32_t efuse_set_write_lock(EFUSE_INFO *efuse_info)
{
	rt_uint8_t write_lock_data[2] = {0};

    memcpy(write_lock_data, efuse_info->status.efuse_write_lock, sizeof(write_lock_data));

	/* write_lock_data[0]~[1]: total 8+4=12 max@blk11 */
	write_lock_data[1] &= 0x0f;
    efuse_write_entry(61, write_lock_data, sizeof(write_lock_data));

	return EFUSE_SUCCESS;
}

rt_int32_t efuse_refresh(void)
{
    EFUSE_REG32(efuse_cmd) = CMD_WFLAG_AUTO;
    if (efuse_detect_complete(CMD_WFLAG_AUTO))
        return EFUSE_WAIT_CMD_WFLAG_AUTO_ERROR;

    EFUSE_REG32(efuse_cmd) = CMD_LOAD_USERCMD;
    if (efuse_detect_complete(CMD_LOAD_USERCMD))
        return EFUSE_WAIT_CMD_LOAD_USERCMD_ERROR;

    return EFUSE_SUCCESS;
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
                temp_reg &= ~(0xff << 24);
                temp_reg |= (efuse_trans_para.map[i].ex_mem_entry / 4) << 24;
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
            temp_reg &= ~(0xff << 24);
            temp_reg |= i << 24;
            EFUSE_REG32(efuse_config) = temp_reg;
            EFUSE_REG32(efuse_cmd) = (i << 20) + 0x04;
            efuse_detect_complete(4);
        }
    }
}



































static rt_err_t  rt_dev_efuse_init(rt_device_t dev)
{
    struct clk *clk_efuse;

    g_efuse = (FH_EFUSE_DEV_P)dev;

    clk_efuse = (struct clk *)clk_get(NULL, "efuse_clock");
    if (clk_efuse)
    {
        clk_set_rate(clk_efuse, 25000000);
        clk_enable(clk_efuse);
    }
    efuse_power_on();
    g_efuse->clk = clk_efuse;

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
    case IOCTL_EFUSE_WRITE_KEY:
        rt_memcpy(temp_buff, efuse_info->key_buff, efuse_info->key_size);
        efuse_biglittle_swap(temp_buff, efuse_info->key_size);
        ret = efuse_write_entry(efuse_info->efuse_entry_no,
                                temp_buff,
                                efuse_info->key_size);
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
	case IOCTL_EFUSE_WRITE_BIT:
        if (efuse_info->bit_val == 0) {
            EFUSE_ERR("only could set bit to '1'..\n");
            break;
        }
        if (efuse_info->bit_pos > 7) {
            EFUSE_ERR("pos from '0' to '7', bit_pos[%d] err..\n", efuse_info->bit_pos);
            break;
        }
        efuse_write_key_bit_to_1(efuse_info->efuse_entry_no, efuse_info->bit_pos);
        break;
	case IOCTL_EFUSE_CHECK_PRO:
        ret = efuse_check_pro_bits(efuse_info->status.protect_bits);
        break;
    case IOCTL_EFUSE_CHECK_LOCK:
        ret = efuse_get_lock_status(&efuse_info->status);
        break;
	case IOCTL_EFUSE_CHECK_ERROR:
        ret = efuse_check_err_bits(&efuse_info->status.error);
        break;
	case IOCTL_EFUSE_SET_LOCK_WRITE_DATA:
		ret = efuse_set_write_lock(efuse_info);
        break;
    case IOCTL_EFUSE_SET_LOCK_DATA:
        ret = efuse_set_read_lock(efuse_info);
        break;
    case IOCTL_EFUSE_GET_LOCK_DATA:
        EFUSE_DBG("not support get lock data ...\n");
        ret = EFUSE_IOCTL_CMD_DUMMY;
        break;
	case IOCTL_EFUSE_TRANS_KEY:
        EFUSE_DBG("please use efuse transkey with cesa...\n");
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

