/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#include "hw_cesa_if.h"
#include "fh_dma.h"
#include "dma.h"

#define EncryptControl        (0x0)
#define FIFOStatus            (0x8)
#define PErrStatus            (0xC)
#define ClearKey0            (0x10)
#define ClearKey1            (0x14)
#define ClearKey2            (0x18)
#define ClearKey3            (0x1C)
#define ClearKey4            (0x20)
#define ClearKey5            (0x24)
#define ClearKey6            (0x28)
#define ClearKey7            (0x2c)
#define InitIV0                (0x30)
#define InitIV1                (0x34)
#define InitIV2                (0x38)
#define InitIV3                (0x3C)
#define DMASrc0                (0x48)
#define DMADes0                (0x4c)
#define DMATrasSize            (0x50)
#define DMACtrl                (0x54)
#define FIFOThold              (0x58)
#define IntrEnable             (0x5c)
#define IntrSrc                (0x60)
#define MaskedIntrStatus       (0x64)
#define IntrClear              (0x68)
#define ExtDmaDebug0           (0x6c)
#define ExtDmaDebug1           (0x70)
#define ExtDmaDebug2           (0x74)
#define ExtDmaDebug3           (0x78)
#define ExtDmaDebug4           (0x7C)
#define LastIV0                (0x80)
#define LastIV1                (0x84)
#define LastIV2                (0x88)
#define LastIV3                (0x8c)
#define ExtDmaCfg0             (0x90)
#define ExtDmaCfg1             (0x94)
#define ExtDmaCfg2             (0x98)

extern void mmu_clean_dcache(rt_uint32_t addr, rt_uint32_t size);

typedef union _HW_CIPHER_CTRL_REG
{
  uint32_t all;
  struct {
  uint32_t OPER_MODE:1;
  uint32_t ALGO_MODE:3;
  uint32_t WORK_MODE:3;
  uint32_t FIRST_BLOCK:1;
  uint32_t DES_PCHK:1;
  uint32_t SELF_KEY_GEN_EN:1;
  uint32_t SELF_SEC_FIRST:1;
  uint32_t SELF_SEC_IV_SEL:1;
  uint32_t SELF_KEY_ENDIAN:1;
  uint32_t:19;
  } bitc;
} HW_CIPHER_CTRL_REG;

static void biglittle_swap(uint8_t *buf)
{
    uint8_t tmp, tmp1;

    tmp = buf[0];
    tmp1 = buf[1];
    buf[0] = buf[3];
    buf[1] = buf[2];
    buf[2] = tmp1;
    buf[3] = tmp;
}

static rt_bool_t use_efuse_key(HW_CESA_ADES_CTRL_S *hw_ctrl, uint32_t key_size)
{
    if (hw_ctrl->efuse_para.mode & CRYPTO_EX_MEM_SET_KEY)
    {
#ifdef FH_USING_EFUSE
        extern void efuse_trans_key(void *efuse_para);
        efuse_trans_key(&hw_ctrl->efuse_para);
#else
        rt_kprintf("Failed to use efuse key due to FH_USING_EFUSE is off\n");
#endif
        return RT_TRUE;
    } else
        return RT_FALSE;
}

