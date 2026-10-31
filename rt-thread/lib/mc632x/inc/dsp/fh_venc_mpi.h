#ifndef __FH_VENC_MPI_H__
#define __FH_VENC_MPI_H__
/**VENC**/
#include "fh_venc_mpipara.h"
#include "types/type_def.h"
#include "fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/*
*   Name: FH_VENC_SetAOV_INFO
*
*   Parameters:
*       [in] 透传数据inf0-3.
*
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*       在AOV模式下，arc rtt侧调用
*       
*/
FH_SINT32 FH_VENC_SetAOV_INFO(FH_UINT32 inf0,FH_UINT32 inf1,FH_UINT32 inf2,FH_UINT32 inf3);

/*
*   Name: FH_VENC_SetLoopCnt
*         RTT模式下，设置online模式下编码veulpcnt。替代linux下loadko传参  insmod venc.ko veulpcnt=8
*         veulpcnt每加一个值就是中间缓存buf的高度增加64行。 设置成8就是512的高。对应开辟的内存也增大了。
*
*   Parameters:
*
*       [in] FH_SINT32 lpCnt
*            veulpcnt的cnt数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*        RTT系统在所有通道创建之前调用，只需要调用一次。
*        Linux系统此接口不支持。通过loadko传参  insmod venc.ko veulpcnt=8
*/
FH_SINT32 FH_VENC_SetLoopCnt(FH_SINT32 lpCnt);

/*
*   Name: FH_VENC_SetAOVStreamBufFull_threshold
*         AOV RTT模式下，设置码流Buffer即将满的百分比。例如设30.则码流buff还剩余30%时。将码流即将满的标志位置为1.
*         APP可以通过FH_VENC_GetChnStatus查询到此allmost full flag
*
*   Parameters:
*
*       [in] FH_UINT32 percentage
*            剩余free水位百分比。范围1-99
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*        AOV RTT系统只需要调用一次。如果不设置，默认剩余水位30%
* 
*/

FH_SINT32 FH_VENC_SetAOVStreamBufFull_threshold(FH_UINT32 percentage);

/*
*   Name: FH_VENC_Set_Autopafterrst
*         RTT模式下，打开此模式lpreset后,根据硬件状态自动判决是否使用P帧模式。替代linux下loadko传参  insmod venc.ko autopafterrst=1
*         此接口在RTT系统调用
*
*   Parameters:
*
*       [in] FH_SINT32 enable
*            1,开启venc支持Autopafterrst模式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*        RTT系统在所有通道创建之前调用，只需要调用一次。
*        Linux系统此接口不支持。通过loadko传参  insmod venc.ko autopafterrst=1
*/
FH_SINT32 FH_VENC_Set_Autopafterrst(FH_SINT32 enable);

/*
*   Name: FH_VENC_SetVicapofflinemode
*         RTT模式下，设置vicap处于offline模式。替代linux下loadko传参  insmod venc.ko gvicapoffline=1
*         设置此参数后，vicap offline模式下，支持vpu-veu 直通联动。目前不支持vicap online和offline通道同时共存
*
*   Parameters:
*
*       [in] FH_SINT32 enable
*            1,开启venc支持vicap offline模式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*        RTT系统在所有通道创建之前调用，只需要调用一次。
*        Linux系统此接口不支持。通过loadko传参  insmod venc.ko gvicapoffline=1
*/
FH_SINT32 FH_VENC_SetVicapofflinemode(FH_SINT32 enable);


/*
*   Name: FH_VENC_CreateChn
*            创建编码通道，指定通道支持的最大编码幅面，和编码格式。
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VENC_CHN_CAP *stVencChnAttr
*            通道属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          - 按编码处理的最大能力创建,可以同时支持多种不同的编码类型,幅面不能超过硬件最大能力。
*          - 支持多种编码类型时，分配内存时按各编码类型中最大的一块内存进行内存分配，在设置通道属性时将这块内存给相应编码驱动使用。
*          - 驱动默认开启的通道数非最大通道数, 如使用比较大的通道号时可以通过maxencnum的命令对通道数量进行调整。
*/
FH_SINT32 FH_VENC_CreateChn(FH_UINT32 chan,const FH_VENC_CHN_CAP *stVencChnAttr);

/*
*   Name: FH_VENC_DestroyChn
*            销毁编码通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          - 销毁不存在通道,返回失败。
*          - 需对应OS平台VMM模块支持释放单块内存时此接口有效。
*/
FH_SINT32 FH_VENC_DestroyChn(FH_UINT32 chan);


/*
*   Name: FH_VENC_SetChnAttr
*            设置编码通道属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VENC_CHN_CONFIG *pstVencChnAttr
*            编码通道属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          - 调用此接口需要先调用FH_VENC_StopRecvPic停止编码通道(JPEG抓图通道不需要stop/start)。
*          - 在切换编码类型时通道绑定关系会失效,按通道缓存的码流会被清空。
*          - 设置通道属性后原有编码属性会失效需要重新设置(如ROI,多slice,旋转等)。
*          - 设置通道属性后会重置RC统计并强制I帧
*/
FH_SINT32 FH_VENC_SetChnAttr(FH_UINT32 chan,const FH_VENC_CHN_CONFIG *pstVencChnAttr);

