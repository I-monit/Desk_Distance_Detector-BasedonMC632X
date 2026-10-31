#lib
MEDIA_LIBS = -ldci_rtt -lveu -lvou -llcm_module -lfyfb
MEDIA_LIBS += -lsensor_rtt -lmipi_rtt -lrtt_isp -lisp_rtt -lispcore_rtt -lmedia_process_rtt -ldsp_rtt

# TOF (ATK-MS53L1M via UART1) + k-algorithm glue
# 注: firmware_demo 作为联调宿主, 始终编译 tof (独立 FH_TOF_ENABLE 开关在 Kconfig 保留,
#     但此处不再门控, 避免 build server .config 不同步导致漏编)
# 注意: tof/ 在 app 根目录 (与 sample_demo/ 平级), 不能用 $(SAMPLE_DEMO_DIR) 前缀,
#       否则 wildcard 静默返回空 -> 漏编且不报错 (2026-09-28 上板按键无反应的根因)
SRCS += $(wildcard tof/ATK_MS53L1M/*.c)
SRCS += $(wildcard tof/KEY/*.c)
SRCS += $(wildcard tof/alg_glue.c)
# RT-Thread sensor 框架 (vendored v4.1.1, INT/FIFO 已按本板裁剪) + MS53L1M 薄适配层:
# 设备 "tof_ms53l1m" (RT_SENSOR_CLASS_TOF), alg 层经 rt_device_read 取数;
# 不依赖 RT_USING_SENSOR 宏, 此处无条件编入 (同 tof 门控移除的理由, 防漏编)
SRCS += $(wildcard tof/sensor/*.c)
INCS += -Itof
INCS += -Itof/sensor
# TCP ctrl server (PC-Qt GUI 适配): 200ms 状态 JSON 推送 + KEY1 命令注入, msh: svc_qt
# 注: tcp_ctrl/ 同样在 app 根目录 (与 tof/ 平级), 不能加 $(SAMPLE_DEMO_DIR) 前缀 (同上漏编教训)
SRCS += $(wildcard tcp_ctrl/*.c)
INCS += -Itcp_ctrl
# 单路 RTSP 出流 (ch0=560): 限制 libdmc 每组仅开 1 个 RTSP server (裁掉原 562/564 闲置监听)
CFLAGS += -DDMC_RTSP_CHN_NUM=1
# Software guard daemon (看门狗守护 tof_alg + tcp_ctrl, 死亡自动拉起): msh guard_stat / guard_retry
# 注: tof/guard/ 同在 app 根目录, 同上不能用 $(SAMPLE_DEMO_DIR) 前缀 (防 wildcard 静默漏编)
SRCS += $(wildcard tof/guard/guard.c)
# rtthread.h / rtdevice.h / rtconfig.h / finsh_api.h 依赖 (RT-Thread 内核头路径需显式追加)
INCS += -I$(SDKROOT)/kernel/include
INCS += -I$(SDKROOT)/drivers/arch
INCS += -I$(SDKROOT)/drivers/arch/drivers  # pin.h: vendored sensor.h 依赖 struct rt_device_pin_mode
INCS += -I$(SDKROOT)/drivers/ipc
INCS += -I$(SDKROOT)/out/rtconfig
INCS += -I$(SDKROOT)/shell
# NNA_APP
MEDIA_LIBS += -lnnprimary_det_post -lnna_rtt
# VISE: 平台 lib (board_startup.c / media_proc.c) 无条件引用 fh_vise_module_init + vise_read_proc,
#       必须无条件链接 libvise_rtt, 与 FH_APP_OPEN_VISE 无关 (后者只控 vise_demo 业务逻辑)
MEDIA_LIBS += -lvise_rtt
NN_ENABLE = n
ifeq ($(NN_ENABLE_G0), y)
NN_ENABLE = y
else ifeq ($(NN_ENABLE_G1), y)
NN_ENABLE = y
endif
ifeq ($(NN_ENABLE), y)
CFLAGS += -DFH_NN_ENABLE
# Algorithm: pinhole distance calibration (NN bbox + TOF) and realtime W = k * P
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/nna_app/src/algorithm.c)

# NNA obj detect (person detect, 默认开启)
ifeq ($(FH_NN_PERSON_DETECT_ENABLE), y)
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/nna_app/src/sample_nna_obj_detect.c)
CFLAGS += -DFH_NN_DETECT_ENABLE
endif

# NNA face feature locate (主入口, 喂 alg_input_nn)
ifeq ($(FH_NN_FACE_FEATURE_LOCATE_ENABLE), y)
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/nna_app/src/sample_nna_face_feature_locate.c)
MEDIA_LIBS += -lnnprimary_det_post
CFLAGS += -DFH_FACE_FEATURE_LOCATE_ENABLE
endif
endif
# OSD
# 注: TOSD 文字叠加层已裁剪 (font_array.h 1.7MB 字库占 89% .data, 画面信息由 PC 端
#     Qt GUI 呈现); GBOX 检测框保留 (nna 检测结果可视化必需)
ifeq ($(FH_APP_OPEN_OVERLAY), y)
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/overlay/src/sample_common_overlay_v2.c)
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/overlay/src/overlay_gbox.c)
CFLAGS += -DFH_OVERLAY_ENABLE
endif

#sensor
OVOS02K_MIPI_ENABLE = 0
ifeq ($(FH_USING_OVOS02K_MIPI_G0),y)
OVOS02K_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS02K_MIPI_G1),y)
OVOS02K_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS02K_MIPI_G2),y)
OVOS02K_MIPI_ENABLE = 1
endif
ifeq ($(OVOS02K_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_OVOS02K_MIPI
MEDIA_LIBS += -lovos02k_mipi_rtt
endif

OVOS02D_MIPI_ENABLE = 0
ifeq ($(FH_USING_OVOS02D_MIPI_G0),y)
OVOS02D_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS02D_MIPI_G1),y)
OVOS02D_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS02D_MIPI_G2),y)
OVOS02D_MIPI_ENABLE = 1
endif
ifeq ($(OVOS02D_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_OVOS02D_MIPI
MEDIA_LIBS += -lovos02d_mipi_rtt
endif

OVOS04C10_MIPI_ENABLE = 0
ifeq ($(FH_USING_OVOS04C10_MIPI_G0),y)
OVOS04C10_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS04C10_MIPI_G1),y)
OVOS04C10_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS04C10_MIPI_G2),y)
OVOS04C10_MIPI_ENABLE = 1
endif
ifeq ($(OVOS04C10_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_OVOS04C10_MIPI
MEDIA_LIBS += -lovos04c10_mipi_rtt
endif

OVOS05_MIPI_ENABLE = 0
ifeq ($(FH_USING_OVOS05_MIPI_G0),y)
OVOS05_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS05_MIPI_G1),y)
OVOS05_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS05_MIPI_G2),y)
OVOS05_MIPI_ENABLE = 1
endif
ifeq ($(OVOS05_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_OVOS05_MIPI
MEDIA_LIBS += -lovos05_mipi_rtt
endif

OVOS08_MIPI_ENABLE = 0
ifeq ($(FH_USING_OVOS08_MIPI_G0),y)
OVOS08_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OVOS08_MIPI_G1),y)
OVOS08_MIPI_ENABLE = 1
endif
ifeq ($(OVOS08_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_OVOS08_MIPI
MEDIA_LIBS += -lovos08_mipi_rtt
endif

IMX415_MIPI_ENABLE = 0
ifeq ($(FH_USING_IMX415_MIPI_G0),y)
IMX415_MIPI_ENABLE = 1
else ifeq ($(FH_USING_IMX415_MIPI_G1),y)
IMX415_MIPI_ENABLE = 1
endif
ifeq ($(IMX415_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_IMX415_MIPI
MEDIA_LIBS += -limx415_mipi_rtt
endif

SC3336_MIPI_ENABLE = 0
ifeq ($(FH_USING_SC3336_MIPI_G0),y)
SC3336_MIPI_ENABLE = 1
else ifeq ($(FH_USING_SC3336_MIPI_G1),y)
SC3336_MIPI_ENABLE = 1
endif
ifeq ($(SC3336_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_SC3336_MIPI
MEDIA_LIBS += -lsc3336_mipi_rtt
endif

SC5336_MIPI_ENABLE = 0
ifeq ($(FH_USING_SC5336_MIPI_G0),y)
SC5336_MIPI_ENABLE = 1
else ifeq ($(FH_USING_SC5336_MIPI_G1),y)
SC5336_MIPI_ENABLE = 1
endif
ifeq ($(SC5336_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_SC5336_MIPI
MEDIA_LIBS += -lsc5336_mipi_rtt
endif

SC4336_MIPI_ENABLE = 0
ifeq ($(FH_USING_SC4336_MIPI_G0),y)
SC4336_MIPI_ENABLE = 1
else ifeq ($(FH_USING_SC4336_MIPI_G1),y)
SC4336_MIPI_ENABLE = 1
endif
ifeq ($(SC4336_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_SC4336_MIPI
MEDIA_LIBS += -lsc4336_mipi_rtt
endif

SC530AI_MIPI_ENABLE = 0
ifeq ($(FH_USING_SC530AI_MIPI_G0),y)
SC530AI_MIPI_ENABLE = 1
else ifeq ($(FH_USING_SC530AI_MIPI_G1),y)
SC530AI_MIPI_ENABLE = 1
endif
ifeq ($(SC530AI_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_SC530AI_MIPI
MEDIA_LIBS += -lsc530ai_mipi_rtt
endif

OV2735_MIPI_ENABLE = 0
ifeq ($(FH_USING_OV2735_MIPI_G0),y)
OV2735_MIPI_ENABLE = 1
else ifeq ($(FH_USING_OV2735_MIPI_G1),y)
OV2735_MIPI_ENABLE = 1
endif
ifeq ($(OV2735_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_OV2735_MIPI
MEDIA_LIBS += -lov2735_mipi_rtt
endif

SC3335_MIPI_ENABLE = 0
ifeq ($(FH_USING_SC3335_MIPI_G0),y)
SC3335_MIPI_ENABLE = 1
else ifeq ($(FH_USING_SC3335_MIPI_G1),y)
SC3335_MIPI_ENABLE = 1
endif
ifeq ($(SC3335_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_SC3335_MIPI
MEDIA_LIBS += -lsc3335_mipi_rtt
endif

CV2005_MIPI_ENABLE = 0
ifeq ($(FH_USING_CV2005_MIPI_G0),y)
CV2005_MIPI_ENABLE = 1
else ifeq ($(FH_USING_CV2005_MIPI_G1),y)
CV2005_MIPI_ENABLE = 1
else ifeq ($(FH_USING_CV2005_MIPI_G2),y)
CV2005_MIPI_ENABLE = 1
endif
ifeq ($(CV2005_MIPI_ENABLE),1)
CFLAGS += -DFH_USING_CV2005_MIPI
MEDIA_LIBS += -lcv2005_mipi_rtt
endif

DUMMY_SENSOR_ENABLE = 0
ifeq ($(FH_USING_DUMMY_SENSOR_G0),y)
DUMMY_SENSOR_ENABLE = 1
else ifeq ($(FH_USING_DUMMY_SENSOR_G1),y)
DUMMY_SENSOR_ENABLE = 1
else ifeq ($(FH_USING_DUMMY_SENSOR_G2),y)
DUMMY_SENSOR_ENABLE = 1
endif
ifeq ($(DUMMY_SENSOR_ENABLE),1)
CFLAGS += -DFH_USING_DUMMY_SENSOR
MEDIA_LIBS += -ldummy_sensor_rtt
endif

LIBS += $(MEDIA_LIBS)
