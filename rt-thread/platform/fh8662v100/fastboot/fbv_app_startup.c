#include "fbv_common.h"
int __fastvideo_init(void)
{
    int ret = 0;
    fbv_time_stat_init("fbv_time");
    fbv_time_stat_add("fbv_time", "Dsp init start");
    ret = fbv_dsp_init();
    fbv_time_stat_add("fbv_time", "Dsp init end");
    fbv_time_stat_add("fbv_time", "Isp init start");
    ret  = fbv_isp_init();
    fbv_time_stat_add("fbv_time", "Isp init end");
    fbv_isp_start_proc();
    return ret;
}

int __fastvideo_startup(void)
{
    int ret = 0;
    ret = fbv_dsp_enc_init();
    return ret;
}