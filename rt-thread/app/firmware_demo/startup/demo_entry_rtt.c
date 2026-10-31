
#include <optparse.h>
#include <rtthread.h>
#include "sample_common.h"
#include "vlcview/vlcview.h"

extern int msh_exec(char *cmd, rt_size_t length); /* shell/msh.c: FSymTab 查表执行, 与交互输入等价 */

typedef unsigned int UINT32;
#define reg_write(addr,value) (*(volatile UINT32 *)(addr)=(value))
#define SET_REG(addr, value)   reg_write(addr, value)
void user_main(void)
{
#if !defined(FH_FAST_BOOT) && !defined(RPC_MEDIA)
    extern FH_SINT32 fh_nna_module_init(FH_VOID);
    int ret;
    ret = fh_nna_module_init();
    if (ret)
    {
        printf("fh_nna_module_init failed %x !\n", ret);
    }
#endif
/* 0x102000xx = pad 功能复用(mux)寄存器, 偏移对应 pinctrl -l 的 addr 列 (cfg 域 0x1c1000xx 是另一套).
   media_demo 遗留的 LCD 复用(FUNC5)会抢占下列 pad, 本管线 VOU 已关无物理 LCD, 必须注释:
     0x38 GPIO2_5  0x3c GPIO2_6(未用)  0x48 UART1_TXD  0x4c UART1_RXD(TOF!)
   实测依据: pinctrl -l 显示 GPIO2_6 val=0x5(LCD_D12), UART1_TXD/RXD 被 -d UART1 修回后 val=0x2. */
/* SET_REG(0x10200030,0x05); */  /* pad10 USB_PWREN: 无 LCD 不需要, 保留 pinctrl 配置 */
/* SET_REG(0x10200034,0x05); */  /* pad11 GPIO1_7 : 同上 */
/* SET_REG(0x10200038,0x05); */  /* pad12 GPIO2_5 : LCD_D13 冲突 */
/* SET_REG(0x1020003c,0x05); */  /* pad13 GPIO2_6 : LCD_D12 冲突 (KEY1 已改 TCP/msh 注入, 不用此脚) */
SET_REG(0x10200040,0x00);       /* pad14 UART0_RXD: FUNC0=UART0, 控制台, 必须保留 */
SET_REG(0x10200044,0x00);       /* pad15 UART0_TXD: FUNC0=UART0, 控制台, 必须保留 */
/* SET_REG(0x10200048,0x05); */  /* pad16 UART1_TXD: LCD_D9 冲突, TOF 串口! */
/* SET_REG(0x1020004c,0x05); */  /* pad17 UART1_RXD: LCD_D8 冲突, TOF 串口! */
SET_REG(0x10200064,0x05);
SET_REG(0x10200068,0x05);
SET_REG(0x1020006c,0x05);
SET_REG(0x10200070,0x05);
SET_REG(0x10200074,0x05);
SET_REG(0x10200078,0x05);
SET_REG(0x1020007c,0x05);
SET_REG(0x10200080,0x05);
SET_REG(0x10200084,0x05);
SET_REG(0x10200088,0x05);
SET_REG(0x102000ac,0x05);
SET_REG(0x102000b0,0x05);
SET_REG(0x102000d4,0x05);
SET_REG(0x102000d8,0x05);

//_vlcview(NULL,1240);

/* 出流方式 (单路 RTSP: ch0=560 摄像头+bbox; 原 ch1=562 与板端 LVGL 已整体移除):
   shell 手动启动:
   1) ifconfig e0 192.168.1.16 netmask 255.255.255.0
   2) svc_rtsp -p 554
 */

SET_REG(0x0c8010c0,0x0c520c41);
SET_REG(0x0c8010c4,0x00052507);
SET_REG(0x0c8010c8,0x2307b9ab);
SET_REG(0x0c8010cc,0x00000000);
SET_REG(0x0c801008,0xffffffff);

/* fh BSP 链接脚本无 .rti_fn 段, INIT_*_EXPORT 全部失效 (实测 __rt_init_start==__rt_init_end),
   tof_alg 守护线程 (TOF + KEY1 + k算法标定) 必须在此显式拉起 */
{
    extern int alg_start(void);
    alg_start();
}

/* 开机自启: 网络/RTSP/TCP 三连 (原手工 msh 操作), 无需用户输入, 见文件尾 bootrun */
{
    extern void bootrun(void);
    bootrun();
}

/* 软件看门狗守护: tof_alg + tcp_ctrl 死亡自动拉起 (连续 5 次失败停手, msh guard_retry 手动恢复);
   媒体管线心跳冻结检测+告警 (不自动重启: SDK 热重启有已知缺陷, 见 guard.c 头注释),
   msh guard_stat 一览 / guard_retry [alg|tcp|media] 手动恢复 */
{
    extern int guard_start(void);
    guard_start();
}
}