int32_t hw_cesa_ades_config(HW_CESA_ADES_CTRL_S *hw_ctrl)
{
    HW_CIPHER_CTRL_REG ctrl_reg;
    uint32_t temp_key_buf[8];
    uint32_t temp_iv_buf[4];
    uint32_t i, key_size, iv_size;


    ctrl_reg.all = ADES_REG32(EncryptControl);
    ctrl_reg.bitc.OPER_MODE = hw_ctrl->oper_mode;
    ctrl_reg.bitc.ALGO_MODE = hw_ctrl->algo_mode;
    ctrl_reg.bitc.WORK_MODE = hw_ctrl->work_mode;
    ctrl_reg.bitc.FIRST_BLOCK = 1;
    ctrl_reg.bitc.SELF_KEY_GEN_EN = hw_ctrl->self_key_gen;
    ADES_REG32(EncryptControl) = ctrl_reg.all;

    /*** KEY ***/
    switch (hw_ctrl->algo_mode)
    {
    case HW_CESA_ADES_ALGO_MODE_DES:
        key_size = 8;
        break;
    case HW_CESA_ADES_ALGO_MODE_TDES:
        key_size = 24;
        break;
    case HW_CESA_ADES_ALGO_MODE_AES128:
        key_size = 16;
        break;
    case HW_CESA_ADES_ALGO_MODE_AES192:
        key_size = 24;
        break;
    case HW_CESA_ADES_ALGO_MODE_AES256:
        key_size = 32;
        break;
    default:
        rt_kprintf("cipher error algo_mode!!\n");
        return CESA_ADES_ALGO_MODE_ERROR;
    }

    if (use_efuse_key(hw_ctrl, key_size) == RT_FALSE)
    {
        memcpy((uint8_t *) &temp_key_buf[0], hw_ctrl->key, key_size);
        for (i = 0; i < key_size / sizeof(uint32_t); i++)
        {
            biglittle_swap((uint8_t *) (temp_key_buf + i));
            ADES_REG32(ClearKey0 + 4*i) = temp_key_buf[i];
        }
    }

    /*** IV ***/
    switch (hw_ctrl->work_mode)
    {
    case HW_CESA_ADES_WORK_MODE_CBC:
    case HW_CESA_ADES_WORK_MODE_CTR:
    case HW_CESA_ADES_WORK_MODE_CFB:
    case HW_CESA_ADES_WORK_MODE_OFB:
        if (hw_ctrl->algo_mode >= HW_CESA_ADES_ALGO_MODE_AES128)
            iv_size = 16;
        else
            iv_size = 8;
        break;
    case HW_CESA_ADES_WORK_MODE_ECB:
        iv_size = 0;
        break;
    default:
        rt_kprintf("cipher error work_mode!!\n");
        return CESA_ADES_WORK_MODE_ERROR;
    }


    for (i = 0; i < iv_size; i++)
    {
        if (hw_ctrl->iv_last[i] != 0)
        {
            uint32_t *temp_iv_ptr = (uint32_t *)hw_ctrl->iv_last;

            for (i = 0; i < iv_size / sizeof(uint32_t); i++)
                ADES_REG32(InitIV0 + 4*i) = temp_iv_ptr[i];

            break;
        }
    }

    if (i >= iv_size)
    {
        memcpy((uint8_t *) &temp_iv_buf[0], hw_ctrl->iv_init, iv_size);
        for (i = 0; i < iv_size / sizeof(uint32_t); i++)
        {
            biglittle_swap((uint8_t *) (temp_iv_buf + i));
            ADES_REG32(InitIV0 + 4*i) = temp_iv_buf[i];
        }
    }

    /* algo_mode & work_mode conflict check */
    if (hw_ctrl->algo_mode <= HW_CESA_ADES_ALGO_MODE_TDES
        && hw_ctrl->work_mode == HW_CESA_ADES_WORK_MODE_CTR)
        return CESA_ADES_WORK_ALGO_MODE_CONFLICT;

    return CESA_SUCCESS;
}

void hw_ades_config_selfkey(HW_CESA_ADES_CTRL_S *hw_ctrl, bool use_self_key, uint8_t *self_key_text)
{
    struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");

    if (plat_data->ext_dma_enable != CESA_USE_EXT_DMA)
        return;

    if (use_self_key) {
        hw_ctrl->self_key_gen = true;
        memcpy(hw_ctrl->self_key_text, self_key_text, KEY_MAX_SIZE);
    } else {
        hw_ctrl->self_key_gen = false;
        memset(hw_ctrl->self_key_text, 0, KEY_MAX_SIZE);
    }

	return;
}

