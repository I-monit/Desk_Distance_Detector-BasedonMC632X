#ifndef __SAMPLE_COMMON_STREAM_H__
#define __SAMPLE_COMMON_STREAM_H__
#include "sample_common_media.h"
#include "enc_config.h"
#include "vpu_config.h"
#include "vpu_bind_info.h"
#include "jpeg_config.h"

#define HTTP_MJPEG_PORT (1111)
#define YUV_BUF_NUM 3

enum stream_type
{
    FH_PES = 0x1,
    FH_RTSP = 0x2,
    FH_HTTP = 0x4,
    FH_RAW = 0x8,
};

typedef struct stream_info
{
    FH_UINT32 grpid;
    enum stream_type type;
} STREAM_INFO;

FH_SINT32 sample_common_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port);

FH_SINT32 sample_common_dmc_deinit(FH_VOID);

FH_SINT32 sample_common_start_get_stream(FH_VOID);

FH_SINT32 sample_common_stop_get_stream(FH_VOID);

FH_SINT32 sample_common_start_send_stream(FH_VOID);

FH_SINT32 sample_common_stop_send_stream(FH_VOID);
#endif // __SAMPLE_COMMON_STREAM_H__
