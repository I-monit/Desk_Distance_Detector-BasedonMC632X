#ifndef _OSD_LOG_H_
#define _OSD_LOG_H_

#define OSD_LOG_EN 0

#if OSD_LOG_EN
#define OSD_LOG printf
#else
#define OSD_LOG(...)
#endif  // OSD_LOG_EN

#endif  // !_OSD_LOG_H_