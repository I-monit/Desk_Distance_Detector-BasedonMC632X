#ifndef __MEDIA_PROC_H__
#define __MEDIA_PROC_H__

// System Include
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define VPU_PROC "/proc/driver/vpu"
#define ENC_PROC "/proc/driver/enc"
#define JPEG_PROC "/proc/driver/jpeg"
#define BGM_PROC "/proc/driver/bgm"
#define ISP_PROC "/proc/driver/isp"
#define MIPI_PROC "/proc/driver/mipi"
#define TRACE_PROC "/proc/driver/trace"
#define VICAP_PROC "/proc/driver/vicap"

#define WR_PROC_DEV(device, cmd) write_media_proc(device, cmd)

void write_media_proc(char *mod, char *config);

#endif
