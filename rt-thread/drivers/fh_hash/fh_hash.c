#include <rtdevice.h>
#include <rthw.h>
#include "fh_def.h"
#include "chip.h"
#include "board_info.h"
#include "fh_clock.h"
#include "fh_pmu.h"

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "hw_hash.h"
#include "fh_hash.h"
#include "dma.h"
#include "dma_mem.h"
#include "fh_dma.h"

#define FHSM_DEF_PERIOD (128)


struct fh_hash_obj_t
{
    int irq_no;
    void *regs;
    struct clk *clk;
    struct rt_completion    run_complete;
    struct rt_mutex        lock;
    struct rt_dma_device *dma_dev;
    unsigned char *dma_buffer;
};


#define HASH_CFG     0
#define RX_FIFO_CFG0     0x4
#define RX_FIFO_CFG1     0x8
#define RX_FIFO_CFG2     0xc
#define REQ_CFG1         0x10
#define MES_INFO_CFG0     0x14
#define MES_INFO_CFG1     0x18
#define MES_INFO_CFG2     0x1C
#define HMAC_KEY_INFO_CFG0     0x20
#define HMAC_KEY_INFO_CFG1     0x24
#define HMAC_KEY_INFO_CFG2     0x28
#define DEBUG_BUS     0x2C
#define HASH_H0                 0x30
#define HASH(x)             (HASH_H0 + ((x) << 2))
#define HASH_MSG_LEN_CFG0             0x70
#define HASH_MSG_LEN_CFG1             0x74
#define HASH_MSG_LEN_CFG2             0x78
#define HASH_MSG_LEN_CFG3             0x7c
#define HKEY_LEN_CFG                 0x80
#define HKEY_CFG0                     0x84
#define HKEY_CFG(x)                (HKEY_CFG0 + ((x) << 2))
#define HASH_FIFO                    0x100
#define HASH_INT_CFG                0xc4
#define PWDATA_SWAP                0xcc


#define HASH_MODE_OFS                    0x0
#define HASH_MODE_MASK                    0x7
#define HASH_SHA1                        0x0
#define HASH_SHA256                        0x1
#define HASH_SHA384                        0x2
#define HASH_SHA512                        0x3

#define HASH_START                        BIT(3)
#define HMAC_START                        BIT(4)
#define DBG_BUS_SEL_OFS                    5
#define DBG_BUS_SEL_MASK                    0x1F70


#define MAX_HASH_TRANS_ONETIME (0x4000)

struct fh_hash_obj_t *hash_obj  = NULL;
struct rt_completion    hash_dma_complete;


void  hash_dma_cb(void *p)
{
     rt_completion_done(&hash_dma_complete);
}