/*
*   Name: FH_VENC_GetChnAttr
*            获取编码通道属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_VENC_CHN_CONFIG *pstVencChnAttr
*            编码通道属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetChnAttr(FH_UINT32 chan, FH_VENC_CHN_CONFIG *pstVencChnAttr);

/*
*   Name: FH_VENC_GetChnFd
*            打开一个设备fd，并绑定到通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_SINT32 *fd
*            设备fd
*
*   Return:
*           0(成功)
*          非0(失败)
*/
FH_SINT32 FH_VENC_GetChnFd(FH_UINT32 chan, FH_SINT32 *fd);

/*
*   Name: FH_VENC_CloseChnFd
*            关闭通道绑定的设备fd，并解绑
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败)
*/
FH_SINT32 FH_VENC_CloseChnFd(FH_UINT32 chan);

/*
*   Name: FH_VENC_StartRecvPic
*            通道开启接收图像数据
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          - 通道为JPEG抓图模式时，不需要调用FH_VENC_StartRecvPic/ FH_VENC_StopRecvPic。
*/
FH_SINT32 FH_VENC_StartRecvPic(FH_UINT32 chan);

/*
*   Name: FH_VENC_StopRecvPic
*            通道停止接收图像数据
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 通道为JPEG抓图模式时，不需要调用FH_VENC_StartRecvPic/ FH_VENC_StopRecvPic。
*/
FH_SINT32 FH_VENC_StopRecvPic(FH_UINT32 chan);

/*弃用接口,请调用FH_VENC_Submit_ENC_Ex*/
FH_SINT32 FH_VENC_Submit_ENC(FH_UINT32 chan,const FH_ENC_FRAME *pstVencsubmitframe);

/*
*   Name: FH_VENC_Submit_ENC_Ex
*            向编码通道提交图像数据进行编码
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_ENC_FRAME *pstVencsubmitframe
*            图像数据信息
*
*       [in] FH_VPU_VO_MODE datamode
*            图像数据格式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          -　此接口逐步弃用中,建议用户切换至FH_VENC_SendFrame,注意事项同FH_VENC_SendFrame。
*/
FH_SINT32 FH_VENC_Submit_ENC_Ex(FH_UINT32 chan,const FH_ENC_FRAME *pstVencsubmitframe,FH_VPU_VO_MODE datamode);

/*
*   Name: FH_VENC_SendFrame
*            向编码通道提交图像数据进行编码
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VIDEO_FRAME *pstVencsendframe
*            图像数据信息
*
*       [in] FH_UINT32 timeout_ms
*            超时时间,如提交数据时ENC输入队列满,会阻塞到队列空完成入队.此超时仅作为入队成功的阻塞，不代表该帧编码完成.
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          -　使用此接口进行离线数据编码时通道不应处于绑定状态,否则视频顺序会混乱。
*          -　提交数据的时间戳为0时驱动会自动赋值提交时的时间戳，不为0时前后帧时间戳不能相同。
*          -　另需满足<视频函数开发参考手册>　离线编码(非绑定模式)章节相关要求。
*/
FH_SINT32 FH_VENC_SendFrame(FH_SINT32 chan,const FH_VIDEO_FRAME *pstVencSendframe, FH_UINT32 timeout_ms);

/*
*   Name: FH_VENC_CheckAddr
*            检查VENC输入缓存地址是否被使用
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_PHYADDR addr
*            查询地址
*
*       [in] FH_UINT32 size
*            查询大小
*
*       [in] FH_UINT32 timeout_ms
*            接口阻塞时间(ms)
*
*       [in] FH_UINT32 isidle
*            是否正在被使用
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          -　此接口主要用于非VB内存,应用需要在编码完成时才能释放输入图像地址,否则会存在内存覆盖等问题。
*          -　VB内存时不需要通过接口保证硬件完成，用户可以直接在提交完成后释放相关内存。
*/
FH_SINT32 FH_VENC_CheckAddr(FH_UINT32 chan,FH_PHYADDR addr,FH_UINT32 size,FH_UINT32 timeout_ms,FH_UINT32 *isidle);

/*
*   Name: FH_VENC_GetChnStatus
*            查询通道状态信息
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_CHN_STATUS *pstVencStatus
*            通道状态信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 未配置通道属性时会返回错误
*/
FH_SINT32 FH_VENC_GetChnStatus(FH_UINT32 chan, FH_CHN_STATUS *pstVencStatus);

/*
*   Name: FH_VENC_GetStream
*            获取编码通道码流。非阻塞。
*
*   Parameters:
*
*       [in] FH_UINT32 request_type
*            需要获取的码流类型(支持的类型见枚举FH_STREAM_TYPE，可以支持查询多种类型的码流)
*
*       [out] FH_VENC_STREAM *pstVencstreamAttr
*            码流信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          -　此接口逐步弃用中,建议用户切换至FH_VENC_GetStream_Timeout,注意事项同FH_VENC_GetStream_Timeout。
*/
FH_SINT32 FH_VENC_GetStream (FH_UINT32 request_type,FH_VENC_STREAM *pstVencstreamAttr);

/*
*   Name: FH_VENC_GetStream_Block
*            获取编码通道码流。阻塞时间1s。
*
*   Parameters:
*
*       [in] FH_UINT32 request_type
*            需要获取的码流类型(支持的类型见枚举FH_STREAM_TYPE，可以支持查询多种类型的码流)
*
*       [out] FH_VENC_STREAM *pstVencstreamAttr
*            码流信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          -　此接口逐步弃用中,建议用户切换至FH_VENC_GetStream_Timeout,注意事项同FH_VENC_GetStream_Timeout。
*/
FH_SINT32 FH_VENC_GetStream_Block(FH_UINT32 request_type,FH_VENC_STREAM *pstVencstreamAttr);

