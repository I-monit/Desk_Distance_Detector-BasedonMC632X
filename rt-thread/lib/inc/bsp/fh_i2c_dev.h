#ifndef __FH_I2C_DEV_H__
#define __FH_I2C_DEV_H__

#include <stdint.h>

#define FH_I2C0_DEVICE_NAME "i2c0"
#define FH_I2C1_DEVICE_NAME "i2c1"

#define FH_I2C_DEV_CTRL_10BIT        0x20
#define FH_I2C_DEV_CTRL_ADDR         0x21
#define FH_I2C_DEV_CTRL_TIMEOUT      0x22
#define FH_I2C_DEV_CTRL_RW           0x23

#define FH_I2C_WR                0x0000
#define FH_I2C_RD               (1u << 0)

struct fh_i2c_msg
{
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t  *buf;
};

struct fh_i2c_priv_data
{
    struct fh_i2c_msg  *msgs;
    uint32_t  msg_num;
};

#endif
