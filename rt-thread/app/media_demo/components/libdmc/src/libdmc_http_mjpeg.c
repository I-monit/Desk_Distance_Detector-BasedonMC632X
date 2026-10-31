#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libdmc/include/libdmc.h"
#include "libdmc/include/libdmc_http_mjpeg.h"
#include <libmjpeg_over_http/include/libhttp_mjpeg.h>
#include "jpeg_config.h"

static MJPEG_HANDLE g_mjpeg_server[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static unsigned int g_mjpeg_port[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static unsigned int g_printed[MAX_GRP_NUM * MAX_VPU_CHN_NUM];

static int _http_mjpeg_input_fn(int media_chn,
                                int media_type,
                                int media_subtype,
                                unsigned long long frame_pts,
                                unsigned char *frame_data,
                                int frame_len,
                                int frame_end_flag)
{
    libmjpeg_send_stream(&g_mjpeg_server[MAX_MJPEG_CHN - media_chn], frame_data, frame_len);

    if (!g_printed[MAX_MJPEG_CHN - media_chn])
    {
        printf("http_mjpeg: listen on port %d ... \n", g_mjpeg_port[MAX_MJPEG_CHN - media_chn]);
        g_printed[MAX_MJPEG_CHN - media_chn] = 1;
    }

    return 0;
}

FH_SINT32 dmc_http_mjpeg_subscribe(FH_UINT16 port)
{
    int ret = 0;
    ENC_INFO *mjpeg_info = NULL;
    int mjpeg_max_chn = MAX_MJPEG_CHN;
    int mjpeg_grp_code = 0;
    int index = 0;

    for (int grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        for (int chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
        {
            mjpeg_info = get_mjpeg_config(grp_id, chn_id);
            if (mjpeg_info->enable)
            {
                for (; index < MAX_GRP_NUM * MAX_VPU_CHN_NUM; index++)
                {
                    if (g_mjpeg_server[index] == NULL)
                    {
                        g_mjpeg_port[index] = port + index;
                        ret = libmjpeg_start_server(&g_mjpeg_server[index], index, g_mjpeg_port[index]);
                        if (ret)
                        {
                            printf("libmjpeg_start_server failed, ret = %d!\n", ret);
                            return ret;
                        }
                        g_printed[index] = 0;
                        printf("[MJPEG SERVER]Start MJPEG Server[%d] GRP[%d] CHN[%d] MJPEG CHN[%d]!\n", index, grp_id, chn_id, mjpeg_max_chn);
                        mjpeg_grp_code |= 1 << (mjpeg_max_chn / MAX_VPU_CHN_NUM);
                        mjpeg_max_chn--;
                        break;
                    }
                }
            }
        }
    }
    ret = dmc_subscribe("HTTP_MJPEG", mjpeg_grp_code, DMC_MEDIA_TYPE_MJPEG, _http_mjpeg_input_fn);
    if (ret)
    {
        printf("dmc_subscribe failed, ret = %d!\n", ret);
    }

    return ret;
}

FH_SINT32 dmc_http_mjpeg_unsubscribe(FH_VOID)
{
    dmc_unsubscribe("HTTP_MJPEG", DMC_MEDIA_TYPE_MJPEG);

    for (int index = 0; index < MAX_GRP_NUM * MAX_VPU_CHN_NUM; index++)
    {
        if (g_mjpeg_server[index] != NULL)
        {
            libmjpeg_stop_server(&g_mjpeg_server[index]);
            g_mjpeg_server[index] = NULL;
            g_printed[index] = 0;
            g_mjpeg_port[index] = 0;
        }
    }

    return 0;
}