/*
*   Name: FH_VENC_GetStream_Timeout
*            获取编码通道码流，自定义超时时间。
*
*   Parameters:
*
*       [in] FH_UINT32 request_type
*            需要获取的码流类型(支持的类型见枚举FH_STREAM_TYPE，可以支持查询多种类型的码流)
*
*       [out] FH_VENC_STREAM *pstVencstreamAttr
*            码流信息
*
*       [in] FH_UINT32 timeout_ms
*            超时时间,ms
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 建议用户使用前先阅读<视频函数开发参考手册>　编码码流内存章节。
*          - 此接口仅可获取使用共用缓冲池的编码通道的码流数据，独立通道缓存应使用FH_VENC_GetChnStream_Timeout。
*          - H264&H265在共用缓冲池模式时会公用一个缓冲池,因此FH_STREAM_H264和FH_STREAM_H265等价于(FH_STREAM_H264|FH_STREAM_H265)。
*          - MJPEG/JPEG在编码码流超过分配的缓冲区大小时会返回大小为0的码流,此时依然需要调用FH_VENC_ReleaseStream释放码流。
*          - 可以同时查询两者码流(例如FH_STREAM_JPEG|FH_STREAM_MJPEG)或直接查询所有编码模块(FH_STREAM_ALL)
*          - 支持多个线程查询不同类型码流，但多个线程不能获取同一类型码流。
*          - 例如：
*          - 正确：线程1：查询FH_STREAM_JPEG|FH_STREAM_MJPEG，线程2：查询FH_STREAM_H264
*          - 错误：线程1：查询FH_STREAM_JPEG|FH_STREAM_MJPEG，线程2：查询FH_STREAM_MJPEG
*/
FH_SINT32 FH_VENC_GetStream_Timeout(FH_UINT32 request_type,FH_VENC_STREAM *pstVencstreamAttr,FH_UINT32 timeout_ms);

/*
*   Name: FH_VENC_GetChnStream_Timeout
*            获取指定编码通道的码流，自定义超时时间。
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            需要获取的编码通道
*
*       [out] FH_VENC_STREAM *pstVencstreamAttr
*            码流信息
*
*       [in] FH_UINT32 timeout_ms
*            超时时间,ms
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 建议用户使用前先阅读<视频函数开发参考手册>　编码码流内存章节。
*          - 此接口仅可获取使用独立通道缓存的编码通道的码流数据，共用缓冲池应使用FH_VENC_GetStream_Timeout。
*          - H264&H265在共用缓冲池模式时会公用一个缓冲池,因此FH_STREAM_H264和FH_STREAM_H265等价于(FH_STREAM_H264|FH_STREAM_H265)。
*          - MJPEG/JPEG在编码码流超过分配的缓冲区大小时会返回大小为0的码流,此时依然需要调用FH_VENC_ReleaseStream释放码流。
*          - 支持多个线程查询不同通道码流，但多个线程不能获取同一通道码流。
*/
FH_SINT32 FH_VENC_GetChnStream_Timeout(FH_UINT32 chan,FH_VENC_STREAM *pstVencstreamAttr,FH_UINT32 timeout_ms);

/*
*   Name: FH_VJPEG_GetChnStream_Timeout
*            获取指定编码通道的码流，自定义超时时间。
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            需要获取的编码通道
*
*       [out] FH_VENC_STREAM *pstVencstreamAttr
*            码流信息
*
*       [in] FH_UINT32 timeout_ms
*            超时时间,ms
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 建议用户使用前先阅读<视频函数开发参考手册>　编码码流内存章节。
*          - 此接口仅可获取使用独立通道缓存的编码通道的码流数据，共用缓冲池应使用FH_VENC_GetStream_Timeout。
*          - H264&H265在共用缓冲池模式时会公用一个缓冲池,因此FH_STREAM_H264和FH_STREAM_H265等价于(FH_STREAM_H264|FH_STREAM_H265)。
*          - MJPEG/JPEG在编码码流超过分配的缓冲区大小时会返回大小为0的码流,此时依然需要调用FH_VENC_ReleaseStream释放码流。
*          - 支持多个线程查询不同通道码流，但多个线程不能获取同一通道码流。
*/
FH_SINT32 FH_VJPEG_GetChnStream_Timeout(FH_UINT32 chan,FH_VENC_STREAM *pstVencstreamAttr,FH_UINT32 timeout_ms);

/*
*   Name: FH_VENC_ReleaseStream
*            释放码流
*
*   Parameters:
*
*       [in] FH_VENC_STREAM *pstVencstreamAttr
*            码流结构体参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 此接口可以支持释放FH_VENC_GetStream_Timeout＆FH_VENC_GetChnStream_Timeout 2种不同码流输出模式的码流.
*          - 用户在释放码流前调用FH_VENC_GetStream_Timeout＆FH_VENC_GetChnStream_Timeout会获取到重复的码流.
*/
FH_SINT32 FH_VENC_ReleaseStream(FH_VENC_STREAM *pstVencstreamAttr);


