#ifndef __FH_VOU_MPI_H__
#define __FH_VOU_MPI_H__

#include "fh_vou_mpipara.h"
#include "types/type_def.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**********************************************************************
    Standard API
**********************************************************************/

FH_SINT32 FH_MPI_VO_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr);
FH_SINT32 FH_MPI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);

FH_SINT32 FH_MPI_VO_Enable(VO_DEV VoDev);
FH_SINT32 FH_MPI_VO_Disable(VO_DEV VoDev);

FH_SINT32 FH_MPI_VO_CloseFd(FH_VOID);

FH_SINT32 FH_MPI_VO_SetVideoLayerAttr(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
FH_SINT32 FH_MPI_VO_GetVideoLayerAttr(VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);

FH_SINT32 FH_MPI_VO_EnableVideoLayer(VO_LAYER VoLayer);
FH_SINT32 FH_MPI_VO_DisableVideoLayer(VO_LAYER VoLayer);

FH_SINT32 FH_MPI_VO_BindVideoLayer(VO_LAYER VoLayer, VO_DEV VoDev);
FH_SINT32 FH_MPI_VO_UnBindVideoLayer(VO_LAYER VoLayer, VO_DEV VoDev);

FH_SINT32 FH_MPI_VO_SetVideoLayerPriority(VO_LAYER VoLayer, FH_UINT32 u32Priority);
FH_SINT32 FH_MPI_VO_GetVideoLayerPriority(VO_LAYER VoLayer, FH_UINT32 *pu32Priority);

FH_SINT32 FH_MPI_VO_SetVideoLayerPartitionMode(VO_LAYER VoLayer, VO_PART_MODE_E enPartMode);
FH_SINT32 FH_MPI_VO_GetVideoLayerPartitionMode(VO_LAYER VoLayer, VO_PART_MODE_E *penPartMode);

FH_SINT32 FH_MPI_VO_SetAttrBegin(VO_LAYER VoLayer);
FH_SINT32 FH_MPI_VO_SetAttrEnd(VO_LAYER VoLayer);

FH_SINT32 FH_MPI_VO_GetScreenFrame(VO_LAYER VoLayer, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);
FH_SINT32 FH_MPI_VO_ReleaseScreenFrame(VO_LAYER VoLayer, VIDEO_FRAME_INFO_S *pstVFrame);

FH_SINT32 FH_MPI_VO_SetDispBufLen(VO_LAYER VoLayer, FH_UINT32 u32BufLen);
FH_SINT32 FH_MPI_VO_GetDispBufLen(VO_LAYER VoLayer, FH_UINT32 *pu32BufLen);

FH_SINT32 FH_MPI_VO_EnableChn(VO_LAYER VoLayer, VO_CHN VoChn);
FH_SINT32 FH_MPI_VO_DisableChn(VO_LAYER VoLayer, VO_CHN VoChn);

FH_SINT32 FH_MPI_VO_SetChnAttr(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_ATTR_S *pstChnAttr);
FH_SINT32 FH_MPI_VO_GetChnAttr(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr);

FH_SINT32 FH_MPI_VO_SetChnParam(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_PARAM_S *pstChnParam);
FH_SINT32 FH_MPI_VO_GetChnParam(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_PARAM_S *pstChnParam);

FH_SINT32 FH_MPI_VO_SetChnDispPos(VO_LAYER VoLayer, VO_CHN VoChn, const FH_POINT *pstDispPos);
FH_SINT32 FH_MPI_VO_GetChnDispPos(VO_LAYER VoLayer, VO_CHN VoChn, FH_POINT *pstDispPos);

FH_SINT32 FH_MPI_VO_SetChnFrameRate(VO_LAYER VoLayer, VO_CHN VoChn, FH_SINT32 s32ChnFrmRate);
FH_SINT32 FH_MPI_VO_GetChnFrameRate(VO_LAYER VoLayer, VO_CHN VoChn, FH_SINT32 *ps32ChnFrmRate);

FH_SINT32 FH_MPI_VO_GetChnFrame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstFrame, FH_SINT32 s32MilliSec);
FH_SINT32 FH_MPI_VO_ReleaseChnFrame(VO_LAYER VoLayer, VO_CHN VoChn, const VIDEO_FRAME_INFO_S *pstFrame);

