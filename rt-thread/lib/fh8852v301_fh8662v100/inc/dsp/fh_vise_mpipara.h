#ifndef __FH_VISE_MPIPARA_H__
#define __FH_VISE_MPIPARA_H__
/**|VISE|**/

#include "types/type_def.h"
#include "fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#pragma pack(4)

#define VISE_MAX_CANVAS_NUM     (4)

typedef struct
{
	FH_UINT32 layer;     // 拼接层号,要求必须层号低的先叠加,高的后叠加以支持类似画中画或融合层 | [ ]
	FH_AREA   area;      // 拼接位置 | [ ]
	FH_UINT32 extcopy;   // 区域是否写入独立缓冲区，然后通过CPU或硬件模块拷贝到画布，考虑后续扩增的保留参数，暂不支持 | [ ]
}FH_VISE_CANVAS_AREA;

typedef struct
{
	FH_SIZE             size;            // 图像幅面  | [ ]
	FH_UINT32           offset;          // 通道输出地址偏移(以pixel为单位) | [ ]
	FH_UINT32           stride;          // 通道输出stride(配置为0时为通道幅面宽,以pixel为单位) | [ ]
	FH_UINT32           depth;           // 通道输出队列深度(需要小于通道buffer个数) | [0-6]
	FH_UINT32           pipeline_num;    // 通道流水线个数 | [1-2]
	FH_VPU_VO_MODE      data_format;     // DDR输入格式,支持格式参考开发手册 | [ ]
	FH_UINT32           strict_pts_chk;  // 是否严格检查拼接数据源时间戳一致 | [ ]
	FH_FRAME_CTRL       frame_ctrl;      // 帧率控制参数 | [ ]
	FH_UINT32           canvas_area_num; // 支持的拼接数量 | [ ]
	FH_VISE_CANVAS_AREA canvas_area[VISE_MAX_CANVAS_NUM]; // 拼接区域属性 | [ ]
	/* lpbuf模式下所有画布区域都必须为VPU开启lpbuf的通道，且仅支持上下拼接，拼接区域之间不能有冗余数据,
		循环缓冲下区域的layer会自动配置为由上到下，从低到高, 用户的配置值无效.*/
	FH_UINT32           canvas_lpbuf_en; // 拼接画布是否使用循环缓冲模式 | [ ]
}FH_VISE_CANVAS_ATTR;

typedef struct
{
	FH_UINT32    finish_cnt;     // 通道完成帧数  | [ ]
	FH_UINT32    error_lost_cnt; // 异常丢帧数量,未完成或某些区域帧率不足等原因(按区域单独统计的总数)  | [ ]
	FH_UINT32    nobuf_lost_cnt; // 申请内存失败导致的丢帧(按区域单独统计的总数)  | [ ]
	FH_FRAMERATE status_fps;     // 通道完成帧数  | [ ]
	FH_UINT32    area_recv_cnt[VISE_MAX_CANVAS_NUM];   // 各区域收到帧数  | [ ]
	FH_UINT32    area_finish_cnt[VISE_MAX_CANVAS_NUM]; // 各区域完成帧数  | [ ]
	FH_UINT32    area_fail_cnt[VISE_MAX_CANVAS_NUM];   // 各区域未完成帧数  | [ ]
}FH_VISE_CANVAS_STATUS;

#pragma pack()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

