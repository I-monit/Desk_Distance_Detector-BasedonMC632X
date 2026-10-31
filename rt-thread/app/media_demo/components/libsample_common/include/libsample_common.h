#ifndef __LIBSAMPLE_COMMON_H__
#define __LIBSAMPLE_COMMON_H__
#include "sample_common_media.h"
#ifdef __LINUX_OS__
#include <sys/time.h>
#endif

FH_SINT32 openFile(FILE **fd, FH_CHAR *filename);

FH_VOID* readFile(FH_CHAR *filename);

FH_VOID *readFile_with_len(FH_CHAR *filename, FH_UINT32 *len);

FH_SINT32 readFileToAddr(FH_CHAR *filename, FH_VOID* addr);

FH_SINT32 getFileSize(FH_CHAR *path);

FH_SINT32 readFileRepeatByLength(FILE *fp, FH_UINT32 length, FH_ADDR addr);

FH_SINT32 saveDataToFile(FH_ADDR data, FH_UINT32 dataSize, FH_CHAR *fileName);

FH_UINT64 get_us(FH_VOID);

FH_UINT64 get_ms(FH_VOID);

FH_VOID nv12_to_yuy2(FH_CHAR *nv12_ydata, FH_CHAR *nv12_uvdata, FH_CHAR *yuy2_data, FH_SINT32 width, FH_SINT32 height);

#ifdef __RTTHREAD_OS__
extern unsigned long long read_pts(void);
#endif
#endif // __LIBSAMPLE_COMMON_H__
