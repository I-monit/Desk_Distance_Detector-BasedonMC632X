#ifndef __fh_tele_at_h__
#define __fh_tele_at_h__

/*错误码定义*/
#define FH_TELE_AT_ERR_PARAM     (-1001) /*传入参数错误*/
#define FH_TELE_AT_ERR_NO_DEVICE (-1002) /*串口不存在*/
#define FH_TELE_AT_ERR_NO_MEM    (-1003) /*内存不足*/
#define FH_TELE_AT_ERR_SEND      (-1004) /*发送AT命令时出错*/
#define FH_TELE_AT_ERR_TIMEOUT   (-1005) /*等待模块响应超时*/
#define FH_TELE_AT_ERR_BUZY      (-1006) /*模块忙,正在进行PPP拨号*/

/*错误码定义*/
#define FH_TELE_AT_DEBUG_OFF     (0x00) /*关闭debug功能*/
#define FH_TELE_AT_DEBUG_SEND    (0x01) /*dump发送的AT命令*/
#define FH_TELE_AT_DEBUG_RECV    (0x02) /*dump接收到的模块响应*/

#define FH_TELE_AT_IGNORE        (0x80)  /*用于结构体fh_tele_at_cmd中control的bit7*/
struct fh_tele_at_cmd
{
    char *cmd;           /*发送给模块的AT命令*/
    char *expect_resp;   /*期望模块返回的响应*/
    unsigned char  wait_resp;     /*0:不等待模块响应; 1:等待模块响应*/
    unsigned char  retry_times;   /*重试次数*/
    unsigned char  retry_interval;/*重试间隔,100毫秒为单位*/
    unsigned char  control;       /*bit0~6, 这个命令完成后,需要的附加等待时间,100毫秒为单位 */
                       /* bit7,   bit7为1表示可以忽略这个命令的错误,bit7为0表示不可忽略*/
};

/*
 *功能:判断模块是否已插入并被系统识别
 *返回值: 1表示已插入; 0表示没有插入
 */
extern int fh_tele_is_pluged_in(void);

/*
 *功能:判断模块是否已插入并被系统识别
 *返回值: 1表示已插入; 0表示没有插入
 */
extern int fh_tele_at_debug(int debug_level);

/*
 *功能:通过模块的USB串口1发送AT命令并返回模块的响应
 *cmd: 发送到模块的命令
 *response: 用来接收模块的响应,NULL表示忽略模块的响应
 *timeout:  超时参数,毫秒为单位
 *返回值: 0表示成功, 其他表示错误码,见FH_TELE_AT_ERR_XXX
 *注意：response需要尽快使用，下次再次发送AT命令的时候，其内容会被覆盖．
 */
extern int fh_tele_at_send(unsigned char *cmd, unsigned char **response, int timeout);

/*
 * 功能：和fh_tele_at_send类似，区别如下，
 *   fh_tele_at_send通过模块的USB串口1发送AT命令
 *   fh_tele_at_send_oob通过模块的USB串口2发送AT命令，用于在PPP拨号过程中发送一些AT查询命令.
 *注意：USB串口1一定存在，但USB串口2不一定存在，根据不同的模块而定
 */
extern int fh_tele_at_send_oob(unsigned char *cmd, unsigned char **response, int timeout);

/*
 *功能：通过模块的USB串口1发送一个或多个AT命令
 *commands: 命令数组指针
 *command_num：命令个数
 *返回值: 0表示成功, 其他表示错误码,见FH_TELE_AT_ERR_XXX
 */
extern int fh_tele_at_send_multi(struct fh_tele_at_cmd *commands, int command_num);

#endif /*__fh_tele_at_h__*/