void hash_dma_write_data(struct fh_hash_obj_t *obj, unsigned char *src, int len)
{


    int wl = len;

    if (obj->dma_dev == NULL)
    {
        obj->dma_dev = (struct rt_dma_device *) rt_device_find("fh_dma0");
        obj->dma_dev->ops->control(obj->dma_dev, RT_DEVICE_CTRL_DMA_OPEN, RT_NULL);
        obj->dma_buffer = fh_dma_mem_malloc(MAX_HASH_TRANS_ONETIME);
    }

    struct dma_transfer *hash_trans = rt_malloc(sizeof(struct dma_transfer));

    if (hash_trans == NULL)
        return;

    rt_memset(hash_trans, 0, sizeof(struct dma_transfer));
    hash_trans->channel_number = AUTO_FIND_CHANNEL;

    int ret = obj->dma_dev->ops->control(obj->dma_dev,
        RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL, (void *) hash_trans);
    if (ret != RT_EOK)
        return;



    while (wl > 0)
    {
        int curtranslen = wl >= MAX_HASH_TRANS_ONETIME ? MAX_HASH_TRANS_ONETIME:wl;

        rt_memcpy(obj->dma_buffer, src, curtranslen);

        hash_trans->dma_number = 0;
        hash_trans->dst_add =  (rt_uint32_t)obj->regs+HASH_FIFO;
        hash_trans->dst_hs = DMA_HW_HANDSHAKING;
        hash_trans->dst_inc_mode = DW_DMA_SLAVE_FIX;
        hash_trans->dst_msize = 1;
        hash_trans->dst_per = HASH_DMA_HANDSHAKE;
        hash_trans->dst_width = DW_DMA_SLAVE_WIDTH_32BIT;

        hash_trans->src_add = (rt_uint32_t)obj->dma_buffer;
        hash_trans->src_inc_mode = DW_DMA_SLAVE_INC;
        hash_trans->src_msize = 1;
        hash_trans->src_width = DW_DMA_SLAVE_WIDTH_32BIT;

        hash_trans->trans_len = (curtranslen+3)/4;
        hash_trans->complete_callback = (void *) hash_dma_cb;
        hash_trans->complete_para = (void *) obj;
        hash_trans->fc_mode = DMA_M2P;


        rt_completion_init(&hash_dma_complete);

        if (len > 0x100)
            SET_REG(obj->regs+REQ_CFG1, 0);

        obj->dma_dev->ops->control(obj->dma_dev,
            RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
            (void *) hash_trans);


        rt_completion_wait(&hash_dma_complete, RT_WAITING_FOREVER);

        wl -= curtranslen;
        src += curtranslen;
    }

     obj->dma_dev->ops->control(obj->dma_dev,
        RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL, (void *) hash_trans);

    rt_free(hash_trans);
    return;



}

void fh_hash_isr(int vector, void *param)
{
    struct fh_hash_obj_t *hash_obj = (struct fh_hash_obj_t *)param;

    unsigned int v = GET_REG(hash_obj->regs+HASH_INT_CFG);

    SET_REG(hash_obj->regs+HASH_INT_CFG, v|0x4);

    rt_completion_done(&hash_obj->run_complete);

    return;
}


static void hash_xfer_data_prep(struct fh_hash_obj_t *obj, void *buff, unsigned long buflen)
{
    unsigned int dmaonce = 0;

    SET_REG(obj->regs+HASH_MSG_LEN_CFG0, buflen);
    if (buflen > 0x100)
        SET_REG(obj->regs+REQ_CFG1, 2);

    dmaonce = buflen > 0x10 ? 0x10 : buflen;

    SET_REG(obj->regs+HASH_MSG_LEN_CFG0, buflen);

    SET_REG(obj->regs+MES_INFO_CFG2, dmaonce);

}

static void hash_xfer_data(struct fh_hash_obj_t *obj, void *buff, unsigned long buflen)
{

    hash_dma_write_data(obj, buff, buflen);
}



static int sha1(struct fh_hash_obj_t *obj, void *buff, unsigned long buflen)
{
    hash_xfer_data_prep(obj, buff, buflen);
    SET_REG(obj->regs+HASH_CFG, 0x2008);
    hash_xfer_data(obj, buff, buflen);
    return 0;
}

static int sha256(struct fh_hash_obj_t *obj, void *buff, unsigned long buflen)
{

    hash_xfer_data_prep(obj, buff, buflen);

    SET_REG(obj->regs+HASH_CFG, 0x2009);

    hash_xfer_data(obj, buff, buflen);

    return 0;
}

static int sha384(struct fh_hash_obj_t *obj, void *buff, unsigned long buflen)
{
    hash_xfer_data_prep(obj, buff, buflen);

    SET_REG(obj->regs+HASH_CFG, 0x200a);

    hash_xfer_data(obj, buff, buflen);
    return 0;
}

static int sha512(struct fh_hash_obj_t *obj, void *buff, unsigned long buflen)
{

    hash_xfer_data_prep(obj, buff, buflen);

    SET_REG(obj->regs+HASH_CFG, 0x200b);

    hash_xfer_data(obj, buff, buflen);

    return 0;
}



