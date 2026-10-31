1. 功能说明

bsp demo中包含多种设备的简单使用示例，例如aes、gpio、i2c、pwm、rtc、sadc、sdio及uart。
每种设备的示例代码位于对应的文件夹中。

2. bsp demo使用方法说明

- make xxxx_defconfig, 其中xxxx为芯片型号，如fh8858v200
- make menuconfig 选择app demo为bsp demo
- make all 生成bsp_demo.bin
- 加载bsp_demo.bin, 进入系统后自动运行demo流程。