int32_t hw_ades_ext_dma_req_chan(HW_CESA_ADES_CTRL_S *hw_ctrl)
{
    struct rt_dma_device * dma_dev;
    struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");
    int ret = CESA_SUCCESS;

    if (plat_data->ext_dma_enable != CESA_USE_EXT_DMA)
        return CESA_SUCCESS;

    if (hw_ctrl->ext_dma_dev && hw_ctrl->src_chan && hw_ctrl->dst_chan)
        return CESA_SUCCESS;

    hw_ctrl->ext_dma_dev = (struct rt_dma_device *)rt_device_find(plat_data->ext_dma_name);
    if (hw_ctrl->ext_dma_dev == NULL) {
        rt_kprintf("ERROR: %s, can't find dma device\n", __func__);
        return CESA_INVALID_VALUE;
    } else {
        dma_dev = hw_ctrl->ext_dma_dev;
        dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_OPEN, RT_NULL);
    }

    hw_ctrl->src_chan = (struct dma_transfer *)rt_malloc(sizeof(struct dma_transfer));
    if (hw_ctrl->src_chan == NULL) {
        ret = -RT_ENOMEM;
        goto err0;
    }

    hw_ctrl->dst_chan = (struct dma_transfer *)rt_malloc(sizeof(struct dma_transfer));
    if (hw_ctrl->dst_chan == NULL) {
        ret = -RT_ENOMEM;
        goto err1;
    }

    rt_memset((void *)hw_ctrl->src_chan, 0x0, sizeof(struct dma_transfer));
    rt_memset((void *)hw_ctrl->dst_chan, 0x0, sizeof(struct dma_transfer));
    hw_ctrl->src_chan->dma_number           = 0;
    hw_ctrl->dst_chan->dma_number           = 0;
    hw_ctrl->src_chan->channel_number       = AUTO_FIND_CHANNEL;
    hw_ctrl->dst_chan->channel_number       = AUTO_FIND_CHANNEL;
    if (dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
        (void *)hw_ctrl->src_chan)) {
        rt_kprintf("ERROR: %s, request src channel failed\n", __func__);
        ret = -RT_ENOMEM;
        goto err2;
    }

    if (dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
        (void *)hw_ctrl->dst_chan)) {
        rt_kprintf("ERROR: %s, request dst channel failed\n", __func__);
        ret = -RT_ENOMEM;
        goto err3;
    }

    return ret;

err3:
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL, (void *)hw_ctrl->src_chan);
err2:
    rt_free(hw_ctrl->dst_chan);
err1:
    rt_free(hw_ctrl->src_chan);
err0:
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_CLOSE, NULL);
    hw_ctrl->src_chan = NULL;
    hw_ctrl->dst_chan = NULL;
    hw_ctrl->ext_dma_dev = NULL;
    return ret;
}

void hw_ades_ext_dma_rel_chan(HW_CESA_ADES_CTRL_S *hw_ctrl)
{
    struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");
    struct rt_dma_device * dma_dev = hw_ctrl->ext_dma_dev;

    if (plat_data->ext_dma_enable != CESA_USE_EXT_DMA)
        return;

    if (hw_ctrl->src_chan == NULL &&
        hw_ctrl->dst_chan == NULL &&
        hw_ctrl->ext_dma_dev == NULL)
        return;

    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL, (void *)hw_ctrl->src_chan);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL, (void *)hw_ctrl->dst_chan);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_CLOSE, NULL);
    rt_free(hw_ctrl->src_chan);
    rt_free(hw_ctrl->dst_chan);
    hw_ctrl->src_chan = NULL;
    hw_ctrl->dst_chan = NULL;
    hw_ctrl->ext_dma_dev = NULL;
}

