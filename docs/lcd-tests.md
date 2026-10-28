# RGB LCD 测试

当前目标：阿波罗 V1 的 J3 接口，正点原子 7 寸 1024×600 RGB 模块（厂家 ID 0x7016）。
引脚以 `hardware.md` 为准，配置参数参考用户提供的“实验15 LTDC LCD（RGB屏）实验”中
`HARDWARE/LCD/ltdc.c`、`ltdc.h` 与 `HARDWARE/SDRAM/sdram.c`。
仅提取硬件参数，未复制厂家驱动源码。其他同分辨率屏幕仍需核对时序和电气接口。

## 使用

断电连接屏幕到 J3，核对 1 脚方向。上电后在 USART1 Shell 输入：

```text
fct run lcd
```

依次显示红、绿、蓝、白、黑、五色彩条和网格，每幅至少 1.2 秒。
检查颜色是否正确、有无缺线、闪烁和画面偏移，网格四周边框是否完整。
最后停留在网格，输入：

```text
fct confirm pass
# 或 fct confirm fail
fct results
```

`fct stop` 可取消。测试总超时 60 秒，结束、取消或错误退出都会尝试关闭背光。
未确认不会通过。`fct run all` 跳过此人工观察项目。
单缓冲绘制过程中可能出现撕裂，此阶段用于静态图案检查。

## 硬件与配置

- 数据线 R3～R7：PH9、PH10、PH11、PH12、PG6。
- 数据线 G2～G7：PH13、PH14、PH15、PI0、PI1、PI2。
- 数据线 B3～B7：PG11、PI4、PI5、PI6、PI7。
- DE=PF10，CLK=PG7，HSYNC=PI10，VSYNC=PI9，均 AF14。
- 背光 PB5，高有效；触摸及 RESET 本阶段未接入软件控制。
- 固定配置该型号，不执行 PG6/PI2/PI7 模块 ID 电平识别。
- 像素时钟 40MHz：25MHz HSE / 5 × 80 / 10（PLL3 R）。
- 水平同步/后肩/前肩：20/140/160；垂直：3/20/12。
- 极性保留厂家 HAL 设置：HSPOLARITY_AL、VSPOLARITY_AL、DEPOLARITY_AL、PCPOLARITY_IPC。
  Zephyr 对应 hsync/vsync/de/pixelclk-active 均为 0。
- LTDC 的 panel pixel-format 必须声明 RGB888（驱动约束），物理连接高位 16 线，显存使用 RGB565。
  不能把此属性误改成显存格式。
- W9825G6KH：16 位，9 列/13 行，4 内部 bank，FMC SDRAM bank 1，32MiB。
- PLL2 R=200MHz，FMC 二分频后 SDCLK=100MHz；CAS2，timing=2/8/6/6/2/2/2。
- 初始化等待 500us，自动刷新 8 次，模式寄存器 0x220，刷新计数 677。
  厂家注释的 `64ms/8192*100MHz-20=677` 算术不准确（实际约761）；
  此处保留更短的 677，约57.1ms完成8192次刷新，作为保守初始值，仍需实板验证。
- 单帧 1,228,800 字节，由 Zephyr LTDC 驱动分配到 SDRAM1 NOLOAD 区域。
  MPU 将该区域标记为正常 RAM，驱动逐行写入并清理数据缓存，供 LTDC 读取。
- SDRAM 由 Zephyr MEMC 初始化，优先级 0；LTDC 初始化优先级 85。
  不将外部 SDRAM 用作系统堆或启动栈。

## 分层与验证边界

`app/fct/lcd_test.c` 只调用 HAL 与 FCT/OSAL；`hal/hal_display.h` 为纯 C 同步行写入接口。
Zephyr 后端调用原生 Display API，Linux 和离线 Zephyr 后端返回 ENOSYS。
设备由 FCT 单 worker 独占，行缓存 2KiB，避免在内部 RAM 中再分配整帧。

主机测试覆盖全部图案像素、人工通过/失败、未确认超时、取消、设备缺失、写失败和背光清理失败。
固件编译与链接检查不能证明屏幕已经点亮，也不代表完整 SDRAM 容量/保持时间测试通过。
尚未实板烧录验证，不含触摸、LVGL、DMA2D 或双缓冲。
