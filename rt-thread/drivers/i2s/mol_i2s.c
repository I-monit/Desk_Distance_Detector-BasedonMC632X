/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-29     wangyl307    add license Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <errno.h>
#include "fh_i2s.h"
#include "fh_def.h"
#include "fh_dma.h"
#include "dma.h"
#include "fh_arch.h"
#include "fh_chip.h"
#include "fh_pmu.h"
#include "fh_clock.h"
#include "board_info.h"

#if !defined(FH_USING_DMA) && !defined(FH_USING_AXI_DMA) && !defined(FH_USING_MOL_DMA)
#warning I2S depends on MOL DMA
#endif

/* #define FH_I2S_DEBUG */
// #define I2S_SELFTEST

#if defined(FH_I2S_DEBUG)
#define PRINT_I2S_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_I2S: %s %d: ", __func__, __LINE__); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_I2S_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif


#define AUDIO_DMA_PREALLOC_SIZE (128 * 1024)


#define I2S_TX_FIFO                     (0X0)
#define I2S_RX_FIFO                     (0X0)
#define I2S_CLK_DIV                     (0X4)
#define I2S_CTRL0                       (0X8)
#define I2S_PCM_EN                      (BIT(15))
#define I2S_DMA_EN                      (BIT(14))
#define I2S_SCK_REV                     (BIT(11))
#define I2S_LRCK_REV                    (BIT(10))
#define I2S_CLK_SHIFT                   (BIT(8)) /* 0:MSB mode 1:IIS mode */
#define I2S_TX_MODE                     (BIT(7))
#define I2S_RX_MODE                     (BIT(6))
#define I2S_SLAVE_MODE                  (BIT(3))
#define I2S_LSB_TRANS                   (BIT(2))
#define I2S_TX_2ND                      (BIT(1)) /* tx data shift out */
#define I2S_RX_2ND                      (BIT(0)) /* rx data shift out */

#define I2S_CTRL1                       (0XC)
#define I2S_RX_EN                       BIT(15)
#define I2S_TX_EN                       BIT(14)

#define I2S_CTRL2                       (0X10)
#define SWITCH_16BIT_TX                 (BIT(16))
#define SWITCH_16BIT_RX                 (BIT(18))

#define I2S_RXFIFO_CTL                  (0X14)
#define I2S_RXFIFO_FULL_MSK             GENMASK(7,0)
#define I2S_RXFIFO_EMPTY_MSK            GENMASK(15,8)
#define I2S_RXFIFO_FULL(threshold)      (I2S_RXFIFO_FULL_MSK & threshold)

#define I2S_INT_EN                      (0X18)
#define I2S_INT_CLR                     (0X1c)
#define I2S_INT_RAW                     (0X20)
#define I2S_INT_STS                     (0X24)
#define I2S_STS2                        (0x2c)
#define I2S_TXF_REALEMPTY               BIT(7)
#define I2S_TXF_REALFULL                BIT(6)
#define I2S_RXF_REALEMPTY               BIT(5)
#define I2S_RXF_REALFULL_R              BIT(4)
#define I2S_TXF_EMPTY_W                 BIT(3)
#define I2S_TXF_FULL                    BIT(2)
#define I2S_RXF_EMPTY                   BIT(1)
#define I2S_RXF_FULL_R                  BIT(0)

#define I2S_RXFIFO_STS                  (0X28)
#define I2S_TXFIFO_STS                  (0X3c)

#define I2S_DMA_CTRL                    (0X34)
#define I2S_TX_DMA_SEL                  (BIT(5))
#define I2S_RX_DMA_SEL                  (BIT(4))

#define I2S_TXFIFO_CTL                  (0X38)
#define I2S_TXFIFO_FULL_MSK             GENMASK(7,0)
#define I2S_TXFIFO_EMPTY_MSK            GENMASK(15,8)
#define I2S_TXFIFO_FULL                 (I2S_TXFIFO_FULL_MSK & 0xff)
#define I2S_TXFIFO_EMPTY(threshold)     (I2S_TXFIFO_EMPTY_MSK & (threshold << 8))

#define I2S_CLK_CTRL                    (0X40)
#define I2S_SCK_AON                     BIT(3)

#define I2S_CH_LEN                      (0X60) /* channel len */

#define I2S_SET_REG(off, val)   SET_REG(i2s_dev.reg_base+off, val)
#define I2S_GET_REG(off)        GET_REG(i2s_dev.reg_base+off)
#define I2S_SET_REG_M(off, val, mask) \
                SET_REG_M(i2s_dev.reg_base+off, val, mask)

enum audio_type
{
    capture = 0,
    playback,
};

enum audio_state
{
    normal = 0,
    xrun,
    stopping,
    running,
    pending,
    inited,
    paused,
};

struct audio_cfg
{
    int rate;
    int volume;
    enum io_select io_type;
    int frame_bit;
    int channels;
    int buffer_size;
    int period_size;
    int buffer_bytes;
    int period_bytes;
    int start_threshold;
    int stop_threshold;
};

struct audio_ptr_t
{
    struct audio_cfg cfg;
    enum audio_state state;
    rt_thread_t thread;
    long size;
    int hw_ptr;
    int appl_ptr;
    struct rt_semaphore lock;
    void *area;      /*virtual pointer*/
    unsigned int addr; /*physical address*/
    unsigned char *mmap_addr;
};

struct fh_audio_cfg
{
    struct rt_dma_device *capture_dma;
    struct rt_dma_device *playback_dma;
    struct dma_transfer *capture_trans;
    struct dma_transfer *playback_trans;
    struct audio_ptr_t capture;
    struct audio_ptr_t playback;
    struct rt_semaphore sem_capture;
    struct rt_semaphore sem_playback;
};

struct channel_assign
{
    int capture_channel;
    int playback_channel;
};

struct audio_dev
{
    int reg_base;
    unsigned long ctrl_base;
    struct channel_assign channel_assign;
    int dma_master;
    struct fh_audio_cfg *audio_config;
} i2s_dev = {
    .channel_assign = {
        .capture_channel = AUTO_FIND_CHANNEL,
        .playback_channel = AUTO_FIND_CHANNEL,
    },
    .dma_master = 0,
};

static void i2s_prealloc_dma_buffer(struct audio_ptr_t *config);
static void i2s_free_prealloc_dma_buffer(struct audio_ptr_t *config);
static void reset_dma_buff(struct audio_ptr_t *config);
static void fh_i2s_tx_dma_done(void *arg);
static void fh_i2s_rx_dma_done(struct fh_audio_cfg *arg);

