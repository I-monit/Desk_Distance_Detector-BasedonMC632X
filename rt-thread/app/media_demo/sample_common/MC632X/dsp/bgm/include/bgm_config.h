#ifndef __BGM_CONFIG_H__
#define __BGM_CONFIG_H__
#include "sample_common_media.h"
#include "vpu_chn_config.h"
#include "fh_trip_area.h"

#define BGM_DEBUG 0

#define OPEN_FILE(file, filename, flag)         \
    do                                          \
    {                                           \
        file = fopen(filename, flag);           \
        if (file == NULL)                       \
        {                                       \
            printf("open %s fail\n", filename); \
            goto Exit;                          \
        }                                       \
    } while (0)

#define CLOSE_FILE(file)  \
    do                    \
    {                     \
        if (file != NULL) \
            fclose(file); \
    } while (0)

FH_SINT32 get_bgm_initStatus(FH_SINT32 grp_id);

FH_SINT32 bgm_init(FH_SINT32 grp_id);

FH_SINT32 bgm_exit(FH_SINT32 grp_id);

#endif