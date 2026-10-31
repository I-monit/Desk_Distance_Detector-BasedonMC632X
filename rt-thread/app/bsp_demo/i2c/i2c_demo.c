#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <rttshell.h>
#include "fh_i2c_dev.h"

#define I2C_DEVICE "/dev/i2c0"

void i2c_demo_init(void)
{
    int fd;
    int ret;

    fd = open(I2C_DEVICE, O_RDWR, 0);
    if (fd < 0)
    {
        printf("Error: open /dev/i2c fail\n");
        return;
    }

    ret = ioctl(fd, FH_I2C_DEV_CTRL_10BIT, 0);
    if (ret < 0)
    {
        printf("[i2c demo] Unable to set I2C_TENBIT!\n");
        goto Exit;
    }

    struct fh_i2c_priv_data data;
    struct fh_i2c_msg msg[2];
    uint8_t send_buf[2] = {0x0};
    uint8_t recv_buf[1] = {0x0};

    send_buf[0]  = (0x40 & 0xff);
    send_buf[1]  = 0x22;

    msg[0].addr  = 0x80>>1;
    msg[0].flags = FH_I2C_WR;
    msg[0].buf   = send_buf;
    msg[0].len   = 2;

    data.msgs   = msg;
    data.msg_num = 1;
    ret = ioctl(fd, FH_I2C_DEV_CTRL_RW, &data);
    if (ret)
    {
        printf("[i2c_demo] Write i2c fail\n");
        goto Exit;
    }

    send_buf[0]  = (0x40 & 0xff);
    msg[0].addr  = (0x80 >> 1);
    msg[0].flags = FH_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = send_buf;

    msg[1].addr   = (0x80 >> 1);
    msg[1].flags  = FH_I2C_RD;
    msg[1].len    = 1;
    msg[1].buf    = recv_buf;

    data.msgs   = msg;
    data.msg_num = 2;
    ret = ioctl(fd, FH_I2C_DEV_CTRL_RW, &data);
    if (ret)
    {
        printf("[i2c_demo] Write i2c fail\n");
        goto Exit;
    }

    printf("[i2c demo] data: %x\n", recv_buf[0]);

Exit:
    close(fd);
}