static int i2s_request_playback_channel(struct fh_audio_cfg  *audio_config);
static int i2s_request_capture_channel(struct fh_audio_cfg  *audio_config);
static int config_i2s_clk(unsigned int rate, unsigned int bit);

static rt_uint32_t fh_audio_rx_poll(rt_device_t dev, struct timespec *timeout);
static rt_uint32_t fh_audio_tx_poll(rt_device_t dev, struct timespec *timeout);
#define writel(v, a)    SET_REG(a, v)
#define readl(a)        GET_REG(v)

#define set_bit_val(val, nr, addr) \
        SET_REG_M(addr, (val?BIT(nr):0), BIT(nr))

extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);


static void i2s_tx_mode(int en)
{
    if (en) {
        I2S_SET_REG_M(I2S_CTRL0, I2S_TX_MODE, I2S_TX_MODE);
    } else {
        I2S_SET_REG_M(I2S_CTRL0, 0, I2S_TX_MODE);
    }
}

static void i2s_reset_ip(int resume)
{
    int ckd, ctrl0, ctrl1, clk_ctrl, ch_len;

    if (resume) {
        ckd = I2S_GET_REG(I2S_CLK_DIV);
        ctrl0 = I2S_GET_REG(I2S_CTRL0);
        ctrl1 = I2S_GET_REG(I2S_CTRL1);
        clk_ctrl = I2S_GET_REG(I2S_CLK_CTRL);
        ch_len = I2S_GET_REG(I2S_CH_LEN);
        I2S_SET_REG(I2S_CTRL0, 0);
    }
    fh_pmu_reset_i2s();
    if (resume) {
        I2S_SET_REG(I2S_CLK_DIV, ckd);
        I2S_SET_REG_M(I2S_TXFIFO_CTL, 0x0020, 0xffff);
        I2S_SET_REG_M(I2S_RXFIFO_CTL, 0x0020, 0xffff);

        I2S_SET_REG(I2S_CH_LEN, ch_len);
        I2S_SET_REG(I2S_CTRL0, ctrl0);
        I2S_SET_REG(I2S_CTRL1, ctrl1);
        I2S_SET_REG(I2S_CLK_CTRL, clk_ctrl);
    }
}

static void i2s_tx_start(int en)
{
    if (en)
        i2s_reset_ip(1);

    i2s_tx_mode(en);
    if (en) {
        I2S_SET_REG_M(I2S_CTRL1, I2S_TX_EN, I2S_TX_EN);
    } else {
        I2S_SET_REG_M(I2S_CTRL1, 0, I2S_TX_EN);
    }
}

static void i2s_rx_mode(int en)
{
    if (en) {
        I2S_SET_REG_M(I2S_CTRL0, I2S_RX_MODE, I2S_RX_MODE);
    } else {
         /* clear fifo data */
        while (!(I2S_GET_REG(I2S_STS2) & I2S_RXF_EMPTY))
            I2S_GET_REG(I2S_RX_FIFO);
        I2S_SET_REG_M(I2S_CTRL0, 0, I2S_RX_MODE);
    }
}

static void i2s_rx_start(int en)
{
    if (en)
        i2s_reset_ip(1);

    i2s_rx_mode(en);
    if (en) {
        I2S_SET_REG_M(I2S_CTRL1, I2S_RX_EN, I2S_RX_EN);
    } else {
        I2S_SET_REG_M(I2S_CTRL1, 0, I2S_RX_EN);
    }
}

static inline void copy_to_user(void *to, const void *from, rt_ubase_t count)
{
    mmu_invalidate_dcache((rt_uint32_t)from, count);
    rt_memcpy(to, from, count);
}

static inline void copy_from_user(void *to, const void *from, rt_ubase_t count)
{
    rt_memcpy(to, from, count);
    mmu_clean_dcache((rt_uint32_t)to, count);
}

static int config_i2s_frame_bit(int bit)
{
    if (bit % 8 || bit == 0)
        return -EINVAL;

    I2S_SET_REG(I2S_CH_LEN, bit);
    return 0;
}

static void i2s_init_reg(int master, int frame_bit)
{
    int ctrl0 = I2S_GET_REG(I2S_CTRL0);
    int ctrl1 = I2S_GET_REG(I2S_CTRL1);

    ctrl0 |= I2S_DMA_EN|I2S_CLK_SHIFT|I2S_RX_2ND|I2S_SCK_REV;
    if (master)
        ctrl0 &= ~I2S_SLAVE_MODE;
    else
        ctrl0 |= I2S_SLAVE_MODE;
    if (frame_bit == 16)
        ctrl0 |= I2S_LRCK_REV;
    else
        ctrl0 &= ~I2S_LRCK_REV;
    I2S_SET_REG(I2S_CTRL0, ctrl0);
    I2S_SET_REG(I2S_CTRL1, ctrl1|0xf11);
    I2S_SET_REG_M(I2S_TXFIFO_CTL, 0x0020, 0xffff);
    I2S_SET_REG_M(I2S_RXFIFO_CTL, 0x0020, 0xffff);

    if (master) {
        /* enable sck always on */
        I2S_SET_REG_M(I2S_CLK_CTRL, I2S_SCK_AON, I2S_SCK_AON);
    } else {
        I2S_SET_REG_M(I2S_CLK_CTRL, 0, I2S_SCK_AON);
    }
}

static int fh_i2s_stop_playback(struct fh_audio_cfg *audio_config)
{
    PRINT_I2S_DBG("%x %x\n", rt_thread_self(), audio_config->playback.thread);
    if (rt_thread_self() != audio_config->playback.thread)
        return -EBUSY;
    else
        audio_config->playback.thread = RT_NULL;

    if (audio_config->playback.state == stopping)
    {
        return 0;
    }

    audio_config->playback.state     = stopping;

    if (audio_config->playback_trans == NULL)
        goto disable_playback;

    if (!audio_config->playback_trans->first_lli)
        goto free_channel;
    audio_config->playback_dma->ops->control(audio_config->playback_dma,
                                             RT_DEVICE_CTRL_DMA_CYCLIC_STOP,
                                             audio_config->playback_trans);
    audio_config->playback_dma->ops->control(audio_config->playback_dma,
                                             RT_DEVICE_CTRL_DMA_CYCLIC_FREE,
                                             audio_config->playback_trans);
free_channel:
    audio_config->playback_dma->ops->control(audio_config->playback_dma,
                                             RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                             audio_config->playback_trans);
    if (&audio_config->sem_playback)
        rt_sem_detach(&audio_config->sem_playback);

    i2s_free_prealloc_dma_buffer(&audio_config->playback);
    if (audio_config->playback_trans != RT_NULL)
    {
        rt_free(audio_config->playback_trans);
        audio_config->playback_trans = NULL;
    }

disable_playback:
    i2s_tx_start(0);

    return 0;
}