/*
*   Name: FH_VENC_ReleaseStream
*            释放码流
*
*   Parameters:
*
*       [in] FH_VENC_STREAM *pstVencstreamAttr
*            码流结构体参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 此接口可以支持释放FH_VENC_GetStream_Timeout＆FH_VENC_GetChnStream_Timeout 2种不同码流输出模式的码流.
*          - 用户在释放码流前调用FH_VENC_GetStream_Timeout＆FH_VENC_GetChnStream_Timeout会获取到重复的码流.
*/
FH_SINT32 FH_VJPEG_ReleaseStream(FH_VENC_STREAM *pstVencstreamAttr);

/*
*   Name: FH_VENC_GetStreamBufConfig
*            设置编码码流缓存池信息
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in]  FH_VENC_STM_CONFIG ＊pstVencstreamCfg
*            码流缓存池信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 如果缓冲池模式为公共缓存池模式时，只有当通道设置通道属性配置编码类型后才能获取缓存池的大小和缓存帧数量。
*/
FH_SINT32 FH_VENC_GetStreamBufConfig(FH_UINT32 chan,FH_VENC_STM_CONFIG *pstVencstreamCfg);

/*
*   Name: FH_VENC_GetStreamBufConfig
*            设置编码码流缓存池信息
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in]  FH_VENC_STM_CONFIG ＊pstVencstreamCfg
*            码流缓存池信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 如果缓冲池模式为公共缓存池模式时，只有当通道设置通道属性配置编码类型后才能获取缓存池的大小和缓存帧数量。
*/
FH_SINT32 FH_VJPEG_GetStreamBufConfig(FH_UINT32 chan,FH_VENC_STM_CONFIG *pstVencstreamCfg);


/*
*   Name: FH_VENC_SetRotate
*            设置编码旋转属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in]  FH_ROTATE_OPS pstVencrotateinfo
*            旋转属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 各平台支持功能有所差异,详见《视频函数开发参考手册》 H264/H265 处理能力＆JPEG 处理能力章节中能力定义。
*
*1，venc不支持缩放，crop。直通下不支持旋转，非直通能90度和270度旋转。
*非直通要旋转的话，宽和高都要满足对齐要求：32对齐。
*2，设置旋转角度，通过调用	FH_VENC_SetRotate(chn,FH_RO_OPS_270);
*涉及FW内存资源申请和硬件配置，需要在FH_VENC_CreateChn之后,FH_VENC_SetChnAttr之前调用。
*3，不支持动态旋转。如果要去除旋转，需要销毁通道后，重新创建。
*/
FH_SINT32 FH_VENC_SetRotate(FH_UINT32 chan, FH_ROTATE_OPS pstVencrotateinfo);

/*
*   Name: FH_VENC_GetRotate
*            获取编码旋转属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_ROTATE_OPS *pstVencrotateinfo
*            旋转属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetRotate(FH_UINT32 chan, FH_ROTATE_OPS *pstVencrotateinfo);

/*
*   Name: FH_VENC_SetRoiMap
*            设置ROI Map
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_ROI_MAP *pstVencroimap
*            roi map参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - JPEG、MJPEG通道不支持此接口。
*          - ROI通过位图的形式设置，因此不支持获取ROI配置。
*          - 配置FH_VENC_SetChnAttr后之前ROI属性会失效。
*          - 用户需要确保配置的ROIMap的Level为合法值。
*/
FH_SINT32 FH_VENC_SetRoiMap(FH_UINT32 chan,const FH_ROI_MAP *pstVencroimap);

/*
*   Name: FH_VENC_SetRoiCfg
*            设置ROI属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_ROI *pstVencroiinfo
*            ROI配置
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - JPEG、MJPEG通道不支持此接口。
*          - ROI通过位图的形式设置，因此不支持获取ROI配置。
*          - 配置FH_VENC_SetChnAttr后之前ROI属性会失效。
*/
FH_SINT32 FH_VENC_SetRoiCfg(FH_UINT32 chan,const FH_ROI *pstVencroiinfo);

/*
*   Name: FH_VENC_ClearRoi
*            清除之前设置的ROI配置
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - JPEG、MJPEG通道不支持此接口。
*/
FH_SINT32 FH_VENC_ClearRoi(FH_UINT32 chan);

/*
*   Name: FH_VENC_RequestIDR
*            申请强制I帧
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - JPEG、MJPEG通道不支持此接口。
*          - 强制I帧的码率会计入码控统计,连续强制I帧会导致图像变差。
*/
FH_SINT32 FH_VENC_RequestIDR(FH_UINT32 chan);

/*
*   Name: FH_VENC_GetCurPts
*            获取当前时间戳
*
*   Parameters:
*
*       [out] FH_UINT64 *Systemcurpts
*            时间戳
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetCurPts(FH_UINT64 *Systemcurpts);


/*
*   Name: FH_VENC_SetRCAttr
*            设置码率控制参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VENC_RC_ATTR *pstVencrcattr
*            码率控制参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 推荐用户使用此接口来更新码控参数，和FH_VENC_SetChnAttr比可以保持编码通道属性.
*/
FH_SINT32 FH_VENC_SetRCAttr(FH_UINT32 chan,const FH_VENC_RC_ATTR *pstVencrcattr);

