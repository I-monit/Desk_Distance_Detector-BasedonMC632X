/*****************************************************************************
*	Copyright (c) 2010-2017 Shanghai Fullhan Microelectronics Co., Ltd.
*						All Rights Reserved. Confidential.
******************************************************************************/
#ifndef _OSD_LOG_H_
#define _OSD_LOG_H_

// #define OSD_DEBUG

#ifdef OSD_DEBUG
#define OSD_LOG printf
#else
#define OSD_LOG(...)
#endif

#endif  // _OSD_LOG_H_