static int fh_i2s_stop_capture(struct fh_audio_cfg *audio_config)
{
    PRINT_I2S_DBG("%x %x\n", rt_thread_self(), audio_config->capture.thread);
    if (rt_thread_self() != audio_config->capture.thread)
        return -EBUSY;
    else
        audio_config->capture.thread = RT_NULL;

    if (audio_config->capture.state == stopping)
    {
        PRINT_I2S_DBG(" capture is stopped\n");
        return 0;
    }

    audio_config->capture.state = stopping;

    if (audio_config->capture_trans == NULL)
        goto disable_capture;
    if (!audio_config->capture_trans->first_lli)
        goto free_channel;
    audio_config->capture_dma->ops->control(audio_config->capture_dma,
                                            RT_DEVICE_CTRL_DMA_CYCLIC_STOP,
                                            audio_config->capture_trans);

    audio_config->capture_dma->ops->control(audio_config->capture_dma,
                                            RT_DEVICE_CTRL_DMA_CYCLIC_FREE,
                                            audio_config->capture_trans);
free_channel:
    audio_config->capture_dma->ops->control(audio_config->capture_dma,
                                            RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                            audio_config->capture_trans);
    if (&audio_config->sem_capture)
        rt_sem_detach(&audio_config->sem_capture);

    i2s_free_prealloc_dma_buffer(&audio_config->capture);
    if (audio_config->capture_trans != RT_NULL)
    {
        rt_free(audio_config->capture_trans);
        audio_config->capture_trans = NULL;
    }
disable_capture:
    i2s_rx_start(0);

    return 0;
}

static inline int frames_to_bytes(int frame_bit, int frames)
{
    return frames * frame_bit / 8;
}

static int init_audio(enum audio_type type, struct fh_audio_cfg *audio_config,
        struct fh_audio_cfg_arg *cfg)
{
    struct audio_ptr_t *config = NULL;
    int ret = 0;

    if (type == capture)
        config = &audio_config->capture;
    else if (type == playback)
        config = &audio_config->playback;

    PRINT_I2S_DBG("######### %d %x\n", type, rt_thread_self());
    if (config->thread)
        return -EBUSY;

    config->cfg.io_type = cfg->io_type;
    config->cfg.volume = cfg->volume;
    config->cfg.rate = cfg->rate;
    config->cfg.channels = cfg->channels;
    config->cfg.buffer_size = cfg->buffer_size;
    config->cfg.frame_bit = cfg->frame_bit;
    config->cfg.period_size = cfg->period_size;
    config->cfg.buffer_bytes =
        frames_to_bytes(config->cfg.frame_bit, config->cfg.buffer_size);
    config->cfg.period_bytes =
        frames_to_bytes(config->cfg.frame_bit, config->cfg.period_size);
    config->cfg.start_threshold = config->cfg.buffer_bytes;
    config->cfg.stop_threshold = config->cfg.buffer_bytes;
    i2s_prealloc_dma_buffer(config);
    reset_dma_buff(config);

    ret = config_i2s_clk(cfg->rate, cfg->frame_bit);
    if (ret)
    {
        rt_kprintf("config_i2s_clk error %d\n", ret);
        return ret;
    }

#ifdef CONFIG_MOL_I2S_SLAVE
    i2s_init_reg(0, cfg->frame_bit);
#else
    i2s_init_reg(1, cfg->frame_bit);
#endif

    config->thread = rt_thread_self();
    config->state = inited;

    if (type == capture)
        rt_sem_init(&audio_config->sem_capture, "sem_capture", 0, RT_IPC_FLAG_FIFO);
    else if (type == playback)
        rt_sem_init(&audio_config->sem_playback, "sem_playback", 1, RT_IPC_FLAG_FIFO);

    return 0;
}

static int avail_data_len(enum audio_type type, struct fh_audio_cfg *stream)
{
    int delta;

    if (capture == type)
    {
        delta = stream->capture.hw_ptr - stream->capture.appl_ptr;

        if (delta < 0)
            delta += stream->capture.size;
        return delta;
    }
    else
    {
        delta = stream->playback.appl_ptr - stream->playback.hw_ptr;

        if (delta <= 0)
            delta += stream->playback.size;
        return stream->playback.size - delta;
    }
}

static rt_err_t fh_audio_close(rt_device_t dev)
{
    struct fh_audio_cfg *i2s_config = dev->user_data;

    fh_i2s_stop_playback(i2s_config);

    fh_i2s_stop_capture(i2s_config);

    return RT_EOK;
}


static int register_tx_dma(struct fh_audio_cfg *i2s_config)
{
    int ret;
    struct dma_transfer *playback_trans;

    playback_trans = i2s_config->playback_trans;
    struct rt_dma_device *rt_dma_dev;

    rt_dma_dev = i2s_config->playback_dma;
    if ((i2s_config->playback.cfg.buffer_bytes <
         i2s_config->playback.cfg.period_bytes) ||
        (i2s_config->playback.cfg.buffer_bytes <= 0) ||
        (i2s_config->playback.cfg.period_bytes <= 0))
    {
        rt_kprintf("buffer_size and period_size are invalid\n");
        return -RT_ERROR;
    }

    ret = rt_dma_dev->ops->control(
        rt_dma_dev, RT_DEVICE_CTRL_DMA_CYCLIC_PREPARE, playback_trans);
    if (ret)
    {
        rt_kprintf("can't playback cyclic prepare\n");
        return -RT_ERROR;
    }

    ret = rt_dma_dev->ops->control(rt_dma_dev, RT_DEVICE_CTRL_DMA_CYCLIC_START,
            i2s_config->playback_trans);
    if (ret)
    {
        rt_kprintf("can't start playback\n");
        return -RT_ERROR;
    }

    return 0;
}

static int register_rx_dma(struct fh_audio_cfg *i2s_config)
{
    int ret;
    struct dma_transfer *capture_slave;

    capture_slave = i2s_config->capture_trans;
    struct rt_dma_device *rt_dma_dev;

    rt_dma_dev = i2s_config->capture_dma;
    if (!capture_slave)
    {
        return -ENOMEM;
    }

    if ((i2s_config->capture.cfg.buffer_bytes <
         i2s_config->capture.cfg.period_bytes) ||
        (i2s_config->capture.cfg.buffer_bytes <= 0) ||
        (i2s_config->capture.cfg.period_bytes <= 0))
    {
        return -RT_ERROR;
    }

    ret = rt_dma_dev->ops->control(
        rt_dma_dev, RT_DEVICE_CTRL_DMA_CYCLIC_PREPARE, capture_slave);
    if (ret)
    {
        PRINT_I2S_DBG("can't capture cyclic prepare\n");
        return -RT_ERROR;
    }
    ret = rt_dma_dev->ops->control(
        rt_dma_dev, RT_DEVICE_CTRL_DMA_CYCLIC_START, capture_slave);
    if (ret)
    {
        PRINT_I2S_DBG("can't capture cyclic start\n");
        return -RT_ERROR;
    }

    return 0;
}