/*
*   Name: FH_VENC_GetRCAttr
*            获取码控参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_VENC_RC_ATTR *pstVencrcattr
*            码率控制参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetRCAttr(FH_UINT32 chan,FH_VENC_RC_ATTR *pstVencrcattr);

/*
*   Name: FH_VENC_SetRcChangeParam
*            实时调整部分码控参数，通过该接口设置时会保留之前的码率情况，不会重置码控和强制I帧
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VENC_RC_CHANGEPARAM *rcparam
*            码控参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 该接口可以用作一些对码率比较敏感的应用根据应用的反馈实时进行码率调整。
*          - 但调整时应注意尽可以的避免码率频繁震荡的来回设置，以免出现码率控制不准的情况。
*          - MJPEG/JPEG通道不支持该接口。
*/
FH_SINT32 FH_VENC_SetRcChangeParam(FH_UINT32 chan,const FH_VENC_RC_CHANGEPARAM *rcparam);

/*
*   Name: FH_VENC_GetRcChangeParam
*            获取当前实时调整支持的码控参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_VENC_RC_CHANGEPARAM *rcparam
*            码控参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetRcChangeParam(FH_UINT32 chan, FH_VENC_RC_CHANGEPARAM *pstVencrcattr);

/*
*   Name: FH_VENC_SetLostFrameAttr
*            设置丢帧策略参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_DROP_FRAME *pstVencdropattr
*            丢帧策略参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 目前支持以下3种模式的丢帧策略：
*          1. 用户设定的主动丢帧
*               - 作为用户自身丢帧策略的接口
*               - 作为在YUV通道一绑多情况下,不同通道的2次帧率控制
*          2. QP门限触发的丢帧(FixQP下不生效)
*          - 在码控QP调整到某个值(可能未到MAXQP时),优先降低帧率来降低码率,以帧率换取图像质量
*          3. 瞬时码率的丢帧(FixQP下不生效)
*               - 瞬时码率超过预设峰值时进行丢帧
*          - 用户可选择丢帧时是否按照pskip的方式编码。当非pskip编码丢帧时, 会出现时间戳的跳跃。
*          - 在开启跳帧参考时不支持开启pskip。
*/
FH_SINT32 FH_VENC_SetLostFrameAttr(FH_UINT32 chan,const FH_DROP_FRAME *pstVencdropattr);

/*
*   Name: FH_VENC_GetLostFrameAttr
*            获取丢帧策略参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_DROP_FRAME *pstVencdropattr
*            丢帧策略参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetLostFrameAttr(FH_UINT32 chan,FH_DROP_FRAME *pstVencdropattr);

/*
*   Name: FH_VENC_SetBkgQP
*            设置背景QP配置
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_UINT32 bkg_delta_qp
*            背景QP与前景QP之间的差值
*
*       [in] FH_UINT32 bkg_max_qp
*            背景QP的最大值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - MJPEG/JPEG通道不支持该接口。
*          - 在智能模式下生效.背景QP与前景QP之间的差值和背景QP的最大值,
*          - 当码控QP大于背景QP时，在QP分配上将退化为和普通编码一致;
*/
FH_SINT32 FH_VENC_SetBkgQP(FH_UINT32 chan,FH_UINT32 bkg_delta_qp,FH_UINT32 bkg_max_qp);

/*
*   Name: FH_VENC_GetBkgQP
*            获取背景QP配置
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_UINT32 *bkg_delta_qp
*            背景QP与前景QP之间的差值
*
*       [out] FH_UINT32 *bkg_max_qp
*            背景QP的最大值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetBkgQP(FH_UINT32 chan,FH_UINT32 *bkg_delta_qp,FH_UINT32 *bkg_max_qp);

/*
*   Name: FH_VENC_SetEncParam
*            配置一些编码参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_ENCPARAM_CMD cmd
*            命令号
*
*       [in] FH_ENC_PARAM_UNION *param
*            命令号对应结构体
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - Cmd命令不适用于所有类型编码。
*          - 此函数主要定义一些不常用的参数设置。以减少SDK的API数量和变动。
*          - 正常流程中不需要使用此接口,主要用于用户效果调试和一些特殊功能的实现等。
*/
FH_SINT32 FH_VENC_SetEncParam(FH_UINT32 chan,FH_ENCPARAM_CMD cmd,const FH_ENC_PARAM_UNION *param);

/*
*   Name: FH_VENC_GetEncParam
*            获取一些编码参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_ENCPARAM_CMD cmd
*            命令号
*
*       [out] FH_ENC_PARAM_UNION *param
*            命令号对应结构体
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetEncParam(FH_UINT32 chan,FH_ENCPARAM_CMD cmd,FH_ENC_PARAM_UNION *param);

/*
*   Name: FH_VENC_SetEncryptSeed
*            设置加密密钥种子
*
*   Parameters:
*
*       [in] FH_UINT32 hash_seed0
*            加密密钥种子(和支持人员确定)
*
*       [in] FH_UINT32 hash_seed1
*            加密密钥种子(和支持人员确定)
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - MJPEG/JPEG通道不支持该接口。
*          - 一般用户无需设置该接口，除非富瀚支持人员要求。
*/
FH_SINT32 FH_VENC_SetEncryptSeed(FH_UINT32 hash_seed0,FH_UINT32 hash_seed1);