FH_SINT32 FH_MPI_VO_PauseChn(VO_LAYER VoLayer, VO_CHN VoChn);
FH_SINT32 FH_MPI_VO_ResumeChn(VO_LAYER VoLayer, VO_CHN VoChn);
FH_SINT32 FH_MPI_VO_StepChn(VO_LAYER VoLayer, VO_CHN VoChn);
FH_SINT32 FH_MPI_VO_RefreshChn( VO_LAYER VoLayer, VO_CHN VoChn);

FH_SINT32 FH_MPI_VO_ShowChn(VO_LAYER VoLayer, VO_CHN VoChn);
FH_SINT32 FH_MPI_VO_HideChn(VO_LAYER VoLayer, VO_CHN VoChn);

FH_SINT32 FH_MPI_VO_SetZoomInWindow(VO_LAYER VoLayer, VO_CHN VoChn, const VO_ZOOM_ATTR_S *pstZoomAttr);
FH_SINT32 FH_MPI_VO_GetZoomInWindow(VO_LAYER VoLayer, VO_CHN VoChn, VO_ZOOM_ATTR_S *pstZoomAttr);

FH_SINT32 FH_MPI_VO_SetPlayToleration(VO_LAYER VoLayer, FH_UINT32 u32Toleration);
FH_SINT32 FH_MPI_VO_GetPlayToleration(VO_LAYER VoLayer, FH_UINT32 *pu32Toleration);

FH_SINT32 FH_MPI_VO_GetChnPts(VO_LAYER VoLayer, VO_CHN VoChn, FH_UINT64 *pu64ChnPts);

FH_SINT32 FH_MPI_VO_SendFrame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);

FH_SINT32 FH_MPI_VO_SetChnBorder(VO_LAYER VoLayer, VO_CHN VoChn, const VO_BORDER_S *pstBorder);
FH_SINT32 FH_MPI_VO_GetChnBorder(VO_LAYER VoLayer, VO_CHN VoChn, VO_BORDER_S *pstBorder);

FH_SINT32 FH_MPI_VO_BindGraphicLayer(GRAPHIC_LAYER GraphicLayer, VO_DEV VoDev);
FH_SINT32 FH_MPI_VO_UnBindGraphicLayer(GRAPHIC_LAYER GraphicLayer, VO_DEV VoDev);

/**********************************************************************
    Extension API
**********************************************************************/

FH_SINT32 FH_MPI_VO_SetGraphicLayerPos(GRAPHIC_LAYER GraphicLayer, const FH_POINT *pstPos);
FH_SINT32 FH_MPI_VO_SetGraphicLayerSize(GRAPHIC_LAYER GraphicLayer, const FH_SIZE *pstSize);
FH_SINT32 FH_MPI_VO_SetGraphicLayerAlpha(GRAPHIC_LAYER GraphicLayer, const ALPHA_S *pstAlpha);
FH_SINT32 FH_MPI_VO_UpdateGraphicLayer(GRAPHIC_LAYER GraphicLayer, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);
FH_SINT32 FH_MPI_VO_EnableGraphicLayer(GRAPHIC_LAYER GraphicLayer);
FH_SINT32 FH_MPI_VO_DisableGraphicLayer(GRAPHIC_LAYER GraphicLayer);
FH_SINT32 FH_MPI_VO_ShowGraphicLayer(GRAPHIC_LAYER GraphicLayer);
FH_SINT32 FH_MPI_VO_HideGraphicLayer(GRAPHIC_LAYER GraphicLayer);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__FH_VOU_MPI_H__ */