static inline int is_current_thread(struct audio_ptr_t *config)
{
    return (rt_thread_self() == config->thread);
}

static int fh_i2s_start_playback(struct fh_audio_cfg *audio_config)
{
    int ret;

    if (!is_current_thread(&audio_config->playback))
        return -EBUSY;

    if (audio_config->playback.state == running)
    {
        rt_kprintf("playback is running\n");
        return -EBUSY;
    }

    if (audio_config->playback.cfg.buffer_bytes >= AUDIO_DMA_PREALLOC_SIZE)
    {
        rt_kprintf(
            "DMA prealloc buffer is smaller than  audio_config->buffer_bytes %x\n",
            audio_config->playback.cfg.buffer_bytes);
        return -ENOMEM;
    }

    reset_dma_buff(&audio_config->playback);
    audio_config->playback.appl_ptr = audio_config->playback.cfg.period_bytes;

    rt_memset(audio_config->playback.area, 0,
              audio_config->playback.cfg.buffer_bytes);
    audio_config->playback.size = audio_config->playback.cfg.buffer_bytes;
    mmu_clean_invalidated_dcache((rt_uint32_t)audio_config->playback.area, audio_config->playback.cfg.buffer_bytes);

    ret = i2s_request_playback_channel(audio_config);

    if (ret)
    {
        rt_kprintf("can't request playback channel\n");
        return ret;
    }

    ret = register_tx_dma(audio_config);
    if (ret < 0)
    {
        rt_kprintf("can't register tx dma\n");
        return ret;
    }

    i2s_tx_start(1);

    audio_config->playback.state = running;
    return RT_EOK;
}

static int fh_i2s_start_capture(struct fh_audio_cfg *audio_config)
{
    int ret;

    if (!is_current_thread(&audio_config->capture))
        return -EBUSY;

    if (audio_config->capture.state == running)
    {
        rt_kprintf("capture is running\n");
        return -EBUSY;
    }
    if (audio_config->capture.cfg.buffer_bytes >= AUDIO_DMA_PREALLOC_SIZE)
    {
        rt_kprintf(
            "DMA prealloc buffer is smaller than  audio_config->buffer_bytes %x\n",
            audio_config->capture.cfg.buffer_bytes);
        return -ENOMEM;
    }
    reset_dma_buff(&audio_config->capture);
    rt_memset(audio_config->capture.area, 0,
              audio_config->capture.cfg.buffer_bytes);
    mmu_clean_invalidated_dcache((rt_uint32_t)audio_config->capture.area, audio_config->capture.cfg.buffer_bytes);

    audio_config->capture.size  = audio_config->capture.cfg.buffer_bytes;
    audio_config->capture.state = running;
    ret                         = i2s_request_capture_channel(audio_config);
    if (ret)
    {
        rt_kprintf("can't request capture channel\n");
        return ret;
    }

    i2s_rx_start(1);

    return register_rx_dma(audio_config);
}

static void fh_i2s_rx_dma_done(struct fh_audio_cfg *arg)
{
    struct fh_audio_cfg *audio_config = (struct fh_audio_cfg *) arg;
    struct audio_ptr_t *cap = &audio_config->capture;
    int hw_pos = 0;

    int dar = 0;

    dar = audio_config->capture_dma->ops->control(audio_config->capture_dma,
        RT_DEVICE_CTRL_DMA_GET_DAR, audio_config->capture_trans);

    hw_pos = dar - cap->addr;

    /* align to period_bytes*/
    hw_pos -= hw_pos % cap->cfg.period_bytes;

    cap->hw_ptr = hw_pos;

    int avail = avail_data_len(capture, audio_config);

    if (avail >= cap->cfg.period_bytes)
    {
        PRINT_I2S_DBG("rx dma done avail=%d\n", avail);
        rt_sem_release(&audio_config->sem_capture);
    }
}

static void fh_i2s_tx_dma_done(void *arg)
{
    struct fh_audio_cfg *audio_config = (struct fh_audio_cfg *)arg;
    struct audio_ptr_t *play = &audio_config->playback;
    int hw_pos = 0;
    int delta = 0;
    int sar = 0;

    sar = audio_config->playback_dma->ops->control(audio_config->playback_dma,
        RT_DEVICE_CTRL_DMA_GET_SAR, audio_config->playback_trans);

    hw_pos = sar - play->addr;

    /* align to period_bytes*/
    hw_pos -= hw_pos % play->cfg.period_bytes;
    delta = hw_pos - play->hw_ptr;

    if (delta < 0)
    {
        rt_memset(play->area + play->hw_ptr, 0, play->size - play->hw_ptr);
        rt_memset(play->area, 0, hw_pos);
        mmu_clean_invalidated_dcache((rt_uint32_t)(play->area + play->hw_ptr), play->size - play->hw_ptr);
        mmu_clean_invalidated_dcache((rt_uint32_t)play->area, hw_pos);
    }
    else
    {
        rt_memset(play->area + play->hw_ptr, 0, delta);
        mmu_clean_invalidated_dcache((rt_uint32_t)(play->area + play->hw_ptr), delta);
    }

    play->hw_ptr = hw_pos;

    int avail = avail_data_len(playback, audio_config);

    if (avail >= play->cfg.period_bytes)
        rt_sem_release(&audio_config->sem_playback);
}

int arg_config_support(struct fh_audio_cfg_arg *cfg)
{

    return RT_EOK;
}

static void reset_dma_buff(struct audio_ptr_t *config)
{
    config->appl_ptr = 0;
    config->hw_ptr = 0;
}