/*
*   Name: FH_VENC_SetEncRefMode
*            设置编码跳帧参考
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VENC_REF_MODE *pVencrefmode
*            跳帧属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 建议用户使用前先阅读<视频函数开发参考手册>　跳帧参考章节。
*          - MJPEG/JPEG通道不支持该接口。
*          - 配置跳帧参考后会触发强制I帧。
*          - 跳帧参考开启时不能使用PSkip的功能。
*          - 跳帧参考开启可能需要额外的通道内存,使用时应在创建通道前通过FH_VENC_SetChnParam配置需要使用的跳帧参考模式。
*/
FH_SINT32 FH_VENC_SetEncRefMode(FH_UINT32 chan,const FH_VENC_REF_MODE *pVencrefmode);

/*
*   Name: FH_VENC_GetEncRefMode
*            获取编码跳帧参考
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_VENC_REF_MODE *pVencrefmode
*            跳帧属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          MJPEG/JPEG通道不支持该接口。
*/
FH_SINT32 FH_VENC_GetEncRefMode(FH_UINT32 chan, FH_VENC_REF_MODE *pVencrefmode);
/*
*   Name: FH_VENC_SetH264Entropy
*            设置熵编码类型
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H264_ENTROPY *pstVencentropy
*            熵编码属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口,。
*/
FH_SINT32 FH_VENC_SetH264Entropy(FH_UINT32 chan,const FH_H264_ENTROPY *pstVencentropy);
/*
*   Name: FH_VENC_GetH264Entropy
*            获取熵编码类型
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_H264_ENTROPY *pstVencentropy
*            熵编码属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH264Entropy(FH_UINT32 chan,FH_H264_ENTROPY *pstVencentropy);
/*
*   Name: FH_VENC_SetH264Dblk
*            设置Deblocking参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H264_DBLK *pstVencdblk
*            Deblocking参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口,含义参考H264协议。
*/
FH_SINT32 FH_VENC_SetH264Dblk(FH_UINT32 chan,const FH_H264_DBLK *pstVencdblk);
/*
*   Name: FH_VENC_GetH264Dblk
*            获取Deblocking参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_H264_DBLK *pstVencdblk
*            Deblocking参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH264Dblk(FH_UINT32 chan,FH_H264_DBLK *pstVencdblk);
/*
*   Name: FH_VENC_SetH264SliceSplit
*            设置多slice属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H264_SLICE_SPLIT *pstVencslicesplit
*            多slice属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口, slice数量不能超过16。
*/
FH_SINT32 FH_VENC_SetH264SliceSplit(FH_UINT32 chan,const FH_H264_SLICE_SPLIT *pstVencslicesplit);
/*
*   Name: FH_VENC_GetH264SliceSplit
*            获取多slice属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_H264_SLICE_SPLIT *pstVencslicesplit
*            多slice属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH264SliceSplit(FH_UINT32 chan,FH_H264_SLICE_SPLIT *pstVencslicesplit);
/*
*   Name: FH_VENC_SetH264IntraFresh
*            设置帧内刷新属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H264_INTRA_FRESH *pstVencintrafresh
*            帧内刷新属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*/
FH_SINT32 FH_VENC_SetH264IntraFresh(FH_UINT32 chan,const FH_H264_INTRA_FRESH *pstVencintrafresh);
/*
*   Name: FH_VENC_GetH264IntraFresh
*            获取帧内刷新属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_H264_INTRA_FRESH *pstVencintrafresh
*            帧内刷新属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH264IntraFresh(FH_UINT32 chan, FH_H264_INTRA_FRESH *pstVencintrafresh);
/*
*   Name: FH_VENC_SetH264VUI
*            设置H264 VUI语法参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H264_VUI *pstVencvui
*            VUI语法参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*          - 设置后下一个I帧生效，建议先获取默认参数后再修改对应参数。
*/
FH_SINT32 FH_VENC_SetH264VUI(FH_UINT32 chan,const FH_H264_VUI *pstVencvui);
/*
*   Name: FH_VENC_GetH264VUI
*            获取H264 VUI语法参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_H264_VUI *pstVencvui
*            VUI语法参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H264编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH264VUI(FH_UINT32 chan,FH_H264_VUI *pstVencvui);

/*
*   Name: FH_VENC_SetH265Entropy
*            设置熵编码类型
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H265_ENTROPY *pstVencentropy
*            熵编码属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_SetH265Entropy(FH_UINT32 chan,const FH_H265_ENTROPY *pstVencentropy);
/*
*   Name: FH_VENC_GetH265Entropy
*            获取熵编码类型
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_H265_ENTROPY *pstVencentropy
*            熵编码属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH265Entropy(FH_UINT32 chan,FH_H265_ENTROPY *pstVencentropy);

/*
*   Name: FH_VENC_SetH265Dblk
*            设置Deblocking参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H265_DBLK *pstVencdblk
*            Deblocking参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_SetH265Dblk(FH_UINT32 chan,const FH_H265_DBLK *pstVencdblk);
/*
*   Name: FH_VENC_GetH265Dblk
*            获取Deblocking参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_H265_DBLK *pstVencdblk
*            Deblocking参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH265Dblk(FH_UINT32 chan,FH_H265_DBLK *pstVencdblk);
/*
*   Name: FH_VENC_SetH265SliceSplit
*            设置多slice属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H265_SLICE_SPLIT *pstVencslicesplit
*            多slice属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_SetH265SliceSplit(FH_UINT32 chan,const FH_H265_SLICE_SPLIT *pstVencslicesplit);
/*
*   Name: FH_VENC_GetH265SliceSplit
*            获取多slice属性
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_H265_SLICE_SPLIT *pstVencslicesplit
*            多slice属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH265SliceSplit(FH_UINT32 chan,FH_H265_SLICE_SPLIT *pstVencslicesplit);
/*
*   Name: FH_VENC_SetH265IntraFresh
*            设置帧内刷新参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H265_INTRA_FRESH *pstVencintrafresh
*            帧内刷新参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_SetH265IntraFresh(FH_UINT32 chan,const FH_H265_INTRA_FRESH *pstVencintrafresh);
/*
*   Name: FH_VENC_GetH265IntraFresh
*            获取帧内刷新参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_H265_INTRA_FRESH *pstVencintrafresh
*            帧内刷新参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH265IntraFresh(FH_UINT32 chan, FH_H265_INTRA_FRESH *pstVencintrafresh);
/*
*   Name: FH_VENC_SetH265VUI
*            设置H265 VUI语法参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_H265_VUI *pstVencvui
*            VUI语法参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*          - 设置后下一个I帧生效，建议先获取默认参数后再修改对应参数。
*/
FH_SINT32 FH_VENC_SetH265VUI(FH_UINT32 chan,const FH_H265_VUI *pstVencvui);
/*
*   Name: FH_VENC_GetH265VUI
*            获取H265 VUI语法参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_H265_VUI *pstVencvui
*            VUI语法参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅H265编码支持该接口。
*/
FH_SINT32 FH_VENC_GetH265VUI(FH_UINT32 chan,FH_H265_VUI *pstVencvui);
/*
*   Name: FH_VENC_SetDeBreathEffect
*            设置去呼吸效应
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_DEBREATH *pstDeBreath
*            是否使能去呼吸效应功能
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 建议用户使用前先阅读<视频函数开发参考手册>　去呼吸效应章节。
*/
FH_SINT32 FH_VENC_SetDeBreathEffect(FH_UINT32 chan,const FH_DEBREATH *pstDeBreath);
/*
*   Name: FH_VENC_GetDeBreathEffect
*            获取去呼吸效应使能状态
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_DEBREATH *pstDeBreath
*            是否使能去呼吸效应
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetDeBreathEffect(FH_UINT32 chan, FH_DEBREATH *pstDeBreath);

/*
*   Name: FH_VENC_ClearYuvQueue
*            清空编码yuv队列
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 调用此接口需要先调用FH_VENC_StopRecvPic停止编码通道(JPEG抓图通道不需要stop/start)。
*/
FH_SINT32 FH_VENC_ClearYuvQueue(FH_UINT32 chan);

