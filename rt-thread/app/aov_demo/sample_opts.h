#ifndef __SAMPLE_OPTS_H__
#define __SAMPLE_OPTS_H__

struct channel_info
{
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT8 frame_count;
    FH_UINT8 frame_time;
    FH_UINT32 bps;
};

#define CHANNEL_COUNT 2

/* #define OPEN_RECORD */
/* #define FH_USING_STREAM_UDP */

#endif /* __SAMPLE_OPTS_H__ */