int32_t hw_ades_ext_dma_done(HW_CESA_ADES_CTRL_S *hw_ctrl)
{
	uint32_t timeout;
	struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");

	if (plat_data->ext_dma_enable != CESA_USE_EXT_DMA)
		return CESA_SUCCESS;

	timeout = 0xffff;
	while (hw_ctrl->src_done == 0) {
		timeout--;
		if (timeout == 0) {
			rt_kprintf("ades external dma_src timeout!\n");
			return CESA_ADES_DMA_DONE_TIMEOUT;
		}
	}

	timeout = 0xffff;
	while (hw_ctrl->dst_done == 0) {
		timeout--;
		if (timeout == 0) {
			rt_kprintf("ades external dma_dst timeout!\n");
			return CESA_ADES_DMA_DONE_TIMEOUT;
		}
	}

	return CESA_SUCCESS;
}

static void hw_ades_dma_src_done(void *arg)
{
    HW_CESA_ADES_CTRL_S *hw_ctrl = (HW_CESA_ADES_CTRL_S *) arg;

    hw_ctrl->src_done = 1;
}

static void hw_ades_dma_dst_done(void *arg)
{
    HW_CESA_ADES_CTRL_S *hw_ctrl = (HW_CESA_ADES_CTRL_S *) arg;

    hw_ctrl->dst_done = 1;
}
struct usrdef_trans_desc desc[2];
static void hw_ades_dma_set_src(HW_CESA_ADES_CTRL_S *hw_ctrl, uint32_t phy_src_addr, uint32_t length)
{
    struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");
    struct rt_dma_device * dma_dev = hw_ctrl->ext_dma_dev;

    if (hw_ctrl->self_key_gen) {
        struct usrdef_trans_desc desc[2];

        hw_ctrl->src_chan->fc_mode              = DMA_M2P;
        hw_ctrl->src_chan->dst_hs               = DMA_HW_HANDSHAKING;
        hw_ctrl->src_chan->dst_per              = plat_data->ext_dma_rx_handshake;
        hw_ctrl->src_chan->complete_callback    = hw_ades_dma_src_done;
        hw_ctrl->src_chan->complete_para        = hw_ctrl;
        hw_ctrl->src_done = 0;

        desc[0].src_width       = DW_DMA_SLAVE_WIDTH_32BIT;
        desc[0].src_msize       = DW_DMA_SLAVE_MSIZE_8;
        desc[0].src_add         = (rt_uint32_t)hw_ctrl->self_key_text;
        desc[0].src_inc_mode    = DW_DMA_SLAVE_INC;
        desc[0].dst_width       = DW_DMA_SLAVE_WIDTH_32BIT;
        desc[0].dst_msize       = DW_DMA_SLAVE_MSIZE_8;
        desc[0].dst_add         = g_cesa->ades_reg + ExtDmaCfg2;
        desc[0].dst_inc_mode    = DW_DMA_SLAVE_FIX;
        desc[0].size            = KEY_MAX_SIZE;
        desc[0].data_switch     = SWT_ABCD_DCBA;
        desc[0].ot_len_flag     = USR_DEFINE_ONE_TIME_LEN;
        desc[0].ot_len_len      = 0x20;

        desc[1].src_width       = DW_DMA_SLAVE_WIDTH_32BIT;
        desc[1].src_msize       = DW_DMA_SLAVE_MSIZE_8;
        desc[1].src_add         = phy_src_addr;
        desc[1].src_inc_mode    = DW_DMA_SLAVE_INC;
        desc[1].dst_width       = DW_DMA_SLAVE_WIDTH_32BIT;
	    desc[1].dst_msize       = DW_DMA_SLAVE_MSIZE_8;
        desc[1].dst_add         = g_cesa->ades_reg + ExtDmaCfg2;
        desc[1].dst_inc_mode    = DW_DMA_SLAVE_FIX;
        desc[1].size            = length;
        desc[1].data_switch     = SWT_ABCD_DCBA;
        desc[1].ot_len_flag     = USR_DEFINE_ONE_TIME_LEN;
        desc[1].ot_len_len      = 0x20;

        hw_ctrl->src_chan->p_desc = desc;
        hw_ctrl->src_chan->desc_size = 2;
        mmu_clean_dcache((rt_uint32_t)hw_ctrl->self_key_text, KEY_MAX_SIZE);
        mmu_clean_dcache(phy_src_addr,length);
        dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_USR_DEF,
                                   (void *)hw_ctrl->src_chan);
    } else {
        hw_ctrl->src_chan->fc_mode              = DMA_M2P;
        hw_ctrl->src_chan->src_width            = DW_DMA_SLAVE_WIDTH_32BIT;
        hw_ctrl->src_chan->src_msize            = DW_DMA_SLAVE_MSIZE_8;
        hw_ctrl->src_chan->src_add              = phy_src_addr;
        hw_ctrl->src_chan->src_inc_mode         = DW_DMA_SLAVE_INC;	
        hw_ctrl->src_chan->dst_hs               = DMA_HW_HANDSHAKING;
        hw_ctrl->src_chan->dst_per              = plat_data->ext_dma_rx_handshake;
        hw_ctrl->src_chan->dst_width            = DW_DMA_SLAVE_WIDTH_32BIT;
        hw_ctrl->src_chan->dst_msize            = DW_DMA_SLAVE_MSIZE_8;
        hw_ctrl->src_chan->dst_add              = g_cesa->ades_reg + ExtDmaCfg2;
        hw_ctrl->src_chan->dst_inc_mode         = DW_DMA_SLAVE_FIX;
        hw_ctrl->src_chan->trans_len            = length/4;
        hw_ctrl->src_chan->complete_callback    = hw_ades_dma_src_done;    
        hw_ctrl->src_chan->complete_para        = hw_ctrl;
        hw_ctrl->src_chan->data_switch          = SWT_ABCD_DCBA;
        hw_ctrl->src_chan->ot_len_flag          = USR_DEFINE_ONE_TIME_LEN;
        hw_ctrl->src_chan->ot_len_len           = 0x20;
        hw_ctrl->src_done = 0;

        mmu_clean_dcache(phy_src_addr,length);
        dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                   (void *)hw_ctrl->src_chan);
    }
}

