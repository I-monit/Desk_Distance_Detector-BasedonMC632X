#ifndef __lib_http_mjpeg_h__
#define __lib_http_mjpeg_h__
#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *MJPEG_HANDLE;

FH_SINT32 libmjpeg_start_server(MJPEG_HANDLE *handle, FH_UINT16 id, FH_UINT16 port);
extern FH_VOID libmjpeg_stop_server(MJPEG_HANDLE *handle);
extern FH_VOID libmjpeg_send_stream(MJPEG_HANDLE *handle, FH_VOID *data, FH_UINT32 size);

#ifdef __cplusplus
}
#endif

#endif /*__lib_http_mjpeg_h__*/