static rt_err_t fh_audio_ioctl(rt_device_t dev, int cmd, void *arg)
{
    struct fh_audio_cfg_arg *cfg;
    struct timespec *timeout;
    struct fh_audio_cfg *audio_config = (struct fh_audio_cfg *)dev->user_data;
    int ret = 0;

    switch (cmd)
    {
    case AC_INIT_CAPTURE_MEM:
        cfg = (struct fh_audio_cfg_arg *)arg;

        if (arg_config_support(cfg) == 0)
            ret = init_audio(capture, audio_config, cfg);
        else
            return -EINVAL;

        break;
    case AC_INIT_PLAYBACK_MEM:
        cfg = arg;
        if (arg_config_support(cfg) == 0)
            ret = init_audio(playback, audio_config, cfg);
        else
            return -EINVAL;
        break;
    case AC_AI_EN:
        PRINT_I2S_DBG("AC_AI_EN\n");
        return fh_i2s_start_capture(audio_config);
    case AC_AO_EN:
        PRINT_I2S_DBG("AC_AO_EN\n");
        return fh_i2s_start_playback(audio_config);

    case AC_AI_DISABLE:
        PRINT_I2S_DBG("AC_AI_DISABLE\n");
        return fh_i2s_stop_capture(audio_config);
        break;

    case AC_AO_DISABLE:
        PRINT_I2S_DBG("AC_AO_DISABLE\n");
        return fh_i2s_stop_playback(audio_config);
        break;

    case AC_AI_PAUSE:
        if (!is_current_thread(&audio_config->capture))
            return -EBUSY;
        audio_config->capture.state = paused;
        i2s_rx_start(0);
        PRINT_I2S_DBG("capture pause\n");
        break;
    case AC_AI_RESUME:
        if (!is_current_thread(&audio_config->capture))
            return -EBUSY;
        audio_config->capture.state = running;
        i2s_rx_start(1);
        PRINT_I2S_DBG("capture resume\n");
        break;
    case AC_AO_PAUSE:
        if (!is_current_thread(&audio_config->playback))
            return -EBUSY;
        audio_config->playback.state = paused;
        i2s_tx_start(0);
        PRINT_I2S_DBG("playback pause\n");
        break;
    case AC_AO_RESUME:
        if (!is_current_thread(&audio_config->playback))
            return -EBUSY;
        audio_config->playback.state = running;
        i2s_tx_start(1);
        PRINT_I2S_DBG("playback pause\n");
        PRINT_I2S_DBG("playback resume\n");
        break;
    case AC_READ_SELECT:
        timeout = (struct timespec *)arg;
        ret     = fh_audio_rx_poll(dev, timeout);
        if (ret < 0)
            PRINT_I2S_DBG("read select err %d\n", ret);
        break;
    case AC_WRITE_SELECT:
        timeout = (struct timespec *)arg;
        ret     = fh_audio_tx_poll(dev, timeout);
        if (ret < 0)
            PRINT_I2S_DBG("write select err %d\n", ret);
        break;
    case AC_SET_INPUT_MODE:
    case AC_SET_OUTPUT_MODE:
    case AC_AI_SET_VOL:
    case AC_AO_SET_VOL:
    case AC_AI_MICIN_SET_VOL:
        return 0;

    default:
        return -ENOTTY;
    }

    return ret;
}

static rt_err_t fh_audio_open(rt_device_t dev, rt_uint16_t oflag)
{
    return 0;
}

static rt_uint32_t fh_audio_tx_poll(rt_device_t dev, struct timespec *timeout)
{
    struct fh_audio_cfg *audio_config = dev->user_data;
    int mask = 0;

    if ((timeout->tv_sec < 0) || (timeout->tv_nsec < 0))
    {
        PRINT_I2S_DBG("set tx_poll timeout unavailiable\n");
        return -EINVAL;
    }

    unsigned int ticks = rt_tick_from_millisecond(timeout->tv_sec * 1000 +
                                                    timeout->tv_nsec / 1000000);
    if (ticks == 0)
        ticks = RT_WAITING_FOREVER;

    mask = avail_data_len(playback, audio_config);
    if (mask >= audio_config->playback.size)
        return mask;

    rt_sem_control(&audio_config->sem_playback, RT_IPC_CMD_RESET, 0);
    if (rt_sem_take(&audio_config->sem_playback, ticks) < 0)
    {
        PRINT_I2S_DBG("tx poll timeout\n");
        return -ETIMEDOUT;
    }

    mask = avail_data_len(playback, audio_config);

    return mask;
}

/* return 0, no data; >0 available data size */
static rt_uint32_t fh_audio_rx_poll(rt_device_t dev, struct timespec *timeout)
{
    struct fh_audio_cfg *i2s_config = dev->user_data;
    int mask = 0, ticks;

    if ((timeout->tv_sec < 0) || (timeout->tv_nsec < 0))
    {
        PRINT_I2S_DBG("set rx_poll timeout unavailiable\n");
        return -EINVAL;
    }
    ticks = rt_tick_from_millisecond(timeout->tv_sec * 1000 +
                                       timeout->tv_nsec / 1000000);

    if (ticks == 0)
        ticks = RT_WAITING_FOREVER;

    rt_sem_control(&i2s_config->sem_capture, RT_IPC_CMD_RESET, 0);
    if (rt_sem_take(&i2s_config->sem_capture, ticks) < 0)
    {
        PRINT_I2S_DBG("rx poll timeout\n");
        return -ETIMEDOUT;
    }

    mask = avail_data_len(capture, i2s_config);

    return mask;
}

static rt_size_t fh_audio_read(rt_device_t dev, rt_off_t pos, void *buffer,
                               rt_size_t size)
{
    struct fh_audio_cfg *audio_config = dev->user_data;
    int after, left;
    int avail;
    struct timespec timeout = {1, 0};

    if (audio_config->capture.thread != rt_thread_self())
        return -EBUSY;

    if (audio_config->capture.area == NULL)
    {
        PRINT_I2S_DBG("cannot read\n");
        return -EINVAL;
    }

    size -= size % (audio_config->capture.cfg.frame_bit / 8 * 2);

    avail = avail_data_len(capture, audio_config);
    PRINT_I2S_DBG("read %d %d\n", size, avail);

    if (avail < size)
        avail = fh_audio_rx_poll(dev, &timeout);
    else
        avail = size;
    if (avail <= 0)
        return 0;
    else if (avail > size)
        avail = size;
    rt_sem_take(&audio_config->capture.lock, RT_WAITING_FOREVER);
    after = avail + audio_config->capture.appl_ptr;
    if (after > audio_config->capture.size)
    {
        left = avail - (audio_config->capture.size - audio_config->capture.appl_ptr);
        copy_to_user(buffer, audio_config->capture.area + audio_config->capture.appl_ptr,
                (audio_config->capture.size - audio_config->capture.appl_ptr));
        copy_to_user(buffer + (audio_config->capture.size - audio_config->capture.appl_ptr),
                audio_config->capture.area, left);
        audio_config->capture.appl_ptr = left;
    }
    else
    {
        copy_to_user(buffer, audio_config->capture.area + audio_config->capture.appl_ptr,
                    avail);
        audio_config->capture.appl_ptr += avail;
    }
    rt_sem_release(&audio_config->capture.lock);

    return avail;
}