static void hw_ades_dma_set_dst(HW_CESA_ADES_CTRL_S *hw_ctrl, uint32_t phy_dst_addr, uint32_t length)
{
    struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");
    struct rt_dma_device * dma_dev = hw_ctrl->ext_dma_dev;

    hw_ctrl->dst_chan->fc_mode              = DMA_P2M;
    hw_ctrl->dst_chan->src_hs               = DMA_HW_HANDSHAKING;
    hw_ctrl->dst_chan->src_per              = plat_data->ext_dma_tx_handshake;
    hw_ctrl->dst_chan->src_width            = DW_DMA_SLAVE_WIDTH_32BIT;
    hw_ctrl->dst_chan->src_msize            = DW_DMA_SLAVE_MSIZE_8;
    hw_ctrl->dst_chan->src_add              = g_cesa->ades_reg + ExtDmaCfg2;
    hw_ctrl->dst_chan->src_inc_mode         = DW_DMA_SLAVE_FIX;
    hw_ctrl->dst_chan->dst_width            = DW_DMA_SLAVE_WIDTH_32BIT;
    hw_ctrl->dst_chan->dst_msize            = DW_DMA_SLAVE_MSIZE_8;
    hw_ctrl->dst_chan->dst_add              = phy_dst_addr;
    hw_ctrl->dst_chan->dst_inc_mode         = DW_DMA_SLAVE_INC;
    hw_ctrl->dst_chan->trans_len            = length/4;
    hw_ctrl->dst_chan->complete_callback    = hw_ades_dma_dst_done;
    hw_ctrl->dst_chan->complete_para        = hw_ctrl;
    hw_ctrl->dst_chan->data_switch          = SWT_ABCD_DCBA;
    hw_ctrl->dst_chan->ot_len_flag          = USR_DEFINE_ONE_TIME_LEN;
    hw_ctrl->dst_chan->ot_len_len           = 0x20;
    hw_ctrl->dst_done = 0;

    mmu_clean_dcache(phy_dst_addr,length);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                   (void *)hw_ctrl->dst_chan);
}