/*
*   Name: FH_VENC_ResetChn
*            清空编码yuv队列 & 输出码流队列
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 调用此接口需要先调用FH_VENC_StopRecvPic停止编码通道(JPEG抓图通道不需要stop/start)。
*          - 编码通道码流缓存必须工作在独立缓存模式下，否则会返回不支持。
*/
FH_SINT32 FH_VENC_ResetChn(FH_UINT32 chan);
/*
*	Name: FH_VENC_SetDrvCfg
*			 设置默认编码参数
*
*	Parameters:
*
*		[in] FH_VENC_SET_DRV_CONFIG *pstDrvCfg
*			 编码默认参数
*
*
*	Return:
*			0(成功)
*		   非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_SetDrvCfg(FH_VENC_SET_DRV_CONFIG *pstDrvCfg);
/*
*   Name: FH_VENC_InsertUserData
*            设置用户SEI信息,data设置为NULL或len为0时关闭SEI信息添加
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_UINT8* data
*            用户数据指针
*
*       [in] FH_UINT32 len
*            用户数据长度,最大长度1024,单位byte.
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅编码H264&H265支持该接口
*/
FH_SINT32 FH_VENC_InsertUserData(FH_UINT32 chan,FH_UINT8 *data,FH_UINT32 len);

/*
*   Name: FH_VENC_SetBias
*            设置编码模式倾向性参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_ENC_BIAS *bias
*            倾向性参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅编码H264&H265支持该接口
*/
FH_SINT32 FH_VENC_SetBias(FH_UINT32 chan,FH_ENC_BIAS *bias);

/*
*   Name: FH_VENC_GetBias
*            获取编码模式倾向性参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_ENC_BIAS *bias
*            倾向性参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 仅编码H264&H265支持该接口
*/
FH_SINT32 FH_VENC_GetBias(FH_UINT32 chan,FH_ENC_BIAS *bias);

/*
*   Name: FH_VENC_SetModParam
*            设置编码相关模块参数
*
*   Parameters:
*
*       [in] FH_VENC_MOD_PARAM *pstmodparam
*            编码模块参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 必须在未创建过编码通道前调用此接口配置相关属性,否则配置参数可能无效.
*/
FH_SINT32 FH_VENC_SetModParam(const FH_VENC_MOD_PARAM *pstmodparam);


/*
*   Name: FH_VENC_GetModParam
*            获取编码相关模块参数
*
*   Parameters:
*
*       [out] FH_VENC_MOD_PARAM *pstmodparam
*            编码模块参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetModParam(FH_VENC_MOD_PARAM *pstmodparam);

/*
*   Name: FH_VENC_SetChnParam
*            设置编码通道参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_VENC_CHN_PARAM *pstchnparam
*            编码通道参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          - 必须在未创建对应编码通道前调用此接口配置相关属性,否则配置参数可能无效.
*          - 驱动默认开启的通道数非最大通道数, 如使用比较大的通道号时可以通过maxencnum的命令对通道数量进行调整。
*/
FH_SINT32 FH_VENC_SetChnParam(FH_UINT32 chan,const FH_VENC_CHN_PARAM *pstchnparam);