static rt_size_t fh_audio_write(rt_device_t dev, rt_off_t pos,
                                const void *buffer, rt_size_t size)
{
    struct fh_audio_cfg *audio_config = dev->user_data;
    int after, left;
    int avail = 0;
    struct timespec timeout = {1, 0};

    if (audio_config->playback.thread != rt_thread_self())
        return -EBUSY;

    if (audio_config->playback.area == NULL)
    {
        PRINT_I2S_DBG("cannot write\n");
        return -EINVAL;
    }

    size -= size % (audio_config->playback.cfg.frame_bit / 8 * 2);

    avail = avail_data_len(playback, audio_config);
    PRINT_I2S_DBG("write %d %d\n", size, avail);

    if (avail < size)
        avail = fh_audio_tx_poll(dev, &timeout);
    else
        avail = size;
    if (avail <= 0)
        return 0;
    else if (avail > size)
        avail = size;
    rt_sem_take(&audio_config->playback.lock, RT_WAITING_FOREVER);
    after = avail + audio_config->playback.appl_ptr;
    if (after >= audio_config->playback.size)
    {
        left = avail - (audio_config->playback.size - audio_config->playback.appl_ptr);
        copy_from_user(audio_config->playback.area + audio_config->playback.appl_ptr,
                  buffer, (audio_config->playback.size - audio_config->playback.appl_ptr));
        copy_from_user(audio_config->playback.area,
                buffer + (audio_config->playback.size - audio_config->playback.appl_ptr), left);
        audio_config->playback.appl_ptr = left;
    }
    else
    {
        copy_from_user(audio_config->playback.area + audio_config->playback.appl_ptr, buffer, avail);
        audio_config->playback.appl_ptr += avail;
    }
    rt_sem_release(&audio_config->playback.lock);

    return avail;
}

static void i2s_prealloc_dma_buffer(struct audio_ptr_t *config)
{
    config->area = (void *)rt_malloc(AUDIO_DMA_PREALLOC_SIZE);

    if (!config->area)
    {
        rt_kprintf("no enough mem for capture  buffer alloc\n");
        return;
    }
    config->addr = (int)config->area;
}

static void i2s_free_prealloc_dma_buffer(struct audio_ptr_t *config)
{
    if (config->area)
    {
        rt_free(config->area);
        config->area = RT_NULL;
    }
}