FH_VOID svc_rtsp_exit(FH_VOID)
{
#ifdef UVC_ENABLE
    printf("UVC not support exit!\n");
    return;
#endif
    _vlcview_exit();

    buffer_reset_vmm(); /* decomment it after lib reupdate  */
}

FH_VOID svc_rtsp_usage(FH_VOID)
{
    printf("\nusage:  svc_rtsp  [-i: | p: | -q | -h]\n\n");
    printf("        -i ip    :ip address (only for pes)\n");
    printf("        -p port  :port (only for pes and rtsp)\n");
    printf("        -q       :quit\n");
    printf("        -h       :help\n\n");
    printf("example: \n");
    printf("        svc_rtsp -i 10.81.187.8 -p 1234\n");
    printf("        svc_rtsp -q\n");
}

FH_VOID svc_rtsp(int argc, char *argv[])
{
    struct optparse options;
    FH_CHAR *dst_ip = NULL;
    unsigned int port = 1234;
    int cmd;
    int get_cmd = 0;

    optparse_init(&options, argv);
    while ((cmd = optparse(&options, "i:p:qh")) != -1)
    {
        get_cmd = 1;
        switch (cmd)
        {
        case 'i':
        {
            dst_ip = options.optarg;
            break;
        }
        case 'p':
        {
            port = strtol(options.optarg, NULL, 0);
            break;
        }
        case 'q':
        {
            svc_rtsp_exit();
            return;
        }
        default:
        {
            svc_rtsp_usage();
            return;
        }
        }
    }

    if (!get_cmd)
    {
#ifndef UVC_ENABLE
        svc_rtsp_usage();
        return;
#endif
    }

    _vlcview(dst_ip, port);
}

#include <rttshell.h>
SHELL_CMD_EXPORT(svc_rtsp, svc_rtsp());
/* ===== 开机自启脚本: 替代手工 msh 三连, 由 user_main 末尾自动执行 =====
   命令经 msh_exec 走 FSymTab 查表, 与交互输入完全等价 (telnet.c 同款用法)。
   调试: svc_rtsp -q / svc_qt stop 退掉服务后, msh 手敲 bootrun 可重新拉起全套。 */
void bootrun(void)
{
#define BOOTRUN_EXEC(c) msh_exec((char *)(c), rt_strlen(c))

    rt_kprintf("[bootrun] autostart: ifconfig -> svc_rtsp -p 554 -> svc_qt\n");

    /* 1) 静态 IP (C 类自动掩码 255.255.255.0, 与 PC 直连; e0 由 eth 驱动注册, 小延时避时序) */
    rt_thread_mdelay(500);
    BOOTRUN_EXEC("ifconfig e0 192.168.1.16");

    /* 2) RTSP 出流 (ch0=560 摄像头+bbox), 同步初始化数秒后返回 */
    BOOTRUN_EXEC("svc_rtsp -p 554");

    /* 3) TCP 6000 状态/标定服务 (Qt GUI), tcp_ctrl_start 幂等 */
    BOOTRUN_EXEC("svc_qt");

    rt_kprintf("[bootrun] autostart done\n");

#undef BOOTRUN_EXEC
}
SHELL_CMD_EXPORT(bootrun, autostart: ifconfig e0 + svc_rtsp -p 554 + svc_qt);
