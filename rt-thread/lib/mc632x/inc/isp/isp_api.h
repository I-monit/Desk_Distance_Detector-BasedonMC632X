#ifndef _ISP_API_H_
#define _ISP_API_H_


#include "isp_common.h"
#include "isp_sensor_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#define lift_shift_bit_num(bit_num)			(1<<bit_num)
// #define DEBUG_API 1

#define CHECK_VALIDATION(x, a, b)                                                                                                  \
    do                                                                                                                             \
    {                                                                                                                              \
        int r;                                                                                                                     \
        if ((x < a) || (x > b))                                                                                                    \
        {                                                                                                                          \
            if (x < a)                                                                                                             \
                r = a;                                                                                                             \
            else                                                                                                                   \
                r = b;                                                                                                             \
            fh_printf("[WARNING]parameter out of range @%s %s= %d | range=[%d,%d] | auto clip to %d\n", __func__, #x, x, a, b, r); \
            x = r;                                                                                                                 \
        }                                                                                                                          \
    } while (0)

#define TRACE_API ispTrace_FunctionName(u32DevId, __func__)

#define FUNC_DEP __attribute__((deprecated))

enum ISP_HW_MODULE_LIST
{
    HW_MODUL_HMIRROR       = lift_shift_bit_num(0),
    HW_MODUL_DPC           = lift_shift_bit_num(1),
    HW_MODUL_GB            = lift_shift_bit_num(2),
    HW_MODUL_LSC           = lift_shift_bit_num(3),
    HW_MODUL_GAIN          = lift_shift_bit_num(4),
    HW_MODUL_NR2D          = lift_shift_bit_num(5),
    HW_MODUL_WB            = lift_shift_bit_num(6),
    HW_MODUL_DRC           = lift_shift_bit_num(7),
    HW_MODUL_CFA           = lift_shift_bit_num(8),
    HW_MODUL_HLR           = lift_shift_bit_num(9),
    HW_MODUL_RGBB          = lift_shift_bit_num(10),
    HW_MODUL_CGAMMA        = lift_shift_bit_num(11),
    HW_MODUL_YNR           = lift_shift_bit_num(12),
    HW_MODUL_CNR           = lift_shift_bit_num(13),
    HW_MODUL_APC           = lift_shift_bit_num(14),
    HW_MODUL_PURPLE        = lift_shift_bit_num(15),
    HW_MODUL_YGAMMA        = lift_shift_bit_num(16),
    HW_MODUL_CHROMA        = lift_shift_bit_num(17),
    HW_MODUL_YIE           = lift_shift_bit_num(18),
    HW_MODUL_CIE           = lift_shift_bit_num(19),
    HW_MODUL_NR3D          = lift_shift_bit_num(20),
    ISP_HW_MODULE_LIST_DUMMY=0xffffffff,
};

/**SYSTEM_CONTROL*/
/*
*   Name: API_ISP_GetBuffSize
*            获取ISP中分配的buffer大小
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 width
*            输入的幅面宽度
*
*       [IN]  FH_UINT32 height
*            输入的幅面高度
*
*   Return:
*            ISP中分配的buffer大小
*   Note:
*       只用于统计内存时调用,isp正常运行程序不能调用,否则会出异常
*/
FH_SINT32 API_ISP_GetBuffSize(FH_UINT32 u32IspDevId, FH_UINT32 width, FH_UINT32 height);
/*
*   Name: API_ISP_MemInit
*            ISP内部使用的memory分配与初始化
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  ISP_MEM_INIT* stMemInit
*            ISP初始化内存的参数配置
*
*   Return:
*            0(正确)
*           -1(ISP设备驱动打开失败)
*   Note:
*       无
*/
FH_SINT32 API_ISP_MemInit(FH_UINT32 u32IspDevId, ISP_MEM_INIT* stMemInit);
/*
*   Name: API_ISP_GetBinAddr
*            获取ISP的param参数的地址和大小
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_PARAM_CONFIG* param_conf
*            param的地址和大小
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetBinAddr(FH_UINT32 u32IspDevId, ISP_PARAM_CONFIG* pstParaCfg);
/*
*   Name: API_ISP_SetCisClk
*            配置供给sensor的时钟
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 cisClk
*            需要配置时钟频率值
*
*   Return:
*            正确则返回0,错误则返回ERROR_ISP_SET_CIS_CLK
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCisClk(FH_UINT32 u32IspDevId, FH_UINT32 cisClk);
/*
*   Name: API_ISP_CisClkEn
*            供给sensor的时钟使能配置
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_BOOL bEn
*            1表示使能时钟，0表示关闭时钟。
*
*   Return:
*            0(正确)，错误(ERROR_ISP_SET_CIS_CLK)
*   Note:
*       无
*/
FH_SINT32 API_ISP_CisClkEn(FH_UINT32 u32IspDevId, FH_BOOL bEn);
/*
*   Name: API_ISP_SetViAttr
*            配置vi相关的一些幅面信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const ISP_VI_ATTR_S *pstViAttr
*            结构体ISP_VI_ATTR_S的指针
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetViAttr(FH_UINT32 u32IspDevId, const ISP_VI_ATTR_S *pstViAttr);
/*
*   Name: API_ISP_GetViAttr
*            获取当前幅面相关信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_VI_ATTR_S *pstViAttr
*            结构体ISP_VI_ATTR_S的指针
*
*   Return:
*            0(正确)，
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetViAttr(FH_UINT32 u32IspDevId, ISP_VI_ATTR_S *pstViAttr);
/*
*   Name: API_ISP_Init
*            初始化ISP硬件寄存器，并启动ISP
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*   Return:
*            0(正确)
*        -1003(ISP初始化异常)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Init(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_LoadIspParam
*            加载指定参数到DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] char *isp_param_buff
*            指定参数的指针
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_LoadIspParam(FH_UINT32 u32IspDevId, char *isp_param_buff);
/*
*   Name: API_ISP_Pause
*            暂停ISP对图像的处理与输出
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Pause(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_Resume
*            恢复ISP对图像处理与输出
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Resume(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_PowerSuspend
*            关闭isp对应的时钟,降低功耗
*
*   Parameters:
*
*       [IN]  无
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_PowerSuspend(FH_VOID);
/*
*   Name: API_ISP_PowerResume
*            打开isp时钟
*
*   Parameters:
*
*       [IN]  无
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_PowerResume(FH_VOID);
/*
*   Name: API_ISP_Run
*            ISP策略处理
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*   Return:
*            0(正确)，
*           -1(图像丢失)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Run(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_SetRunPos
*            ISP执行策略时机点选择
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  ISP_RUN_POS_SEL_E enRunPos
*            ISP策略时机点选择
*
*   Return:
*            0(正确)，
*           -1(图像丢失)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetRunPos(FH_UINT32 u32DevId, ISP_RUN_POS_SEL_E enRunPos);
/*
*   Name: API_ISP_AE_AWB_Run
*            ISP AE&AWB策略处理
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*   Return:
*            0(正确)
*            图像丢失(ERROR_ISP_WAIT_PICEND_FAILED)
*   Note:
*       无
*/
FH_SINT32 API_ISP_AE_AWB_Run(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_Exit
*            ISP线程退出，清理状态
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Exit(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_SensorRegCb
*            拷贝sensor的回调函数到目标地址。
*
*   Parameters:
*
*       [IN] FH_UINT32 u32SensorId
*            无用
*
*       [IN]  struct isp_sensor_if* pstSensorFunc
*            类型为isp_sensor_if的结构体指针，详细成员变量请查看isp_sensor_if结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SensorRegCb(FH_UINT32 u32IspDevId, FH_UINT32 u32SensorId, struct isp_sensor_if* pstSensorFunc);
/*
*   Name: API_ISP_SensorUnRegCb
*            注销ISP中注册的sensor的回调函数。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 u32SensorId
*            无用
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SensorUnRegCb(FH_UINT32 u32IspDevId, FH_UINT32 u32SensorId);
/*
*   Name: API_ISP_Set_HWmodule_cfg
*            配置ISP模块硬件使能
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const FH_UINT32 u32modulecfg
*            每1bit对应一个硬件使能位0表示关闭，1表示打开，详见枚举ISP_HW_MODULE_LIST。
*
*   Return:
*            0(正确)
*   Note:
*       当需要一次开关多个模块时,使用此函数,若只需控制一个模块,则可使用API_ISP_Set_Determined_HWmodule函数实现
*/
FH_SINT32 API_ISP_Set_HWmodule_cfg(FH_UINT32 u32IspDevId, const FH_UINT32 u32modulecfg);
/*
*   Name: API_ISP_Get_HWmodule_cfg
*            获取当前ISP模块硬件状态
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] FH_UINT32 *u32modulecfg
*            每1bit对应一个硬件使能位0表示关闭，1表示打开，详见枚举ISP_HW_MODULE_LIST。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Get_HWmodule_cfg(FH_UINT32 u32IspDevId, FH_UINT32 *u32modulecfg);
/*
*   Name: API_ISP_Set_Determined_HWmodule
*            配置ISP某个模块硬件使能
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const HW_MODULE_CFG *pstModuleCfg
*            传入要控制的某个模块枚举变量以及开关状态，详见HW_MODULE_CFG。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       一次只能传入一个模块,否则会出错
*/
FH_SINT32 API_ISP_Set_Determined_HWmodule(FH_UINT32 u32IspDevId, const HW_MODULE_CFG *pstModuleCfg);
/*
*   Name: API_ISP_Get_Determined_HWmodule
*           获取ISP某个模块硬件使能状态
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [INOUT] HW_MODULE_CFG *pstModuleCfg
*            传入要控制的某个模块枚举变量,获取该模块开关状态，详见HW_MODULE_CFG。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       一次只能传入一个模块,否则会出错
*/
FH_SINT32 API_ISP_Get_Determined_HWmodule(FH_UINT32 u32IspDevId, HW_MODULE_CFG *pstModuleCfg);
/*
*   Name: API_ISP_GetAlgCtrl
*            获取软件控制开关寄存器状态
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] FH_UINT32 *u32AlgCtrl
*            每1bit对应一个软件使能位0表示关闭，1表示打开
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAlgCtrl(FH_UINT32 u32IspDevId, FH_UINT32 *u32AlgCtrl);
/*
*   Name: API_ISP_SetAlgCtrl
*            设置软件控制开关寄存器状态
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 u32AlgCtrl
*            每1bit对应一个软件使能位0表示关闭，1表示打开
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAlgCtrl(FH_UINT32 u32IspDevId, FH_UINT32 u32AlgCtrl);
/*
*   Name: API_ISP_GetDebugDumpSize
*            获取dump数据的大小, 供应用层分配空间
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32* raw_size
*            raw数据大小,单位byte
*
*       [IN] int dbg_dump_pos
*            选择debug dump 输出结点
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       与API_ISP_GetRaw配合使用
*/
FH_SINT32 API_ISP_GetDebugDumpSize(FH_UINT32 u32IspDevId, FH_UINT32 *raw_size, int dbg_dump_pos);
/*
*   Name: API_ISP_DbgDumpBufferMalloc
*            申请 dump buffer 配置到 DBG_OUT_ADDR
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  FH_UINT16 frmCnt
*            申请 buffer 的帧数
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       大小为 width*height*12/8*frmCnt
*/
FH_SINT32 API_ISP_DbgDumpBufferMalloc(FH_UINT32 u32DevId, FH_UINT16 frmCnt);
/*
*   Name: API_ISP_GetRawSize
*            获取raw数据的大小, 供应用层分配空间
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*       [OUT]  FH_UINT32* raw_size
*            raw数据大小,单位byte
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       与API_ISP_GetRaw配合使用
*/
FH_SINT32 API_ISP_GetRawSize(FH_UINT32 u32IspDevId, FH_UINT32 *raw_size);
/*
*   Name: API_ISP_GetRaw
*            获取ISP　NR3D处的raw数据
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  FH_BOOL strategy_en
*            适配arc使用参数
*
*       [OUT]  FH_VOID* pRawBuff
*            存放raw数据的地址
*
*       [IN]  FH_UINT32 u32Size
*            存放raw数据buffer大小
*
*       [IN]  FH_UINT32 u32FrameCnt
*            需要导出的帧数，导出帧数不连续
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetRaw(FH_UINT32 u32IspDevId, FH_BOOL strategy_en, FH_VOID* pRawBuff, FH_UINT32 u32Size, FH_UINT32 u32FrameCnt);
/*
*   Name: API_ISP_SetDebugDumpBuf
*            连续导出多帧raw功能, 配置相关信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  ISP_RAW_BUF *raw_buf
*            类型为ISP_RAW_BUF的结构体指针，详细成员变量请查看ISP_RAW_BUF结构体定义。
*
*       [IN] int dbg_dump_pos
*            选择debug dump 输出结点
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       应用层需申请足够的vmm内存
*/
FH_SINT32 API_ISP_SetDebugDumpBuf(FH_UINT32 u32IspDevId, ISP_RAW_BUF *raw_buf, int dbg_dump_pos);
/*
*   Name: API_ISP_SetViGenCfg
*            使用VI GEN灌输据方式出图
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_VIGEN_CFG *vi_gen
*            类型为ISP_VIGEN_CFG的结构体指针，详细成员变量请查看ISP_VIGEN_CFG结构体定义。
*
*   Return:
*            0(正确)
*        -3002(空指针异常)
*   Note:
*      无
*/
FH_SINT32 API_ISP_SetViGenCfg(FH_UINT32 u32IspDevId, ISP_VIGEN_CFG *vi_gen);
/*
*   Name: API_ISP_DropCnt
*            丢弃VPU的后N帧数据
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 u32DropCnt
*            下一个帧开始的丢帧数
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*   Note:
*      丢帧生效在下一帧的帧开始时，在一帧当中则实际丢帧要下一帧才开始生效
*/
FH_SINT32 API_ISP_DropCnt(FH_UINT32 U32IspDevId, FH_UINT32 u32DropCnt);
/*
*   Name: API_ISP_SetStitchCfg
*            配置拼接相关参数
*
*   Parameters:
*
*       [IN] ISP_STITCH_CFG_S *pstIspStitchCfg
*            类型为ISP_STITCH_CFG_S的结构体指针，详细成员变量请查看ISP_STITCH_CFG_S结构体定义。
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        需要在API_MemInit之前调用，调用前请先调用API_ISP_Open
*/
FH_SINT32 API_ISP_SetStitchCfg(ISP_STITCH_CFG_S *pstIspStitchCfg);
/*
*   Name: API_ISP_SendIspPreData
*            在输入端发送数据给ISP处理
*
*   Parameters:
*
*       [IN] FH_UINT32 u32IspDevId
*            ISP的设备号，通过它选择配置不同的ISP
*
*       [IN] ISP_STREAM_S* pstStream
*            输入数据结构体指针，用于描述输入数据源属性
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 API_ISP_SendIspPreData(FH_UINT32 u32IspDevId, ISP_STREAM_S* pstStream);
/*
*   Name: API_ISP_WaitIspSem
*            等待到指定的信号量或者等待信号量超时后函数返回
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号，通过它选择配置不同的ISP
*
*       [IN] ISP_SEM_TYPE_E enSemType
*            信号量类型
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 API_ISP_WaitIspSem(FH_UINT32 u32DevId, ISP_SEM_TYPE_E enSemType);
/*
*   Name: API_ISP_SetModAddr
*            配置不同模式下的coef的地址
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号，通过它选择配置不同的ISP
*
*       [IN]  ISP_MOD_ADDR_S stUsrData
*            类型为ISP_MOD_ADDR_S的结构体，详细成员变量请查看ISP_MOD_ADDR_S结构体定义。
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_SINT32 API_ISP_SetModAddr(FH_UINT32 u32DevId, ISP_MOD_ADDR_S stUsrData);
/*
*   Name: API_ISP_GetModAddr
*            获取不同模式下的coef的地址
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号，通过它选择配置不同的ISP
*
*       [IN]  ISP_MOD_ADDR_S* pstUsrData
*            类型为ISP_MOD_ADDR_S的结构体指针，详细成员变量请查看ISP_MOD_ADDR_S结构体定义。
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_SINT32 API_ISP_GetModAddr(FH_UINT32 u32DevId, ISP_MOD_ADDR_S *pstUsrData);
/*
*   Name: API_ISP_SetAovWkMode
*            配置AOV模式参数
*
*   Parameters:
*
*       [IN]  ISP_AOV_MODE_CFG_S* pstAvoCfg
*            类型为ISP_AOV_MODE_CFG_S的结构体指针，详细成员变量请查看ISP_AOV_MODE_CFG_S结构体定义。
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_SINT32 API_ISP_SetAovWkMode(FH_UINT32 u32DevId, ISP_AOV_MODE_CFG_S* pstAvoCfg);
/*
*   Name: API_ISP_SetAovWkMode
*            配置AOV模式参数
*
*   Parameters:
*
*       [IN]  ISP_AOV_MODE_CFG_S* pstAvoCfg
*            类型为ISP_AOV_MODE_CFG_S的结构体指针，详细成员变量请查看ISP_AOV_MODE_CFG_S结构体定义。
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_SINT32 API_ISP_GetAovWkMode(FH_UINT32 u32DevId, ISP_AOV_MODE_CFG_S* pstAvoCfg);
/**SENSOR_CONTROL*/

/*
*   Name: API_ISP_SensorInit
*            sensor预初始化，并未配置sensor寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN]  Sensor_Init_t* initCfg
*            sensor的初始化信息
*
*   Return:
*            0(正确)
*   Note:
*       FH885XV310未使用Sensor_Init_t
*/
FH_SINT32 API_ISP_SensorInit(FH_UINT32 u32DevId, Sensor_Init_t* initCfg);
/*
*   Name: API_ISP_SetSensorFmt
*            初始化sensor
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 format
*            传入的时枚举FORMAT中的枚举值，选择幅面
*
*   Return:
*            0(正确)
*        -3002(sensor初始化异常)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSensorFmt(FH_UINT32 u32IspDevId, FH_UINT32 format);
/*
*   Name: API_ISP_SensorKick
*            启动sensor输出，有的sensor需要用到
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*   Return:
*            0(正确返回)
*           -1(sensor相关的回调函数未被注册)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SensorKick(FH_UINT32 u32IspDevId);
/*
*   Name: API_ISP_SetSensorIntt
*            配置sensor的曝光值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 intt
*            传入的曝光值
*
*   Return:
*            0(正确返回)
*           -1(sensor相关的回调函数未被注册)。
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSensorIntt(FH_UINT32 u32IspDevId, FH_UINT32 intt);
/*
*   Name: API_ISP_SetSensorGain
*            配置sensor增益值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 gain
*            增益值
*
*   Return:
*            0(正确返回)
*           -1(sensor相关的回调函数未被注册)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSensorGain(FH_UINT32 u32IspDevId, FH_UINT32 gain);
/*
*   Name: API_ISP_MapInttGain
*            不同幅面切换时根据当前幅面曝光值和增益值转换成目标幅面曝光值和增益值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_SENSOR_COMMON_CMD_DATA0 *pstMapInttGainCfg
*            类型为ISP_SENSOR_COMMON_CMD_DATA0的结构体指针，详细成员变量请查看ISP_SENSOR_COMMON_CMD_DATA0结构体定义。
*
*   Return:
*            0(正确返回)
*           -1(sensor相关的回调函数未被注册)
*   Note:
*       无
*/
FH_SINT32 API_ISP_MapInttGain(FH_UINT32 u32IspDevId, ISP_SENSOR_COMMON_CMD_DATA0 *pstMapInttGainCfg);
/*
*   Name: API_ISP_SetSensorReg
*            配置sensor寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT16 addr
*            sensor寄存器地址
*
*       [IN] FH_UINT16 data
*            配置的值
*
*   Return:
*            0(正确返回)
*           -1(sensor相关的回调函数未被注册)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSensorReg(FH_UINT32 u32IspDevId, FH_UINT16 addr,FH_UINT16 data);
/*
*   Name: API_ISP_GetSensorReg
*            读取sensor寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT16 addr
*            sensor寄存器地址
*
*       [OUT]  FH_UINT16 *data
*            读取的值
*
*   Return:
*            0(正确返回)
*           -1(sensor相关的回调函数未被注册)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetSensorReg(FH_UINT32 u32IspDevId, FH_UINT16 addr, FH_UINT16 *data);
/*
*   Name: API_ISP_ChangeSnsSlaveAddr
*            修改sensor对应的i2c地址
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN] FH_UINT32 u32i2cAddr
*            新的sensor i2caddr
*
*   Return:
*           0
*
*   Note:
*       1. 修改sensor对应的i2c地址 需要在API_ISP_SensorInit调用之前调用
*/
FH_SINT32 API_ISP_ChangeSnsSlaveAddr(FH_UINT32 u32DevId, FH_UINT32 u32i2cAddr);
/*
*   Name: API_ISP_SensorPause
*            暂停sensor输出
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*   Return:
*           0
*
*   Note:
*       1. 用于暂停sensor的输出流
*/
FH_SINT32 API_ISP_SensorPause(FH_UINT32 u32DevId);
/*
*   Name: API_ISP_SensorResume
*            恢复sensor输出
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*   Return:
*           0
*
*   Note:
*       1. 用于恢复sensor的输出流
*/
FH_SINT32 API_ISP_SensorResume(FH_UINT32 u32DevId);
/*
*   Name: API_ISP_SensorSlave
*            将sensor配置为slave模式
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  ISP_SENSOR_MODE_E enMode
*            进入或者离开master或者slave模式
*
*   Return:
*           0
*
*   Note:
*       1. 将sensor配置为slave模式
*/
FH_SINT32 API_ISP_SensorSlave(FH_UINT32 u32DevId, ISP_SENSOR_MODE_E enMode);
/*
*   Name: API_ISP_SensorMaster
*            将sensor配置为slave模式
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  ISP_SENSOR_MODE_E enMode
*            进入或者离开master或者slave模式
*
*   Return:
*           0
*
*   Note:
*       1. 将sensor配置为master模式
*/
FH_SINT32 API_ISP_SensorMaster(FH_UINT32 u32DevId, ISP_SENSOR_MODE_E enMode);
/*
*   Name: API_ISP_SensorAddBlank
*            增加sensor的消隐
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 u32Rate
*            消隐增加的倍率
*
*   Return:
*           0
*
*   Note:
*       1. 增加sensor的消隐
*/
FH_SINT32 API_ISP_SensorAddBlank(FH_UINT32 u32DevId, FH_UINT32 u32Rate);
/*
*   Name: API_ISP_SensorReduceBlank
*            降低sensor的消隐
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 u32Rate
*            消隐降低的倍率
*
*   Return:
*           0
*
*   Note:
*       1. 降低sensor的消隐
*/
FH_SINT32 API_ISP_SensorReduceBlank(FH_UINT32 u32DevId, FH_UINT32 u32Rate);
/** AE **/
/*
*   Name: API_ISP_SetSmartAECfg
*            设置smart_ae相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] SMART_AE_CFG *pstSmartAeCfg
*            类型为SMART_AE_CFG的结构体指针，详细成员变量请查看SMART_AE_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSmartAECfg(FH_UINT32 u32DevId, SMART_AE_CFG *pstSmartAeCfg);
/*
*   Name: API_ISP_GetSmartAECfg
*            获取smart_ae相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] SMART_AE_CFG *pstSmartAeCfg
*            类型为SMART_AE_CFG的结构体指针，详细成员变量请查看SMART_AE_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetSmartAECfg(FH_UINT32 u32DevId, SMART_AE_CFG *pstSmartAeCfg);
/*
*   Name: API_ISP_GetSmartAeStatus
*            获取smart_ae状态值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] SMART_AE_STATUS *pstSmartAeStatus
*            类型为SMART_AE_STATUS的结构体指针，详细成员变量请查看SMART_AE_STATUS结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetSmartAeStatus(FH_UINT32 u32DevId, SMART_AE_STATUS *pstSmartAeStatus);
/*
*   Name: API_ISP_SetAeDefaultCfg
*            设置ae相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] AE_DEFAULT_CFG *pstAeDefaultCfg
*            类型为AE_DEFAULT_CFG的结构体指针，详细成员变量请查看AE_DEFAULT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/

FH_SINT32 API_ISP_SetAeDefaultCfg(FH_UINT32 u32IspDevId, AE_DEFAULT_CFG *pstAeDefaultCfg);
/*
*   Name: API_ISP_GetAeDefaultCfg
*            获取ae相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] AE_DEFAULT_CFG *pstAeDefaultCfg
*            类型为AE_DEFAULT_CFG的结构体指针，详细成员变量请查看AE_DEFAULT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAeDefaultCfg(FH_UINT32 u32IspDevId, AE_DEFAULT_CFG *pstAeDefaultCfg);
/*
*   Name: API_ISP_SetAeRouteCfg
*            设置ae路线模式相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] AE_ROUTE_CFG *pstAeRouteCfg
*            类型为AE_ROUTE_CFG的结构体指针，详细成员变量请查看AE_ROUTE_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAeRouteCfg(FH_UINT32 u32IspDevId, AE_ROUTE_CFG *pstAeRouteCfg);
/*
*   Name: API_ISP_GetAeRouteCfg
*            获取ae路线模式相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] AE_ROUTE_CFG *pstAeRouteCfg
*            类型为AE_ROUTE_CFG的结构体指针，详细成员变量请查看AE_ROUTE_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAeRouteCfg(FH_UINT32 u32IspDevId, AE_ROUTE_CFG *pstAeRouteCfg);
/*
*   Name: API_ISP_SetAeInfo
*            设置ae曝光时间及增益参数至sensor及硬件寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const ISP_AE_INFO *pstAeInfo
*            类型为ISP_AE_INFO的结构体指针，详细成员变量请查看ISP_AE_INFO结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAeInfo(FH_UINT32 u32IspDevId, const ISP_AE_INFO *pstAeInfo);
/*
*   Name: API_ISP_GetAeInfo
*           从sensor及硬件寄存器获取ae曝光时间及增益参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AE_INFO *pstAeInfo
*            类型为ISP_AE_INFO的结构体指针，详细成员变量请查看ISP_AE_INFO结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAeInfo(FH_UINT32 u32IspDevId, ISP_AE_INFO *pstAeInfo);
/*
*   Name: API_ISP_SetAeInfo3
*            设置ae曝光时间(us)及增益参数至sensor及硬件寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const ISP_AE_INFO_3 *pstAeInfo3
*            类型为ISP_AE_INFO_3的结构体指针，详细成员变量请查看ISP_AE_INFO_2结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       与API_ISP_SetAeInfo的区别在于intt以us为单位
*/
FH_SINT32 API_ISP_SetAeInfo3(FH_UINT32 u32IspDevId, const ISP_AE_INFO_3 *pstAeInfo3);
/*
*   Name: API_ISP_GetAeInfo3
*           从sensor及硬件寄存器获取ae曝光时间及增益参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AE_INFO_3 *pstAeInfo3
*            类型为ISP_AE_INFO_2的结构体指针，详细成员变量请查看ISP_AE_INFO_3结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       与API_ISPGetAeInfo的区别在于intt以us为单位
*/
FH_SINT32 API_ISP_GetAeInfo3(FH_UINT32 u32IspDevId, ISP_AE_INFO_3 *pstAeInfo3);
/*
*   Name: API_ISP_GetAeStatus
*            获取ae状态值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AE_STATUS *pstAeStatus
*            类型为ISP_AE_STATUS的结构体指针，详细成员变量请查看ISP_AE_STATUS结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAeStatus(FH_UINT32 u32IspDevId, ISP_AE_STATUS *pstAeStatus);
/*
*   Name: API_ISP_SetAeUserStatus
*            设置ae用户状态值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] AE_USER_STATUS *pstAeUserStatus
*            类型为AE_USER_STATUS的结构体指针，详细成员变量请查看AE_USER_STATUS结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       若用户使用自定义ae策略,则需要调用该函数配置ae相关状态值,提供给isp的其他策略使用,否则其他策略可能运行会有异常
*/
FH_SINT32 API_ISP_SetAeUserStatus(FH_UINT32 u32IspDevId, AE_USER_STATUS *pstAeUserStatus);

/** AWB **/
/*
*   Name: API_ISP_SetAwbDefaultCfg
*            设置awb相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] AWB_DEFAULT_CFG *pstAwbDefaultCfg
*            类型为AWB_DEFAULT_CFG的结构体指针，详细成员变量请查看AWB_DEFAULT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAwbDefaultCfg(FH_UINT32 u32IspDevId, AWB_DEFAULT_CFG *pstAwbDefaultCfg);
/*
*   Name: API_ISP_GetAwbDefaultCfg
*            获取awb相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] AWB_DEFAULT_CFG *pstAwbDefaultCfg
*            类型为AWB_DEFAULT_CFG的结构体指针，详细成员变量请查看AWB_DEFAULT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAwbDefaultCfg(FH_UINT32 u32IspDevId, AWB_DEFAULT_CFG *pstAwbDefaultCfg);
/*
*   Name: API_ISP_GetAwbStatus
*            获取awb状态值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AWB_STATUS *pstAwbStatus
*            类型为ISP_AWB_STATUS的结构体指针，详细成员变量请查看ISP_AWB_STATUS结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAwbStatus(FH_UINT32 u32IspDevId, ISP_AWB_STATUS *pstAwbStatus);
/*
*   Name: API_ISP_SetAwbGain
*            设置awb增益至硬件寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_AWB_GAIN *pstAwbGain
*            类型为ISP_AWB_GAIN的结构体指针，详细成员变量请查看ISP_AWB_GAIN结构体定义。
*
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAwbGain(FH_UINT32 u32IspDevId, const ISP_AWB_GAIN *pstAwbGain);
/*
*   Name: API_ISP_GetAwbGain
*            从硬件寄存器获取awb增益值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] const ISP_AWB_GAIN *pstAwbGain
*            类型为ISP_AWB_GAIN的结构体指针，详细成员变量请查看ISP_AWB_GAIN结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)

*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAwbGain(FH_UINT32 u32IspDevId, ISP_AWB_GAIN *pstAwbGain);
/**BLC*/
/*
*   Name: API_ISP_SetBlcAttr
*            配置BLC的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_BLC_ATTR * pstBlcAttr
*            类型为ISP_BLC_ATTR的结构体指针，详细成员变量请查看ISP_BLC_ATTR结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetBlcAttr(FH_UINT32 u32IspDevId, ISP_BLC_ATTR * pstBlcAttr);
/*
*   Name: API_ISP_GetBlcAttr
*            获取当前配置的BLC的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_BLC_ATTR * pstBlcAttr
*            类型为ISP_BLC_ATTR的结构体指针，详细成员变量请查看ISP_BLC_ATTR结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetBlcAttr(FH_UINT32 u32IspDevId, ISP_BLC_ATTR * pstBlcAttr);

/** GB*/
/*
*   Name: API_ISP_SetGbCfg
*            配置GB的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_GB_CFG *pstGbCfg
*            类型为ISP_GB_CFG的结构体指针，详细成员变量请查看ISP_GB_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetGbCfg(FH_UINT32 u32IspDevId, ISP_GB_CFG *pstGbCfg);
/*
*   Name: API_ISP_GetGbCfg
*            获取当前配置的GB的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_GB_CFG *pstGbCfg
*            类型为ISP_GB_CFG的结构体指针，详细成员变量请查看ISP_GB_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetGbCfg(FH_UINT32 u32IspDevId, ISP_GB_CFG *pstGbCfg);
/** DPC*/
/*
*   Name: API_ISP_SetDpcCfg
*            配置动态DPC的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_DPC_CFG *pstDpcCfg
*            类型为ISP_DPC_CFG的结构体指针，详细成员变量请查看ISP_DPC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetDpcCfg(FH_UINT32 u32IspDevId, ISP_DPC_CFG *pstDpcCfg);
/*
*   Name: API_ISP_GetDpcCfg
*            获取当前配置的动态DPC的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_DPC_CFG *pstDpcCfg
*            类型为ISP_DPC_CFG的结构体指针，详细成员变量请查看ISP_DPC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetDpcCfg(FH_UINT32 u32IspDevId, ISP_DPC_CFG *pstDpcCfg);
/** CFA*/
/*
*   Name: API_ISP_SetCfaCfg
*            配置Cfa的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_CFA_CFG *pstCfaCfg
*            类型为ISP_CFA_CFG的结构体指针，详细成员变量请查看ISP_CFA_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCfaCfg(FH_UINT32 u32IspDevId, ISP_CFA_CFG *pstCfaCfg);
/*
*   Name: API_ISP_GetCfacCfg
*            获取当前配置的Cfa的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_CFA_CFG *pstCfaCfg
*            类型为ISP_CFA_CFG的结构体指针，详细成员变量请查看ISP_CFA_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetCfaCfg(FH_UINT32 u32IspDevId, ISP_CFA_CFG *pstCfaCfg);
/** LSC*/
/*
*   Name: API_ISP_SetLscCfg
*            配置lsc参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_LSC_CFG *pstLscCfg
*            类型为ISP_LSC_CFG的结构体指针，详细成员变量请查看ISP_LSC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetLscCfg(FH_UINT32 u32IspDevId, ISP_LSC_CFG *pstLscCfg);
/** NR3D*/
/*
*   Name: API_ISP_SetNr3dCfg
*            配置NR3D的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_NR3D_CFG *pstNr3dCfg
*            类型为ISP_NR3D_CFG的结构体指针，详细成员变量请查看ISP_NR3D_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetNr3dCfg(FH_UINT32 u32IspDevId, ISP_NR3D_CFG *pstNr3dCfg);
/*
*   Name: API_ISP_GetNr3dCfg
*            获取当前配置的NR3D的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_NR3D_CFG *pstNr3dCfg
*            类型为ISP_NR3D_CFG的结构体指针，详细成员变量请查看ISP_NR3D_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetNr3dCfg(FH_UINT32 u32IspDevId, ISP_NR3D_CFG *pstNr3dCfg);
/** NR2D*/
/*
*   Name: API_ISP_SetNr2dCfg
*            配置NR2D的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_NR2D_CFG *pstNr2dCfg
*            类型为ISP_NR2D_CFG的结构体指针，详细成员变量请查看ISP_NR2D_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetNr2dCfg(FH_UINT32 u32IspDevId, ISP_NR2D_CFG *pstNr2dCfg);
/*
*   Name: API_ISP_GetNr2dCfg
*            获取当前配置的NR2D的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_NR2D_CFG *pstNr2dCfg
*            类型为ISP_NR2D_CFG的结构体指针，详细成员变量请查看ISP_NR2D_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetNr2dCfg(FH_UINT32 u32IspDevId, ISP_NR2D_CFG *pstNr2dCfg);

/**HLR*/
/*
*   Name: API_ISP_SetHlrCfg
*            配置HLR的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_HLR_CFG *pstHlrCfg
*            类型为ISP_HLR_CFG的结构体指针，详细成员变量请查看ISP_HLR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetHlrCfg(FH_UINT32 u32IspDevId, ISP_HLR_CFG *pstHlrCfg);
/*
*   Name: API_ISP_GetHlrCfg
*            获取当前配置的HLR的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_HLR_CFG *pstHlrCfg
*            类型为ISP_HLR_CFG的结构体指针，详细成员变量请查看ISP_HLR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetHlrCfg(FH_UINT32 u32IspDevId, ISP_HLR_CFG *pstHlrCfg);
/**IE*/
/*
*   Name: API_ISP_SetContrastCfg
*            配置对比度相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_CONTRAST_CFG *pstContrastCfg
*            类型为ISP_CONTRAST_CFG的结构体指针，详细成员变量请查看ISP_CONTRAST_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetContrastCfg(FH_UINT32 u32IspDevId, ISP_CONTRAST_CFG *pstContrastCfg);
/*
*   Name: API_ISP_GetContrastCfg
*            获取当前配置的对比度相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_CONTRAST_CFG *pstContrastCfg
*            类型为ISP_CONTRAST_CFG的结构体指针，详细成员变量请查看ISP_CONTRAST_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetContrastCfg(FH_UINT32 u32IspDevId, ISP_CONTRAST_CFG *pstContrastCfg);
/*
*   Name: API_ISP_SetBrightnessCfg
*            配置亮度相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_BRIGHTNESS_CFG *pstBrightnessCfg
*            类型为ISP_BRIGHTNESS_CFG的结构体指针，详细成员变量请查看ISP_BRIGHTNESS_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetBrightnessCfg(FH_UINT32 u32IspDevId, ISP_BRIGHTNESS_CFG *pstBrightnessCfg);
/*
*   Name: API_ISP_GetBrightnessCfg
*            获取当前配置的亮度相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_BRIGHTNESS_CFG *pstBrightnessCfg
*            类型为ISP_BRIGHTNESS_CFG的结构体指针，详细成员变量请查看ISP_BRIGHTNESS_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetBrightnessCfg(FH_UINT32 u32IspDevId, ISP_BRIGHTNESS_CFG *pstBrightnessCfg);
/**CE*/
/*
*   Name: API_ISP_SetSaturation
*            配置对比度相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_SAT_CFG *pstCeCfg
*            类型为ISP_SAT_CFG的结构体指针，详细成员变量请查看ISP_SAT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSaturation(FH_UINT32 u32IspDevId, ISP_SAT_CFG *pstCeCfg);
/*
*   Name: API_ISP_GetSaturation
*            获取当前配置的对比度相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_SAT_CFG *pstCeCfg
*            类型为ISP_SAT_CFG的结构体指针，详细成员变量请查看ISP_SAT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetSaturation(FH_UINT32 u32IspDevId, ISP_SAT_CFG *pstCeCfg);
/**APC*/
/*
*   Name: API_ISP_SetApcCfg
*            配置锐度相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_APC_CFG *pstApcCfg
*            类型为ISP_APC_CFG的结构体指针，详细成员变量请查看ISP_APC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetApcCfg(FH_UINT32 u32IspDevId, ISP_APC_CFG *pstApcCfg);
/*
*   Name: API_ISP_GetApcCfg
*            获取当前配置的锐度相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_APC_CFG *pstApcCfg
*            类型为ISP_APC_CFG的结构体指针，详细成员变量请查看ISP_APC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetApcCfg(FH_UINT32 u32IspDevId, ISP_APC_CFG *pstApcCfg);
/*
*   Name: API_ISP_SetApcMtCfg
*            配置apc与nr3d的联动参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_APC_MT_CFG *pstApcMtCfg
*            类型为ISP_APC_MT_CFG的结构体指针，详细成员变量请查看ISP_APC_MT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetApcMtCfg(FH_UINT32 u32IspDevId, ISP_APC_MT_CFG *pstApcMtCfg);
/*
*   Name: API_ISP_GetApcMtCfg
*            获取APC与nr3d的联动参数。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_APC_MT_CFG *pstApcMtCfg
*             类型为ISP_APC_MT_CFG的结构体指针，详细成员变量请查看ISP_APC_MT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetApcMtCfg(FH_UINT32 u32IspDevId, ISP_APC_MT_CFG *pstApcMtCfg);
/*
*   Name: API_ISP_SetSharpenStatCfg
*            配置SHARPEN统计窗
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] SHARPEN_STAT_CFG *pstSharpenStatCfg
*            类型为SHARPEN_STAT_CFG的结构体指针，详细成员变量请查看SHARPEN_STAT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            空指针异常(ERROR_ISP_NULL_POINTER)，传入参数超过限制范围(ERROR_ISP_PARA_OUTOF_RANGE)，0(正确)
*/
FH_SINT32 API_ISP_SetSharpenStatCfg(FH_UINT32 u32IspDevId, SHARPEN_STAT_CFG *pstSharpenStatCfg);
/*
*   Name: API_ISP_GetSharpenStatCfg
*            获取当前SHARPEN配置的统计窗口信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] SHARPEN_STAT_CFG *pstSharpenStatCfg
*            类型为SHARPEN_STAT_CFG的结构体指针，详细成员变量请查看SHARPEN_STAT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            空指针异常(ERROR_ISP_NULL_POINTER)
*/
FH_SINT32 API_ISP_GetSharpenStatCfg(FH_UINT32 u32IspDevId, SHARPEN_STAT_CFG *pstSharpenStatCfg);
/*
*   Name: API_ISP_GetSharpenStat
*            获取SHARPEN统计信息即锐度统计信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] SHARPEN_STAT *pstSharpenStat
*            类型为SHARPEN_STAT的结构体指针，详细成员变量请查看SHARPEN_STAT结构体定义。
*
*   Return:
*            0(正确)
*            空指针异常(ERROR_ISP_NULL_POINTER)，
*/
FH_SINT32 API_ISP_GetSharpenStat(FH_UINT32 u32IspDevId, SHARPEN_STAT *pstSharpenStat);
/**GAMMA*/
/*
*   Name: API_ISP_SetGammaCfg
*            配置gamma相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_GAMMA_CFG *pstGammaCfg
*            类型为ISP_GAMMA_CFG的结构体指针，详细成员变量请查看ISP_GAMMA_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetGammaCfg(FH_UINT32 u32IspDevId, ISP_GAMMA_CFG *pstGammaCfg);
/*
*   Name: API_ISP_GetGammaCfg
*            获取当前配置的gamma相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_GAMMA_CFG *pstGammaCfg
*            类型为ISP_GAMMA_CFG的结构体指针，详细成员变量请查看ISP_GAMMA_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetGammaCfg(FH_UINT32 u32IspDevId, ISP_GAMMA_CFG *pstGammaCfg);

/**YCNR*/
/*
*   Name: API_ISP_SetYnrCfg
*            配置YNR相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_YNR_CFG *pstYnrCfg
*            类型为ISP_YNR_CFG的结构体指针，详细成员变量请查看ISP_YNR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetYnrCfg(FH_UINT32 u32IspDevId, ISP_YNR_CFG *pstYnrCfg);
/*
*   Name: API_ISP_GetYnrCfg
*            获取当前配置的YNR相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_YNR_CFG *pstYnrCfg
*            类型为ISP_YNR_CFG的结构体指针，详细成员变量请查看ISP_YNR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetYnrCfg(FH_UINT32 u32IspDevId, ISP_YNR_CFG *pstYnrCfg);
/*
*   Name: API_ISP_SetYnrMtCfg
*            配置YNR与nr3d的联动参数。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_YNR_MT_CFG *pstYnrMtCfg
*             类型为ISP_YNR_MT_CFG的结构体指针，详细成员变量请查看ISP_YNR_MT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetYnrMtCfg(FH_UINT32 u32IspDevId, ISP_YNR_MT_CFG *pstYnrMtCfg);
/*
*   Name: API_ISP_GetYnrMtCfg
*           获取YNR与nr3d的联动参数。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_YNR_MT_CFG *pstYnrMtCfg
*             类型为ISP_YNR_MT_CFG的结构体指针，详细成员变量请查看ISP_YNR_MT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetYnrMtCfg(FH_UINT32 u32IspDevId, ISP_YNR_MT_CFG *pstYnrMtCfg);
/*
*   Name: API_ISP_SetCnrCfg
*            配置CNR相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_CNR_CFG *pstCnrCfg
*            类型为ISP_CNR_CFG的结构体指针，详细成员变量请查看ISP_CNR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCnrCfg(FH_UINT32 u32IspDevId, ISP_CNR_CFG *pstCnrCfg);
/*
*   Name: API_ISP_GetCnrCfg
*            获取当前配置的CNR相关的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_CNR_CFG *pstCnrCfg
*            类型为ISP_CNR_CFG的结构体指针，详细成员变量请查看ISP_CNR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetCnrCfg(FH_UINT32 u32IspDevId, ISP_CNR_CFG *pstCnrCfg);
/*
*   Name: API_ISP_SetCnrMtCfg
*           设置CNR与nr3d的联动参数。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_CNR_MT_CFG *pstCnrMtCfg
*             类型为ISP_CNR_MT_CFG的结构体指针，详细成员变量请查看ISP_CNR_MT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCnrMtCfg(FH_UINT32 u32IspDevId, ISP_CNR_MT_CFG *pstCnrMtCfg);
/*
*   Name: API_ISP_GetCnrMtCfg
*           获取CNR与nr3d的联动参数。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_CNR_MT_CFG *pstCnrMtCfg
*             类型为ISP_CNR_MT_CFG的结构体指针，详细成员变量请查看ISP_CNR_MT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetCnrMtCfg(FH_UINT32 u32IspDevId, ISP_CNR_MT_CFG *pstCnrMtCfg);
/**PURPLE*/
/*
*   Name: API_ISP_SetAntiPurpleBoundary
*            配置去紫边相关的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_PURPLEFRI_CFG *pstPurplefriCfg
*            类型为ISP_PURPLEFRI_CFG的结构体指针，详细成员变量请查看ISP_PURPLEFRI_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAntiPurpleBoundary(FH_UINT32 u32IspDevId, ISP_PURPLEFRI_CFG *pstPurplefriCfg);
/*
*   Name: API_ISP_GetAntiPurpleBoundary
*            获取当前配置的去紫边的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_PURPLEFRI_CFG *pstPurplefriCfg
*            类型为ISP_PURPLEFRI_CFG的结构体指针，详细成员变量请查看ISP_PURPLEFRI_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAntiPurpleBoundary(FH_UINT32 u32IspDevId, ISP_PURPLEFRI_CFG *pstPurplefriCfg);
/**DRC*/
/*
*   Name: API_ISP_SetDrcCfg
*            配置Drc的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_DRC_CFG *pstDrcCfg
*            类型为ISP_DRC_CFG的结构体指针，详细成员变量请查看ISP_DRC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetDrcCfg(FH_UINT32 u32IspDevId, ISP_DRC_CFG *pstDrcCfg);
/*
*   Name: API_ISP_GetDrcCfg
*            获取当前配置的DRC的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_DRC_CFG *pstDrcCfg
*            类型为ISP_DRC_CFG的结构体指针，详细成员变量请查看ISP_DRC_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetDrcCfg(FH_UINT32 u32IspDevId, ISP_DRC_CFG *pstDrcCfg);
/**Debug Interface**/
/*
*   Name: API_ISP_ReadMallocedMem
*            读取指定偏移的VMM分配的内存的值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_SINT32 intMemSlot
*            分配的内存的类型，决定基地址。
*
*       [IN]  FH_UINT32 offset
*            偏移地址，选定的内存会给定其基地址。
*
*       [OUT]  FH_UINT32 *pstData
*            存放读取到数据的地址。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_ReadMallocedMem(FH_UINT32 u32IspDevId, FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstData);
/*
*   Name: API_ISP_WriteMallocedMem
*            写指定偏移的VMM分配的内存的值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_SINT32 intMemSlot
*            分配的内存的类型，决定基地址。
*
*       [IN]  FH_UINT32 offset
*            偏移地址，选定的内存会给定其基地址。
*
*       [IN]  FH_UINT32 *pstData
*            目标值，将该值写入目标地址。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_WriteMallocedMem(FH_UINT32 u32IspDevId, FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstData);
/*
*   Name: API_ISP_ImportMallocedMem
*            导入指定大小的数据到指定偏移的VMM分配的内存
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_SINT32 intMemSlot
*            分配的内存的类型，决定基地址。
*
*       [IN]  FH_UINT32 offset
*            偏移地址，选定的内存会给定其基地址。
*
*       [IN]  FH_UINT32 *pstSrc
*            导入数据的起始地址
*
*       [IN]  FH_UINT32 size
*            需要导入的数据大小
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_ImportMallocedMem(FH_UINT32 u32IspDevId, FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstSrc, FH_UINT32 size);
/*
*   Name: API_ISP_ExportMallocedMem
*            从指定偏移的VMM分配的内存导出指定大小的数据
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_SINT32 intMemSlot
*            分配的内存的类型，决定基地址。
*
*       [IN]  FH_UINT32 offset
*            偏移地址，选定的内存会给定其基地址。
*
*       [OUT]  FH_UINT32 *pstDst
*            存放导出数据的起始地址
*
*       [IN]  FH_UINT32 size
*            需要导出的数据大小
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_ExportMallocedMem(FH_UINT32 u32IspDevId, FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstDst, FH_UINT32 size);
/*
*   Name: API_ISP_GetVIState
*            获取当前ISP的一些运行状态信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_VI_STAT_S *pstStat
*            类型为ISP_VI_STAT_S的结构体指针，详细成员变量请查看ISP_VI_STAT_S结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetVIState(FH_UINT32 u32IspDevId, ISP_VI_STAT_S *pstStat);
/*
*   Name: API_ISP_SetSensorFrameRate
*            配置SENSOR的垂直消隐至指定倍率
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] int m
*            垂直消隐的倍率,U.8,也即0x100表示一倍
*
*   Return:
*            0(正确)
*   Note:
*       当ae策略关闭时可使用该函数,策略打开时的帧消隐由策略参数控制,且只支持大于1倍的降帧
*/
FH_SINT32 API_ISP_SetSensorFrameRate(FH_UINT32 u32IspDevId, int m);
/*
*   Name: API_ISP_SetSensorFrameRate2
*            根据延迟帧数配置SENSOR的垂直消隐至指定倍率
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] int m
*            垂直消隐的倍率,U.8,也即0x100表示一倍
*       [IN] int delayFrames
*            降帧延迟帧数,即1表示延迟1帧
*
*   Return:
*            0(正确)
*   Note:
*       当ae策略关闭时可使用该函数,策略打开时的帧消隐由策略参数控制,且只支持大于1倍的降帧
*/
FH_SINT32 API_ISP_SetSensorFrameRate2(FH_UINT32 u32IspDevId, int m, int delayFrames);
/*
*   Name: API_ISP_Dump_Param
*            拷贝所有DRV寄存器值到指定地址
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] FH_UINT32 *addr
*            存放DRV参数的地址
*
*       [IN] FH_UINT32 *size
*            拷贝数据的大小
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_Dump_Param(FH_UINT32 u32IspDevId, FH_UINT32 *addr,FH_UINT32 *size);
/**MIRROR**/
/*
*   Name: API_ISP_MirrorEnable
*            ISP的MIRROR模块，可以实现镜像,镜像后的pattern不需要配置,软件会自动转换。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] MIRROR_CFG_S *pMirror
*            类型为MIRROR_CFG_S的结构体指针，详细成员变量请查看MIRROR_CFG_S结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_MirrorEnable(FH_UINT32 u32IspDevId, MIRROR_CFG_S *pMirror);
/*
*   Name: API_ISP_SetMirrorAndflip
*            配置SENSOR的镜像和水平翻转。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_BOOL mirror
*            镜像(mirror=1)，正常(mirror=0)
*
*       [IN]  FH_BOOL flip
*            水平翻转(flip=1)，正常(flip=0)
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetMirrorAndflip(FH_UINT32 u32IspDevId, FH_BOOL mirror, FH_BOOL flip);
/*
*   Name: API_ISP_SetMirrorAndflipEx
*            配置SENSOR的镜像和水平翻转，同时会更改ISP中相关bayer配置。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_BOOL mirror
*            镜像(mirror=1)，正常(mirror=0)
*
*       [IN]  FH_BOOL flip
*            水平翻转(flip=1)，正常(flip=0)
*
*       [IN] FH_UINT32 bayer
*            镜像和水平翻转影响之后的BAYER格式。
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetMirrorAndflipEx(FH_UINT32 u32IspDevId, FH_BOOL mirror, FH_BOOL flip,FH_UINT32 bayer);
/*
*   Name: API_ISP_GetMirrorAndflip
*            获取当前SENSOR镜像和水平翻转的状态
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] FH_BOOL *mirror
*            镜像(mirror=1)，正常(mirror=0)
*
*       [OUT]  FH_BOOL *flip
*            水平翻转(flip=1)，正常(flip=0)
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetMirrorAndflip(FH_UINT32 u32IspDevId, FH_BOOL *mirror, FH_BOOL *flip);
/**SMART_IR**/
/*
*   Name: API_ISP_SetSmartIrCfg
*            设置智能切换日夜的参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] SMART_IR_CFG *pstSmartIrCfg
*            类型为SMART_IR_CFG的结构体指针，详细成员变量请查看SMART_IR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSmartIrCfg(FH_UINT32 u32IspDevId, SMART_IR_CFG *pstSmartIrCfg);
/*
*   Name: API_ISP_GetSmartIrCfg
*            获取智能切换日夜的参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] SMART_IR_CFG *pstSmartIrCfg
*            类型为SMART_IR_CFG的结构体指针，详细成员变量请查看SMART_IR_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetSmartIrCfg(FH_UINT32 u32IspDevId, SMART_IR_CFG *pstSmartIrCfg);
/**OTHERS*/
/*
*   Name: API_ISP_SetCropInfo
*            配置输入幅面的水平和垂直的处理偏移。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] int offset_x
*            水平方向的偏移
*
*       [IN] int offset_y
*            垂直方向的偏移。
*
*   Return:
*            0(正确)
*        -3004(偏移值超过输入幅面或者为奇数)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCropInfo(FH_UINT32 u32IspDevId, int offset_x,int offset_y);
/*
*   Name: *FH_ISP_Version
*            获取、打印ISP版本号。
*
*   Parameters:
*
*       [IN] FH_UINT32 print_enable
*            打印ISP库版本号(1)，不打印ISP库版本号(0)
*
*   Return:
*            ISP库版本号的字符串。
*   Note:
*       无
*/
FH_CHAR *FH_ISP_Version(FH_UINT32 print_enable);
/*
*   Name: *FH_ISPCORE_Version
*            获取、打印ISP版本号。
*
*   Parameters:
*
*       [IN] FH_UINT32 print_enable
*            打印ISP_CORE库的版本号(1)，不打印ISP_CORE库的版本号(0)
*
*   Return:
*            ISP_CORE库的版本号的字符串。
*   Note:
*       无
*/
FH_CHAR *FH_ISPCORE_Version(FH_UINT32 print_enable);
/*
*   Name: API_ISP_SetSlotLineEndP
*            设置ISP前段结束中断间隔行数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 u32SlotLineEndp
*           ISP前段结束中断间隔行数
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSlotLineEndP(FH_UINT32 u32IspDevId, FH_UINT32 u32SlotLineEndp);
/*
*   Name: API_ISP_SetSlotLineEndF
*            设置ISP后段结束中断间隔行数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT32 u32SlotLineEndf
*           ISP后段结束中断间隔行数
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSlotLineEndF(FH_UINT32 u32IspDevId, FH_UINT32 u32SlotLineEndf);
/*
*   Name: API_ISP_WaitInterruptSemaphore
*            等待对应中断信号量
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] INTERRUPT_CFG_CMD pstInterruptCfg
*            类型为INTERRUPT_CFG_CMD枚举的类型变量，详细取值请查看INTERRUPT_CFG_CMD结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_WaitInterruptSemaphore(FH_UINT32 u32IspDevId, INTERRUPT_CFG_CMD pstInterruptCfg);
/*
*   Name: API_ISP_TestModeCfg
*            ISP测试模式
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_UINT16 u16Fps
*            配置测试模式输出的帧率
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       1. 帧率只支持帧数帧率，由ISP频率计算得出，不一定可以得到确定的准确帧率
*       2. u16Fps配置等于0的时候，关闭测试模式输出
*/
FH_SINT32 API_ISP_TestModeCfg(FH_UINT32 u32IspDevId, FH_UINT16 u16Fps);
/**FAST_BOOT**/
/*
*   Name: API_ISP_RegisterPicStartCallback
*            在ISP PicStartB的时候的回调函数，可以用在RTT下统计时间。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ispIntCallback cb
*            void型的函数指针，指向回调函数的位置。
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_RegisterPicStartCallback(FH_UINT32 u32IspDevId, ispIntCallback cb);
/*
*   Name: API_ISP_RegisterPicEndCallback
*            在ISP PicEndP的时候的回调函数，可以用在RTT下统计时间。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ispIntCallback cb
*            void型的函数指针，指向回调函数的位置。
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_RegisterPicEndCallback(FH_UINT32 u32IspDevId, ispIntCallback cb);
/*
*   Name: API_ISP_RegisterIspInitCfgCallback
*            ISP硬件初始化回调函数，其执行位置在API_ISP_Init之前，此时ISP未启动，流程中仅执行一次。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ispInitCfgCallback cb
*            void型的函数指针，指向回调函数的位置。
*
*   Return:
*            0(正确)
*   Note:
*       无
*/
FH_SINT32 API_ISP_RegisterIspInitCfgCallback(FH_UINT32 u32IspDevId, ispInitCfgCallback cb);
/*
*   Name: API_ISP_SetBlcInitCfg
*            初始化BLC所有通道的减DC值，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const BLC_INIT_CFG *pstBlcInitCfg
*            类型为BLC_INIT_CFG的结构体指针，详细成员变量请查看BLC_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetBlcInitCfg(FH_UINT32 u32IspDevId, const BLC_INIT_CFG *pstBlcInitCfg);
/*
*   Name: API_ISP_SetWbInitCfg
*            初始化WB三个通道增益值，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const WB_INIT_CFG *pstWbInitCfg
*            类型为WB_INIT_CFG的结构体指针，详细成员变量请查看WB_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetWbInitCfg(FH_UINT32 u32IspDevId, const WB_INIT_CFG *pstWbInitCfg);
/*
*   Name: API_ISP_SetCcmInitCfg
*            初始化CCM矩阵硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] CCM_INIT_CFG *pstCcmInitCfg
*            类型为DPC_INIT_CFG的结构体指针，详细成员变量请查看DPC_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCcmInitCfg(FH_UINT32 u32IspDevId, const CCM_INIT_CFG *pstCcmInitCfg);
/*
*   Name: API_ISP_SetDpcInitCfg
*            初始化DPC硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const DPC_INIT_CFG *pstDpcInitCfg
*            类型为DPC_INIT_CFG的结构体指针，详细成员变量请查看DPC_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetDpcInitCfg(FH_UINT32 u32IspDevId, const DPC_INIT_CFG *pstDpcInitCfg);
/*
*   Name: API_ISP_SetApcInitCfg
*            初始化APC硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const APC_INIT_CFG *pstApcInitCfg
*            类型为APC_INIT_CFG的结构体指针，详细成员变量请查看APC_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetApcInitCfg(FH_UINT32 u32IspDevId, const APC_INIT_CFG *pstApcInitCfg);
/*
*   Name: API_ISP_SetYnrInitCfg
*            初始化YNR硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const YNR_INIT_CFG *pstYnrInitCfg
*            类型为YNR_INIT_CFG的结构体指针，详细成员变量请查看YNR_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetYnrInitCfg(FH_UINT32 u32IspDevId, const YNR_INIT_CFG *pstYnrInitCfg);
/*
*   Name: API_ISP_SetCtrInitCfg
*            配置对比度值到硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const CTR_INIT_CFG *pstCtrInitCfg
*            类型为CTR_INIT_CFG的结构体指针，详细成员变量请查看CTR_INIT_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCtrInitCfg(FH_UINT32 u32IspDevId, const CTR_INIT_CFG *pstCtrInitCfg);
/*
*   Name: API_ISP_SetSatInitCfg
*            配置饱和度值到硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const SAT_INIT_CFG *pstSatInitCfg
*            SAT_INIT_CFG的结构体指针，详细参数查看结构体SAT_INIT_CFG注释。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetSatInitCfg(FH_UINT32 u32IspDevId, const SAT_INIT_CFG *pstSatInitCfg);
/*
*   Name: API_ISP_SetGammaModeInitCfg
*            根据传入的gamma mode值，配置相应硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const GAMMA_MODE_INIT_CFG *pstGammaModeInitCfg
*            目标配置的gamma mode。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetGammaModeInitCfg(FH_UINT32 u32IspDevId, const GAMMA_MODE_INIT_CFG *pstGammaModeInitCfg);
/*
*   Name: API_ISP_SetCGammaInitCfg
*            配置cgamma table到硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const GAMMA_INIT_CFG *pstCGammaInitCfg
*            结构体GAMMA_INIT_CFG的指针，目标配置cgamma表。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCGammaInitCfg(FH_UINT32 u32IspDevId, const GAMMA_INIT_CFG *pstCGammaInitCfg);
/*
*   Name: API_ISP_SetYGammaInitCfg
*            配置ygamma table到硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const GAMMA_INIT_CFG *pstYGammaInitCfg
*            结构体GAMMA_INIT_CFG的指针，目标配置的ygamma表。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetYGammaInitCfg(FH_UINT32 u32IspDevId, const GAMMA_INIT_CFG *pstYGammaInitCfg);
/*
*   Name: API_ISP_SetIspGainInitCfg
*            配置Isp Gain到硬件寄存器，调用阶段是在API_ISP_RegisterIspInitCfgCallback中。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const ISP_GAIN_INIT_CFG *pstIspGainInitCfg
*            结构体ISP_GAIN_INIT_CFG的指针，目标配置的Isp Gain。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetIspGainInitCfg(FH_UINT32 u32IspDevId, const ISP_GAIN_INIT_CFG *pstIspGainInitCfg);
/**CCM**/
/*
*   Name: API_ISP_SetCcmCfg
*            配置CCM。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] CCM_TABLE *pstCcmCfg
*             类型为CCM_TABLE的结构体指针，详细成员变量请查看CCM_TABLE结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCcmCfg(FH_UINT32 u32IspDevId, CCM_TABLE *pstCcmCfg);
/*
*   Name: API_ISP_GetCcmCfg
*           获取CCM。
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] CCM_TABLE *pstCcmCfg
*             类型为CCM_TABLE的结构体指针，详细成员变量请查看CCM_TABLE结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetCcmCfg(FH_UINT32 u32IspDevId, CCM_TABLE *pstCcmCfg);
/*
*   Name: API_ISP_SetCcmHwCfg
*            配置ccm参数至硬件寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_CCM_CFG *pstCcmCfg
*             类型为ISP_CCM_CFG的结构体指针，详细成员变量请查看ISP_CCM_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCcmHwCfg(FH_UINT32 u32IspDevId, ISP_CCM_CFG *pstCcmCfg);
/*
*   Name: API_ISP_GetCcmHwCfg
*            从硬件寄存器获取ccm参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_CCM_CFG *pstCcmCfg
*            类型为ISP_CCM_CFG的结构体指针，详细成员变量请查看ISP_CCM_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetCcmHwCfg(FH_UINT32 u32IspDevId, ISP_CCM_CFG *pstCcmCfg);
/*
*   Name: API_ISP_SetCcm2HwCfg
*            配置ccm2参数至硬件寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_CCM_CFG *pstCcmCfg
*             类型为ISP_CCM_CFG的结构体指针，详细成员变量请查看ISP_CCM_CFG结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetCcm2HwCfg(FH_UINT32 u32IspDevId, ISP_CCM_CFG *pstCcmCfg);
/*
*   Name: API_ISP_GetCcm2HwCfg
*            从硬件寄存器获取ccm2参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_CCM_CFG *pstCcmCfg
*            类型为ISP_CCM_CFG的结构体指针，详细成员变量请查看ISP_CCM_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetCcm2HwCfg(FH_UINT32 u32IspDevId, ISP_CCM_CFG *pstCcmCfg);
/*
*   Name: API_ISP_SetAcrCfg
*            配置ACR的DRV寄存器
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] ISP_ACR_Cfg * pstAcrCfg
*            类型为ISP_ACR_Cfg的结构体指针，详细成员变量请查看ISP_ACR_Cfg结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAcrCfg(FH_UINT32 u32IspDevId, ISP_ACR_CFG * pstAcrCfg);
/*
*   Name: API_ISP_GetAcrCfg
*            获取当前配置的ACR的DRV寄存器值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_ACR_Cfg * pstAcrCfg
*            类型为ISP_ACR_Cfg的结构体指针，详细成员变量请查看ISP_ACR_Cfg结构体定义。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAcrCfg(FH_UINT32 u32IspDevId, ISP_ACR_CFG * pstAcrCfg);
/** STATISTICS **/
/*
*   Name: API_ISP_GetAwbStat
*            获取awb 3*3统计信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AWB_STAT * pstAwbStat
*            类型为ISP_AWB_STAT的结构体指针，详细成员变量请查看ISP_AWB_STAT结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAwbStat(FH_UINT32 u32IspDevId, ISP_AWB_STAT * pstAwbStat);
/*
*   Name: API_ISP_SetAwbStatCfg
*            设置awb 3*3统计配置相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const ISP_AWB_STAT_CFG *pstAwbStatCfg
*            类型为ISP_AWB_STAT_CFG的结构体指针，详细成员变量请查看ISP_AWB_STAT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       传入的参数经过计算后可能由于对齐问题与获取的参数不完全一致
*/
FH_SINT32 API_ISP_SetAwbStatCfg(FH_UINT32 u32IspDevId, const ISP_AWB_STAT_CFG *pstAwbStatCfg);
/*
*   Name: API_ISP_GetAwbStatCfg
*            获取awb统计配置相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AWB_STAT_CFG *pstAwbStatCfg
*            类型为ISP_AWB_STAT_CFG的结构体指针，详细成员变量请查看ISP_AWB_STAT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAwbStatCfg(FH_UINT32 u32IspDevId, ISP_AWB_STAT_CFG *pstAwbStatCfg);

/*
*   Name: API_ISP_GetAwbAdvStat
*            获取awb高级统计信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AWB_ADV_STAT * pstAwbAdvStat
*            类型为ISP_AWB_ADV_STAT的结构体指针，详细成员变量请查看ISP_AWB_ADV_STAT结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       统计窗个数默认16*16,也可通过API_ISP_SetAwbAdvStatCfg进行重新配置,最大支持32*32
*       使用示例:
*              ISP_AWB_ADV_STAT awbAdvStat[256];  // 256是默认窗口配置,若改动的话通过API_ISP_GetAwbAdvStatCfg获取当前统计窗个数
*              API_ISP_GetAwbAdvStat(awbAdvStat);
*/
FH_SINT32 API_ISP_GetAwbAdvStat(FH_UINT32 u32IspDevId, ISP_AWB_ADV_STAT * pstAwbAdvStat);
/*
*   Name: API_ISP_SetAwbAdvStatCfg
*            设置awb高级统计配置相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const ISP_AWB_ADV_STAT_CFG *pstAwbStatCfg
*            类型为ISP_AWB_ADV_STAT_CFG的结构体指针，详细成员变量请查看ISP_AWB_ADV_STAT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetAwbAdvStatCfg(FH_UINT32 u32IspDevId, const ISP_AWB_ADV_STAT_CFG *pstAwbStatCfg);
/*
*   Name: API_ISP_GetAwbAdvStatCfg
*            获取awb高级统计配置相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] ISP_AWB_ADV_STAT_CFG *pstAwbStatCfg
*            类型为ISP_AWB_ADV_STAT_CFG的结构体指针，详细成员变量请查看ISP_AWB_ADV_STAT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetAwbAdvStatCfg(FH_UINT32 u32IspDevId, ISP_AWB_ADV_STAT_CFG *pstAwbStatCfg);
/**GLOBAL_STAT**/
/*
*   Name: API_ISP_GetGlobeStat
*            获取global统计信息
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] GLOBE_STAT *pstGlobeStat
*            类型为GLOBE_STAT的结构体指针，详细成员变量请查看GLOBE_STAT结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       使用示例:
*               GLOBE_STAT glStat[256];  // 256是默认窗口配置,若改动的话通过API_ISP_GetGlobeStatCfg获取当前统计窗个数
*               API_ISP_GetGlobeStat(glStat);
*/
FH_SINT32 API_ISP_GetGlobeStat(FH_UINT32 u32IspDevId, GLOBE_STAT *pstGlobeStat);
/*
*   Name: API_ISP_SetGlobeStatCfg
*            设置global统计配置相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] const GLOBE_STAT_CFG *pstGlobeStatCfg
*            类型为GLOBE_STAT_CFG的结构体指针，详细成员变量请查看GLOBE_STAT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_SetGlobeStatCfg(FH_UINT32 u32IspDevId, const GLOBE_STAT_CFG *pstGlobeStatCfg);
/*
*   Name: API_ISP_GetGlobeStatCfg
*            获取global统计配置相关参数
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [OUT] GLOBE_STAT_CFG *pstGlobeStatCfg
*            类型为GLOBE_STAT_CFG的结构体指针，详细成员变量请查看GLOBE_STAT_CFG结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetGlobeStatCfg(FH_UINT32 u32IspDevId, GLOBE_STAT_CFG *pstGlobeStatCfg);
/*
*   Name: API_ISP_Open
*            打开设备
*
*   Parameters:
*             无
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_UINT32 API_ISP_Open(FH_VOID);
/*
*   Name: API_ISP_Close
*            关闭设备
*
*   Parameters:
*             无
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_UINT32 API_ISP_Close(FH_VOID);
/*
*   Name: API_ISP_GetGroupGlobeStat
*            获取拼接组的global统计信息
*
*   Parameters:
*
*       [IN]  FH_UINT8 u8GrpId
*            拼接组的ID
*
*       [OUT] GROUP_GLOBE_STAT *pstGroupGlobeStat
*            类型为GROUP_GLOBE_STAT的结构体指针，详细成员变量请查看GROUP_GLOBE_STAT结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetGroupGlobeStat(FH_UINT8 u8GrpId, GROUP_GLOBE_STAT *pstGroupGlobeStat);
/*
*   Name: API_ISP_GetGroupAwbAdvStat
*            获取拼接组的awb_adv统计信息
*
*   Parameters:
*
*       [IN]  FH_UINT8 u8GrpId
*            拼接组的ID
*
*       [OUT] GROUP_AWB_ADV_STAT *pstGroupAwbAdvStat
*            类型为GROUP_AWB_ADV_STAT的结构体指针，详细成员变量请查看GROUP_AWB_ADV_STAT结构体定义。
*
*   Return:
*           0(成功)
*           非0(失败，详见错误码)
*
*   Note:
*       无
*/
FH_SINT32 API_ISP_GetGroupAwbAdvStat(FH_UINT8 u8GrpId, GROUP_AWB_ADV_STAT *pstGroupAwbAdvStat);
/**MD**/
/*
*   Name: API_ISP_GetMdStat
*            获取表征MD模块的运动静止程度的数值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT] ISP_MD_STAT_S* pstMdStat
*            类型为ISP_MD_STAT_S的结构体指针，详细成员变量请查看ISP_MD_STAT_S结构体定义。
*
*   Return:
*           0
*
*   Note:
*       1. 运动块和静止块的数值总和固定，图像划分成384个区域
        2. 运动块越多，说明图像当前运动的可能性越大，反之则越小
*/
FH_SINT32 API_ISP_GetMdStat(FH_UINT32 u32DevId, ISP_MD_STAT* pstMdStat);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*_ISP_API_H_*/
