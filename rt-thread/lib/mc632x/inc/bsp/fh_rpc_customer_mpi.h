#ifndef __fh_rpc_customer_mpi_h__
#define __fh_rpc_customer_mpi_h__

#define RPC_CUSTOMER_FLAG_NEED_CopyBack (1<<31)

/*
 * 功能：初始化函数
 *
 * 返回值：
 *       等于0表示成功；小于0表示失败
 */
extern int FH_RPC_Customer_Init(void);


/*
 * 功能：模块退出函数，和FH_RPC_Customer_Init对应
 */
extern int FH_RPC_Customer_DeInit(void);


/*
 * 功能：发送命令到MCU端，MCU端的源代码customize/rpc_customer_stub.c中，
 *       会有同名函数进行处理并返回结果。
 *
 * 参数：
 * cmd, 低16bit供客户使用，表示command ID；
 *      高16bit保留，特别的，bit31 RPC_CUSTOMER_FLAG_NEED_CopyBack表示是否需要把参数带回,
 *      如果RPC_CUSTOMER_FLAG_NEED_CopyBack没有置上，那么MCU端的修改不会被带回。
 * arg, 参数指针
 * len，参数字节长度
 *
 * 返回值:
 *      大于等于0表示成功；小于0表示错误码
 */
extern int FH_RPC_Customer_Command(unsigned int cmd, void* arg, unsigned int len);

#endif //__fh_rpc_customer_mpi_h__
