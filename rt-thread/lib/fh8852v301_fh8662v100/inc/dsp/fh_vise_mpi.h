#ifndef __FH_VISE_MPI_H__
#define __FH_VISE_MPI_H__
/**VISE**/

#include "types/type_def.h"
#include "fh_common.h"
#include "fh_vise_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/*
*   Name: FH_VISE_CreateCanvas
*            创建拼接画布通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_VISE_CANVAS_ATTR *pstcanvasattr
*            拼接画布属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_CreateCanvas(FH_UINT32 chan,FH_VISE_CANVAS_ATTR *pstcanvasattr);

/*
*   Name: FH_VISE_DestroyCanvas
*            销毁拼接画布通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_DestroyCanvas(FH_UINT32 chan);


/*
*   Name: FH_VISE_SetCanvasAttr
*            设置拼接画布通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_VISE_CANVAS_ATTR *pstcanvasattr
*            拼接画布属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_SetCanvasAttr(FH_UINT32 chan,FH_VISE_CANVAS_ATTR *pstcanvasattr);

/*
*   Name: FH_VISE_GetCanvasAttr
*            获取拼接画布通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_VISE_CANVAS_ATTR *pstcanvasattr
*            拼接画布属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_GetCanvasAttr(FH_UINT32 chan,FH_VISE_CANVAS_ATTR *pstcanvasattr);


/*
*   Name: FH_VISE_Start
*            开启通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_Start(FH_UINT32 chan);


/*
*   Name: FH_VISE_Stop
*            停止通道
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_Stop(FH_UINT32 chan);

/*
*   Name: FH_VISE_GetChnFrame
*            获取输出的图像数据
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_VIDEO_FRAME *pstframeinfo
*            图像数据信息
*
*       [in] FH_UINT32 timeout_ms
*            接口阻塞超时时间(ms)
*
*       [in] FH_UINT32 is_norpt
*            是否使用NoRpt模式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          需要在FH_VISE_CreateCanvas后调用。
*/
FH_SINT32 FH_VISE_GetChnFrame(FH_UINT32 chan,FH_VIDEO_FRAME *pstframeinfo,FH_UINT32 timeout_ms,FH_UINT32 is_norpt);

/*
*   Name: FH_VISE_ReleaseChnFrame
*            释放被FH_VISE_GetChnFrame锁定的缓存
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_VIDEO_FRAME *pstframeinfo
*            图像数据信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          需要在FH_VISE_CreateCanvas后调用。
*/
FH_SINT32 FH_VISE_ReleaseChnFrame(FH_UINT32 chan,FH_VIDEO_FRAME *pstframeinfo);

/*
*   Name: FH_VISE_SetFramectrl
*            设置帧率控制参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] const FH_FRAME_CTRL *pstframectrl
*            帧率控制参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          需要在FH_VISE_CreateCanvas后调用。
*/
FH_SINT32 FH_VISE_SetFramectrl(FH_UINT32 chan,const FH_FRAME_CTRL *pstframectrl);

/*
*   Name: FH_VISE_GetFramectrl
*            获取帧率控制参数
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out]  FH_FRAME_CTRL *pstframectrl
*            帧率控制参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          需要在FH_VISE_CreateCanvas后调用。
*/
FH_SINT32 FH_VISE_GetFramectrl(FH_UINT32 chan, FH_FRAME_CTRL *pstframectrl);

/*
*   Name: FH_VISE_GetStatus
*            获取通道统计状态
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [out] FH_VISE_CANVAS_STATUS *pstcanvasstatus
*            拼接通道统计
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_VISE_GetStatus(FH_UINT32 chan,FH_VISE_CANVAS_STATUS *pstcanvasstatus);

/*
*   Name: FH_VISE_AttachVbPool
*            将VISE的通道绑定到某个视频缓存VB池中
*
*   Parameters:
*
*       [in] FH_UINT32 chan
*            通道号
*
*       [in] FH_UINT32 vbpool
*            视频缓存VB池信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          1. 支持与其他模块共用私有VB,但建议在内存大小相近时才进行共用
*          2. 如需切换绑定的私有VB只需重新调用FH_VISE_AttachVbPool
*          3. 基于用户获取通道数据和帧率控制等需求，VPU会占用最新的一帧，请合理分配VB的BLK数量
*          4. 需要在FH_VISE_CreateCanvas后调用。
*/
FH_SINT32 FH_VISE_AttachVbPool(FH_UINT32 chan,FH_UINT32 vbpool);

/*
*   Name: FH_VISE_DetachVbPool
*            将VISE通道从某个视频缓存VB池中解绑
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
*          调用此接口返回后,相关VB内存依然有可能被各模块使用,需要用户自行保证相关内存使用完毕后才能销毁
*          需要在FH_VISE_CreateCanvas后调用。
*/
FH_SINT32 FH_VISE_DetachVbPool(FH_UINT32 chan);

/*
*   Name: FH_VISE_GetChnFd
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
FH_SINT32 FH_VISE_GetChnFd(FH_UINT32 chan, FH_SINT32 *fd);

/*
*   Name: FH_VISE_CloseChnFd
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
FH_SINT32 FH_VISE_CloseChnFd(FH_UINT32 chan);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif
