#ifndef __FASTVIDEO_CONFIG_H__
#define __FASTVIDEO_CONFIG_H__

#define FBV_GRP_NUM         (2)

#define NN_ENABLE
#define NN_DETECT_WIDTH     (512)
#define NN_DETECT_HEIGHT    (288)
#define NN_DETECT_CHAN      (2)
#define NN_DETECT_FRAMES    (3)

#define JPEG_ENC_CHNUM      (4)

#define MULTI_SENSOR
#define RT_USING_CG2083_MIPI

/*#define RT_USING_OVOS02K_MIPI*/

#if defined(RT_USING_CG2083_MIPI) || defined(RT_USING_OVOS02K_MIPI)
/* isp configuration */
#define ISP_FORMAT          FORMAT_1080P30
#define ISP_INIT_WIDTH      1920
#define ISP_INIT_HEIGHT     1088

/* dsp configuration */
#define VIDEO_INPUT_WIDTH   1920
#define VIDEO_INPUT_HEIGHT  1088

/* channel 0 configuration */
#define CH0_WIDTH           1920
#define CH0_HEIGHT          1088
#define CH0_BIT_RATE        (2048 * 1024)
#define CH0_FRAME_COUNT     30
#define CH0_FRAME_TIME      1
#define CHN0_DEFAULT_VOFMT (VPU_VOMODE_SCAN)

/* channel 1 configuration */
#define CH1_WIDTH           720
#define CH1_HEIGHT          576
#define CH1_BIT_RATE        (1024 * 1024)
#define CH1_FRAME_COUNT     25
#define CH1_FRAME_TIME      1

/* channel 2 configuration */
#define CH2_WIDTH           352
#define CH2_HEIGHT          288
#define CH2_BIT_RATE        (512 * 1024)
#define CH2_FRAME_COUNT     25
#define CH2_FRAME_TIME      1

#define JPEG_INIT_WIDTH     (CH0_WIDTH)
#define JPEG_INIT_HEIGHT    (CH0_HEIGHT)

#endif /*RT_USING_CG2083_MIPI*/

#endif /*__FASTVIDEO_CONFIG_H__*/