/*
*   Name: FH_VENC_GetChnParam
*            获取编码通道参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_VENC_CHN_PARAM *pstchnparam
*            编码通道参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VENC_GetChnParam(FH_UINT32 chan,FH_VENC_CHN_PARAM *pstchnparam);

/*
*   Name: FH_VENC_SetCmprate
*         设置编码通道压缩率
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_UINT32 val 压缩率
*            0=65%, 1=70%, 2=75%, 3=80%, 4=85%, 5=90%, 6=95%, 7=100%
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          此接口需要在FH_VENC_CreateChn之后,FH_VENC_SetChnAttr之前调用
*/

FH_SINT32 FH_VENC_SetCmprate(FH_UINT32 chan,FH_UINT32 val);
/*
*   Name: FH_VENC_SetGraymode
*         设置编码效果模式为黑白模式还是彩色模式
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_UINT32 val 0，彩色模式，系统默认为0; 1,黑白模式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          此接口需要在FH_VENC_CreateChn之后调用
*/

FH_SINT32 FH_VENC_SetGraymode(FH_UINT32 chan,FH_UINT32 val);

/*
*   Name: FH_VENC_SetOsdScenemode
*         设置编码效果模式为叠加了OSD人脸框
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_UINT32 val 0，没有人脸框，系统默认为0; 1,有人脸框场景的
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          此接口需要在FH_VENC_CreateChn之后调用
*/

FH_SINT32 FH_VENC_SetOsdScenemode(FH_UINT32 chan,FH_UINT32 val);

/*
*   Name: FH_VENC_FREE_PubStreamBuf
*         当所有通道销毁后，释放公共码流接口
*
*   Parameters:
*
*       [in] FH_STREAM_TYPE type
*            目前有三种公共码流
*			 1，FH_STREAM_H264orFH_STREAM_H265 for venc编码
*			 2，FH_STREAM_JPEG for jpeg编码
*            3，FH_STREAM_MJPEG for mjpeg编码
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          此接口需要完全关闭h265+264通道后，可以调用释放venc编码公共码流
*          此接口需要完全关闭jpeg通道后，可以调用释放jpeg编码公共码流
*          此接口需要完全关闭mjpeg通道后，可以调用释放mjpeg编码公共码流
*/
FH_SINT32 FH_VENC_FREE_PubStreamBuf(FH_STREAM_TYPE type);

#ifdef AUTOTEST
/*
*   Name: FH_VENC_SetEncAdvCfgParam
*            设置编码高级参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_ENC_ADV_CFG_PARAM *pstEncAdvCfg
*            编码高级参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          自动测试用，使用时driver编译需要打开AUTOTEST
*/
FH_SINT32 FH_VENC_SetEncAdvCfgParam(FH_UINT32 chan,const FH_ENC_ADV_CFG_PARAM *pstEncAdvCfg);
/*
*   Name: FH_VENC_GetEncAdvCfgParam
*            获取编码高级参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_ENC_ADV_CFG_PARAM *pstEncAdvCfg
*            编码高级参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          自动测试用，使用时driver编译需要打开AUTOTEST
*/
FH_SINT32 FH_VENC_GetEncAdvCfgParam(FH_UINT32 chan,FH_ENC_ADV_CFG_PARAM *pstEncAdvCfg);
/*
*   Name: FH_VENC_ClearTimeOut
*           清空timeout数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          自动测试用，使用时driver编译需要打开AUTOTEST
*/
FH_SINT32 FH_VENC_ClearTimeOut(FH_UINT32 chan);
/*
*   Name: FH_VENC_GetTimeOut
*            获取超时计数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_UINT32 *pu32TimeOut
*            编码超时计数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          自动测试用，使用时driver编译需要打开AUTOTEST
*/
FH_SINT32 FH_VENC_GetTimeOut(FH_UINT32 chan,FH_UINT32 *pu32TimeOut);
/*
*   Name: FH_VENC_Reset
*            复位编码器
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          自动测试用，使用时driver编译需要打开AUTOTEST
*/
FH_SINT32 FH_VENC_Reset(FH_UINT32 chan);
/*
*   Name: FH_VENC_SetCostomGop
*            设置编码用户参考结构
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_VENC_CUSTOM_GOP_CFG *pgop_cfg
*            用户参考结构
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          MJPEG/JPEG通道不支持该接口。
*/
FH_SINT32 FH_VENC_SetCostomGop(FH_UINT32 chan, const FH_VENC_CUSTOM_GOP_CFG *pgop_cfg);
/*
*   Name: FH_VENC_GetCostomGop
*            获取编码跳帧参考
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_VENC_CUSTOM_GOP_CFG *pgop_cfg
*            用户参考结构
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          MJPEG/JPEG通道不支持该接口。
*/
FH_SINT32 FH_VENC_GetCostomGop(FH_UINT32 chan, FH_VENC_CUSTOM_GOP_CFG *pgop_cfg);
#endif // end AUTOTEST
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /*__MPI_VO_H__ */