int hw_sha_mol(struct fh_hash_obj_t *obj, int mode, void *buff, unsigned long buflen, unsigned char *res)
{
    int ret = -1;

    if (buflen == 0)
        return -1;



    fh_pmu_set_hashen(0);
    switch (mode)
    {
    case 160:
        ret = sha1(obj, buff, buflen);
        break;
    case 256:
        ret = sha256(obj, buff, buflen);
        break;
    case 384:
        ret = sha384(obj, buff, buflen);
        break;
    case 512:
        ret = sha512(obj, buff, buflen);
        break;
    default:
        ret = -1;
        break;
    }
    fh_pmu_set_hashen(1);
    if (ret == 0)
    {
        rt_memcpy((void *)res, (void *)obj->regs+HASH_H0, mode/8);
        SET_REG(obj->regs+HASH_CFG, 0);
    }
    return ret;
}



static long fh_hash_ioctl(rt_device_t dev, int cmd, void *arg)
{

    int ret = -RT_EINVAL;
    struct fh_hash_param_t *param = (struct fh_hash_param_t *)arg;

    if (arg == 0)
        return -RT_EINVAL;


    switch (cmd)
    {
    case FH_HASH_CALC:
    {

        rt_mutex_take(&hash_obj->lock, RT_WAITING_FOREVER);
        rt_completion_init(&hash_obj->run_complete);
        ret = hw_sha_mol(hash_obj, param->shalen, param->buffer, param->len, param->res);
        if (ret == 0)
            rt_completion_wait(&hash_obj->run_complete, RT_WAITING_FOREVER);
        else
            ret = -RT_EINVAL;

        rt_mutex_release(&hash_obj->lock);
        break;
    }
    default:
        break;
    }
    return ret;
}



rt_device_t fh_hash_device;


#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops hash_ops = {
    .control = fh_hash_ioctl
};
#endif

static int fh_hash_probe(void *priv_data)
{

    struct fh_hash_obj *pdev = (struct fh_hash_obj *)priv_data;

    hash_obj = rt_malloc(sizeof(struct fh_hash_obj_t));
    if (hash_obj == NULL)
        return -RT_EINVAL;

    hash_obj->irq_no = pdev->irq;
    hash_obj->regs = (void *)pdev->base;

    rt_completion_init(&hash_obj->run_complete);
    rt_mutex_init(&hash_obj->lock, "hash_lock", RT_IPC_FLAG_FIFO);

    hash_obj->dma_dev = NULL;
    hash_obj->dma_buffer = NULL;
    fh_hash_device = rt_malloc(sizeof(struct rt_device));
    rt_memset(fh_hash_device, 0, sizeof(struct rt_device));
#ifdef RT_USING_DEVICE_OPS
    fh_hash_device->ops       = &hash_ops;
#else
    fh_hash_device->control   = fh_hash_ioctl;
#endif
    fh_hash_device->type      = RT_Device_Class_Miscellaneous;
    fh_hash_device->user_data = hash_obj;

    SET_REG(hash_obj->regs+PWDATA_SWAP, 0x0123);
    SET_REG(hash_obj->regs+RX_FIFO_CFG0, 0x1f1ffcfc);


    rt_hw_interrupt_install(hash_obj->irq_no, fh_hash_isr,
                                (void *)hash_obj, "fh_hahs");
    rt_hw_interrupt_umask(hash_obj->irq_no);


    rt_device_register(fh_hash_device, FH_MHASH_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);


    return 0;

}


int fh_hash_exit(void *priv_data) { return 0; }
struct fh_board_ops fh_hash_driver_ops = {
    .probe = fh_hash_probe, .exit = fh_hash_exit,
};

void rt_hw_hash_init(void)
{
    fh_board_driver_register(FH_MHASH_DEVICE_NAME, &fh_hash_driver_ops);
}