int32_t hw_cesa_ades_process(HW_CESA_ADES_CTRL_S *hw_ctrl,
                                    uint32_t phy_src_addr,
                                    uint32_t phy_dst_addr,
                                    uint32_t length)
{
    uint32_t outfifo_thold;
    uint32_t infifo_thold;
    struct fh_cesa_platform_data *plat_data = fh_get_board_info_data("fh_cesa");

    if ((phy_src_addr & 0x00000003) || (phy_dst_addr & 0x00000003))
    {
        rt_kprintf("ades at dma mode: input or output address is not 4bytes multiple\n");
        return CESA_ADES_DMA_ADDR_ERROR;
    }

    /* length check */
    /* AES/DES CFB data length 1 byte align is permitted */
    if (hw_ctrl->algo_mode >= HW_CESA_ADES_ALGO_MODE_AES128)
    {
        if (hw_ctrl->work_mode != HW_CESA_ADES_WORK_MODE_CFB)
        {
            if (length & 0xF)
            {
                rt_kprintf("error: AES ECB/CBC/OFB/CTR data length should be 16 byte align!\n");
                return CESA_ADES_DATA_LENGTH_ERROR;
            }
        }
    }
    else
    {
        if (hw_ctrl->work_mode != HW_CESA_ADES_WORK_MODE_CFB)
        {
            if (length & 0x7)
            {
                rt_kprintf("error: DES ECB/CBC/OFB data length should be 8 byte align!\n");
                return CESA_ADES_DATA_LENGTH_ERROR;
            }
        }
    }

    if (plat_data->ext_dma_enable == CESA_USE_EXT_DMA) {
        hw_ades_dma_set_src(hw_ctrl, phy_src_addr, length);
        hw_ades_dma_set_dst(hw_ctrl, phy_dst_addr, length);

        outfifo_thold = 0x20;
        infifo_thold = 0x20;
        ADES_REG32(ExtDmaCfg1) = outfifo_thold << 16 | infifo_thold;
        ADES_REG32(IntrEnable) = 1;
        ADES_REG32(ExtDmaCfg0) = 1;

        if (hw_ctrl->self_key_gen) {
            hw_ctrl->self_key_gen = false;
            ADES_REG32(DMATrasSize) = length + 0x20;
            ADES_REG32(DMACtrl) = 0x11;
        } else {
            ADES_REG32(DMATrasSize) = length;			
            ADES_REG32(DMACtrl) = 1;
        }
    } else {
        outfifo_thold = 0x40;
        infifo_thold = 0x40;
        ADES_REG32(DMASrc0) = phy_src_addr;
        ADES_REG32(DMADes0) = phy_dst_addr;
        ADES_REG32(DMATrasSize) = length;
        ADES_REG32(FIFOThold) = (outfifo_thold << 8) | infifo_thold;
        ADES_REG32(IntrEnable) = 1;
        ADES_REG32(DMACtrl) = 1;
    }

    return CESA_SUCCESS;
}

uint32_t hw_cesa_ades_intr_src(void)
{
    uint32_t intr_src;

    intr_src = ADES_REG32(IntrSrc);
    ADES_REG32(IntrClear) = 1;

    return intr_src;
}

void hw_cesa_ades_save_lastiv(HW_CESA_ADES_CTRL_S *hw_ctrl)
{
    uint32_t temp_iv_buf[4];

    temp_iv_buf[0] = ADES_REG32(LastIV0);
    temp_iv_buf[1] = ADES_REG32(LastIV1);
    temp_iv_buf[2] = ADES_REG32(LastIV2);
    temp_iv_buf[3] = ADES_REG32(LastIV3);
    memcpy(hw_ctrl->iv_last, temp_iv_buf, 16);
}

void hw_cesa_ades_restore_lastiv(HW_CESA_ADES_CTRL_S *hw_ctrl)
{
    memset(hw_ctrl->iv_last, 0, IV_MAX_SIZE);
}

