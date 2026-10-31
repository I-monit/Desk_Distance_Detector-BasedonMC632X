#ifndef __CONFIG_H__
#define __CONFIG_H__

#define JPEG_CHN_OFFSET 3
#define MAX_VPU_CHN_NUM 3
#define MAX_GRP_NUM 2
/*Automatically generated make config: don't edit */
/*Linux Kernel Configuration */
/*Fri Dec 13 14:34:40 2024 */

#define FH_APP_GRP_ID 0
/*DUAL_CAMERA is not set */
#define FH_USING_PS5270_MIPI
/*FH_USING_OVOS02K_MIPI is not set */
/*FH_USING_SC3336_MIPI is not set */
/*FH_USING_OVOS04C10_MIPI is not set */
/*ENABLE_NNA_DET is not set */
/*ENBALE_SMALL_TO_BIG is not set */
/*FH_APP_USING_FORMAT_192X192P220_G9 is not set */
/*FH_APP_USING_FORMAT_384X384P110_G9 is not set */
#define VIDEO_GRP0

/*ISP Config */

#define ISP_WIDTH_G0 1536
#define ISP_HEIGHT_G0 1536
/*FH_APP_USING_FORMAT_1080P30_G0 is not set */
/*FH_APP_USING_FORMAT_1080P20_G0 is not set */
/*FH_APP_USING_FORMAT_1080P20_SLAVE_G0 is not set */
/*FH_APP_USING_FORMAT_2304X1296P15_G0 is not set */
/*FH_APP_USING_FORMAT_1536X1536P15_G0 is not set */
#define FH_APP_USING_FORMAT_1536X1536P30_G0
#define ISP_FORMAT_G0 FORMAT_1536X1536P30
#define VI_INPUT_WIDTH_G0 1536
#define VI_INPUT_HEIGHT_G0 1536
/*FH_APP_USING_FORMAT_2688X1520P25_G0 is not set */
/*FH_APP_USING_FORMAT_2688X1520P30_G0 is not set */

/*VPU Conifg */

#define CH0_ENABLE_G0
#define VPU_WIDTH_CH0_G0 1536
#define VPU_HEIGHT_CH0_G0 1536
#define VPU_BUF_NUM_CH0_G0 3
#define VENC_ENABLE_G0_CH0
#define CH0_BIT_RATE_G0 2048
#define CH0_FRAME_COUNT_G0 30
#define CH0_FRAME_TIME_G0 1
#define FH_CH0_USING_SAMPLE_H264_G0
/*FH_CH0_USING_SAMPLE_H265_G0 is not set */
/*CH1_ENABLE_G0 is not set */
#define CH2_ENABLE_G0
#define VPU_WIDTH_CH2_G0 512
#define VPU_HEIGHT_CH2_G0 288
#define VPU_BUF_NUM_CH2_G0 3
/*VIDEO_GRP1 is not set */

#endif