static void init_i2s_mutex(struct fh_audio_cfg *i2s_config)
{
    rt_sem_init(&i2s_config->capture.lock, "i2s_rx_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&i2s_config->playback.lock, "i2s_tx_lock", 1, RT_IPC_FLAG_FIFO);
}

int i2s_request_capture_channel(struct fh_audio_cfg *audio_config)
{
    struct rt_dma_device *rt_dma_dev;
    /*request audio rx dma channel*/
    struct dma_transfer *dma_rx_transfer;
    int ret;

    dma_rx_transfer = rt_malloc(sizeof(struct dma_transfer));

    if (!dma_rx_transfer)
    {
        PRINT_I2S_DBG("alloc  dma_rx_transfer failed\n");
        return -RT_ENOMEM;
    }

    rt_memset(dma_rx_transfer, 0, sizeof(struct dma_transfer));
    rt_dma_dev = (struct rt_dma_device *)rt_device_find("fh_dma0");

    if (rt_dma_dev == RT_NULL)
    {
        PRINT_I2S_DBG("can't find dma dev\n");

        return -1;
    }
    audio_config->capture_dma   = rt_dma_dev;
    audio_config->capture_trans = dma_rx_transfer;
    rt_dma_dev->ops->init(rt_dma_dev);

    dma_rx_transfer->channel_number = i2s_dev.channel_assign.capture_channel;

    dma_rx_transfer->dma_number = 0;
    dma_rx_transfer->src_master = i2s_dev.dma_master;
    dma_rx_transfer->dst_master = 0;
    dma_rx_transfer->master_flag = MASTER_SEL_ENABLE;

    dma_rx_transfer->dst_add      = (rt_uint32_t)audio_config->capture.area;
    dma_rx_transfer->dst_inc_mode = DW_DMA_SLAVE_INC;
    dma_rx_transfer->dst_msize    = DW_DMA_SLAVE_MSIZE_32;

    /* blk_len = 2n * (frame_bit*2/8) */
    dma_rx_transfer->ot_len_flag = USR_DEFINE_ONE_TIME_LEN;
    dma_rx_transfer->ot_len_len = audio_config->capture.cfg.frame_bit/2;

    dma_rx_transfer->dst_width = DW_DMA_SLAVE_WIDTH_32BIT;

    dma_rx_transfer->fc_mode   = DMA_P2M;

    dma_rx_transfer->src_add = i2s_dev.reg_base + I2S_RX_FIFO;

    dma_rx_transfer->src_inc_mode = DW_DMA_SLAVE_FIX;
    dma_rx_transfer->src_msize    = DW_DMA_SLAVE_MSIZE_32;
    dma_rx_transfer->src_hs       = DMA_HW_HANDSHAKING;
    dma_rx_transfer->src_width    = dma_rx_transfer->dst_width;
    dma_rx_transfer->trans_len    = (audio_config->capture.cfg.buffer_size);
    dma_rx_transfer->src_per      = I2S_RX; // @suppress("Symbol is not resolved")
    dma_rx_transfer->period_len   = audio_config->capture.cfg.period_size;
    dma_rx_transfer->complete_callback =
        (dma_complete_callback)fh_i2s_rx_dma_done;
    dma_rx_transfer->complete_para = audio_config;

    rt_dma_dev->ops->control(rt_dma_dev, RT_DEVICE_CTRL_DMA_OPEN,
                             dma_rx_transfer);
    ret = rt_dma_dev->ops->control(
        rt_dma_dev, RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL, dma_rx_transfer);
    if (ret)
    {
        PRINT_I2S_DBG("can't request capture channel\n");
        dma_rx_transfer->channel_number = 0xff;
        return -ret;
    }
    return RT_EOK;
}

int i2s_request_playback_channel(struct fh_audio_cfg *audio_config)
{
    struct rt_dma_device *rt_dma_dev;
    int ret;
    struct dma_transfer *dma_tx_transfer;

    dma_tx_transfer = rt_malloc(sizeof(struct dma_transfer));
    if (!dma_tx_transfer)
    {
        rt_kprintf("malloc  dma_tx_transfer failed\n");
        return -RT_ENOMEM;
    }
    audio_config->playback_trans = dma_tx_transfer;
    rt_dma_dev = (struct rt_dma_device *)rt_device_find("fh_dma0");

    if (rt_dma_dev == RT_NULL)
    {
        rt_kprintf("can't find dma dev\n");
        return -1;
    }
    rt_dma_dev->ops->init(rt_dma_dev);
    audio_config->playback_dma = rt_dma_dev;

    rt_memset(dma_tx_transfer, 0, sizeof(struct dma_transfer));
    dma_tx_transfer->channel_number = i2s_dev.channel_assign.playback_channel;
    dma_tx_transfer->dma_number     = 0;
    dma_tx_transfer->src_master     = 0;
    dma_tx_transfer->dst_master     = i2s_dev.dma_master;
    dma_tx_transfer->master_flag = MASTER_SEL_ENABLE;

    /* blk_len = 2n * (frame_bit*2/8) */
    dma_tx_transfer->ot_len_flag = USR_DEFINE_ONE_TIME_LEN;
    dma_tx_transfer->ot_len_len = audio_config->playback.cfg.frame_bit/2;

    dma_tx_transfer->dst_add        = i2s_dev.reg_base + I2S_TX_FIFO;
    dma_tx_transfer->dst_hs         = DMA_HW_HANDSHAKING;
    dma_tx_transfer->dst_inc_mode   = DW_DMA_SLAVE_FIX;
    dma_tx_transfer->dst_msize      = DW_DMA_SLAVE_MSIZE_32;
    dma_tx_transfer->dst_per        = I2S_TX; // @suppress("Symbol is not resolved")
    dma_tx_transfer->dst_width  = DW_DMA_SLAVE_WIDTH_32BIT;
    dma_tx_transfer->fc_mode        = DMA_M2P;
    dma_tx_transfer->src_add        = (rt_uint32_t)audio_config->playback.area;
    dma_tx_transfer->src_inc_mode   = DW_DMA_SLAVE_INC;
    dma_tx_transfer->src_msize      = DW_DMA_SLAVE_MSIZE_32;
    dma_tx_transfer->src_width      = dma_tx_transfer->dst_width;
    dma_tx_transfer->trans_len =
        (audio_config->playback.cfg.buffer_size);  /* BUFF_SIZE; */
    dma_tx_transfer->period_len =
        (audio_config->playback.cfg.period_size);  /* TEST_PER_NO; */
    dma_tx_transfer->complete_callback =
        (dma_complete_callback)fh_i2s_tx_dma_done;
    dma_tx_transfer->complete_para = audio_config;
    rt_dma_dev->ops->control(rt_dma_dev, RT_DEVICE_CTRL_DMA_OPEN,
                             dma_tx_transfer);
    ret = rt_dma_dev->ops->control(
        rt_dma_dev, RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL, dma_tx_transfer);
    if (ret)
    {
        PRINT_I2S_DBG("can't request playbak channel\n");
        dma_tx_transfer->channel_number = 0xff;
        return -ret;
    }
    return RT_EOK;
}

void audio_release_dma_channel(struct fh_audio_cfg *i2s_config)
{
    if (i2s_config->playback_trans != RT_NULL)
    {
        i2s_config->playback_dma->ops->control(
            i2s_config->playback_dma, RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
            i2s_config->playback_trans);
        rt_free(i2s_config->playback_trans);
        i2s_config->playback_trans = NULL;
    }

    if (i2s_config->capture_trans != RT_NULL)
    {
        i2s_config->capture_dma->ops->control(
            i2s_config->capture_dma, RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
            i2s_config->capture_trans);
        rt_free(i2s_config->capture_trans);
        i2s_config->capture_trans = NULL;
    }
}

#define FREQ_12_288_MHz     12288000
#define FREQ_11_2896_MHz    11289600

static int config_mclk_freq(unsigned int freq)
{
    fh_pmu_set_mclk(freq);
    return 0;
}

static struct fh_i2s_platform_data *fh_i2s_data;

void fh_pmu_dwi2s_set_clk(unsigned int div_i2s, unsigned int div_mclk)
{
    I2S_SET_REG(I2S_CLK_DIV, div_i2s* div_mclk/2-1);
}

static int config_i2s_clk(unsigned int rate, unsigned int bit)
{
    unsigned int ret, freq, div;

    if (rate % 8000 == 0)
    {
        freq = FREQ_12_288_MHz;
    }
    else if (rate % 22050 == 0)
    {
        freq = FREQ_11_2896_MHz;
    }
    else
    {
        rt_kprintf("unsupport rate %d\n", rate);
        return -EINVAL;
    }

    ret = config_mclk_freq(freq);
    if (ret)
    {
        rt_kprintf("config_mclk_freq err %d\n", ret);
        return ret;
    }

    div = freq / rate / bit / 2;

    if (div % 2 != 0 || freq % (rate *bit * 2) != 0)
    {
        rt_kprintf("unsupport rate %d and bit %d, div%d\n", rate, bit, div);
        return -EINVAL;
    }

    PRINT_I2S_DBG("freq %d, rate %d, bit %d, div %d\n", freq, rate, bit, div);

    ret = config_i2s_frame_bit(bit);
    if (ret)
    {
        rt_kprintf("config_i2s_frame_bit err %d\n", ret);
        return ret;
    }

    fh_pmu_dwi2s_set_clk(div, 4);
    return 0;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops i2s_ops = {
    .open = fh_audio_open,
    .read = fh_audio_read,
    .write = fh_audio_write,
    .close = fh_audio_close,
    .control = fh_audio_ioctl,
};
#endif

static int fh_i2s_probe(void *priv_data)
{
    struct fh_audio_cfg *i2s_config;
    struct clk *i2s_clk = RT_NULL;
    struct clk *i2s_pclk = RT_NULL;
    struct clk *acodec_mclk = RT_NULL;
    struct clk *acodec_pclk = RT_NULL;
    rt_device_t i2s;
    int ret = 0;

    if (priv_data)
        fh_i2s_data = (struct fh_i2s_platform_data *)priv_data;
    else
        return -EINVAL;

    i2s_dev.channel_assign.capture_channel = fh_i2s_data->dma_capture_channel;
    i2s_dev.channel_assign.playback_channel = fh_i2s_data->dma_playback_channel;
    i2s_dev.dma_master = fh_i2s_data->dma_master;

    if (fh_i2s_data->i2s_clk == NULL)
        fh_i2s_data->i2s_clk = "i2s_clk";

    i2s_clk = clk_get(NULL, fh_i2s_data->i2s_clk);
    if (i2s_clk == NULL)
        rt_kprintf("i2s clk get fail\n");
    else
        clk_enable(i2s_clk);

    if (fh_i2s_data->i2s_pclk)
    {
        i2s_pclk = clk_get(NULL, fh_i2s_data->i2s_pclk);
        if (i2s_pclk == NULL)
            rt_kprintf("i2s_pclk get fail\n");
        else
            clk_enable(i2s_pclk);
    }

    if (fh_i2s_data->acodec_mclk)
    {
        acodec_mclk = clk_get(NULL, fh_i2s_data->acodec_mclk);
        if (acodec_mclk == NULL)
            rt_kprintf("acodec_mclk get fail\n");
        else
            clk_enable(acodec_mclk);
    }

    if (fh_i2s_data->acodec_pclk)
    {
        acodec_pclk = clk_get(NULL, fh_i2s_data->acodec_pclk);
        if (acodec_pclk == NULL)
            rt_kprintf("acodec_pclk get fail\n");
        else
            clk_enable(acodec_pclk);
    }

    i2s_config = rt_malloc(sizeof(struct fh_audio_cfg));
    if (i2s_config == NULL)
    {
        rt_kprintf("%s no mem %d\n", __func__,__LINE__);
        return -ENOMEM;
    }
    rt_memset(i2s_config, 0, sizeof(struct fh_audio_cfg));

    i2s_dev.reg_base = I2S_REG_BASE;

    fh_pmu_reset_i2s();
#ifdef CONFIG_MOL_I2S_SLAVE
    I2S_SET_REG_M(I2S_CTRL0, I2S_SLAVE_MODE, I2S_SLAVE_MODE);
#endif
    /* reset FIFO */
    I2S_SET_REG(I2S_TX_FIFO, 0xffffffff);
    I2S_SET_REG(I2S_TX_FIFO, 0xffffffff);

    init_i2s_mutex(i2s_config);

    i2s = rt_malloc(sizeof(struct rt_device));
    if (i2s == RT_NULL)
    {
        rt_kprintf("%s no mem %d\n", __func__,__LINE__);
        rt_free(i2s_config);
        return -ENOMEM;
    }

    rt_memset(i2s, 0, sizeof(struct rt_device));

    i2s_dev.audio_config = i2s_config;
    i2s->user_data       = i2s_config;
    i2s->type            = RT_Device_Class_Sound;

#ifdef RT_USING_DEVICE_OPS
    i2s->ops = &i2s_ops;
#else
    i2s->open            = fh_audio_open;
    i2s->read            = fh_audio_read;
    i2s->write           = fh_audio_write;
    i2s->close           = fh_audio_close;
    i2s->control         = fh_audio_ioctl;
#endif

    ret = rt_device_register(i2s, "fh_audio", RT_DEVICE_FLAG_RDWR);
    if (ret)
    {
        rt_kprintf("i2s device register error %d\n", ret);
        return ret;
    }

    return 0;
}

static int fh_i2s_exit(void *priv_data)
{
    return 0;
}

static struct fh_board_ops fh_i2s_ops = {
    .probe = fh_i2s_probe,
    .exit = fh_i2s_exit,
};

int fh_i2s_init(void)
{
    return fh_board_driver_register("fh_i2s", &fh_i2s_ops);
}

#ifdef I2S_SELFTEST

#define TEST_FN "/i2s.dat"
#define BUFF_SIZE_I2S 1024 * 8
static rt_uint32_t rx_buff[BUFF_SIZE_I2S] __attribute__((aligned(32)));
static rt_uint32_t tx_buff[BUFF_SIZE_I2S] __attribute__((aligned(32)));

extern int fh_pinctrl_sdev(char *devname, unsigned int flag);

static int fh_i2s_test(int argc, char **argv)
{
    rt_device_t i2s_dev;
    struct fh_audio_cfg_arg cfg;
    int i;
    int select;
    int ret = 0;
    int chunk_bytes = 0;
    int rate = 16000;
    int bit = 16;

    if (argc > 1)
        rate = atoi(argv[1]);
    if (argc > 2)
        bit = atoi(argv[2]);

    rt_kprintf("%s rate:%d bit:%d\n", __func__, rate, bit);

    fh_pinctrl_sdev("DWI2S", 0);

    i2s_dev = (rt_device_t)rt_device_find("fh_audio");

    for (i = 0; i < BUFF_SIZE_I2S; i++)
        tx_buff[i] = i;

    rt_device_open(i2s_dev, 0);

    cfg.buffer_size = BUFF_SIZE_I2S;
    cfg.channels    = 0;
    cfg.rate        = rate;
    cfg.frame_bit   = bit;

    cfg.period_size = BUFF_SIZE_I2S / 8;

    chunk_bytes = frames_to_bytes(cfg.frame_bit, cfg.period_size);

    ret = rt_device_control(i2s_dev, AC_INIT_CAPTURE_MEM, &cfg);

    if (ret)
    {
        rt_kprintf("AC_INIT_CAPTURE_MEM err %d\n", ret);
        goto out;
    }

    ret = rt_device_control(i2s_dev, AC_AI_EN, &cfg);
    if (ret)
    {
        rt_kprintf("AC_AI_EN err %d\n", ret);
        goto out;
    }

    ret = rt_device_control(i2s_dev, AC_INIT_PLAYBACK_MEM, &cfg);
    if (ret)
    {
        rt_kprintf("AC_INIT_PLAYBACK_MEM err %d\n", ret);
        goto out;
    }

    ret = rt_device_control(i2s_dev, AC_AO_EN, &cfg);
    if (ret)
    {
        rt_kprintf("AC_AO_EN err %d\n", ret);
        goto out;
    }

    struct timespec timeout;

    timeout.tv_sec  = 1;
    timeout.tv_nsec = 0;

    for (i = 0; i < 10; i++)
    {
/*
 *      select = rt_device_control(i2s_dev, AC_READ_SELECT, &timeout);
 *      if (select < 0)
 *          PRINT_I2S_DBG("read poll error %d\n", select);
 */
        ret = rt_device_read(i2s_dev, 0, &rx_buff[0], chunk_bytes);
        if (ret <= 0)
        {
            rt_kprintf("read err %d\n", ret);
            goto out;
        }

        rt_kprintf("%d receive: %08x %08x %08x %08x\n", ret,
            rx_buff[0], rx_buff[1], rx_buff[2], rx_buff[3]);
/*
 *      select = rt_device_control(i2s_dev, AC_WRITE_SELECT, &timeout);
 *      if (select < 0)
 *          PRINT_I2S_DBG("write poll error %d\n", select);
 */
        ret = rt_device_write(i2s_dev, 0, &tx_buff[0], chunk_bytes);
        if (ret <= 0)
        {
            rt_kprintf("write err %d\n", ret);
            goto out;
        }
    }

out:
    rt_device_close(i2s_dev);

    return ret;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(fh_i2s_test, fh_i2s_test(rate, bit));
#endif

#endif
