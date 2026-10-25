# 阿波罗开发板调试笔记

## 2026-10-25：Zephyr 直接启动构建

- 硬件依据：`hardware.md`，STM32H743IIT6 / 阿波罗 V1。
- 环境：Zephyr 4.4.2，ARM GCC 14.3.1，Ninja，SDK 1.0.1 主机工具。
- 本机 `apollo_stm32h743` 设备树的 DS0/DS1 与文档相反；项目 overlay 修正为 DS0=PB1、DS1=PB0。有效电平暂沿用基线的低有效，待实板核对。
- 无 boot 需求：删除所有 Flash 分区和 chosen 中的 code-partition，关闭 MCUboot 与 DT 分区装载，Flash 装载偏移为 0。
- 暂关闭 CAN 与未经 FMC 初始化的外部 SDRAM；USART1 使用 PA9/PA10、115200。
- 最小应用打印启动提示，尚未接入 OSAL/HAL/support 的真实实现。
- 构建：`cmake -G Ninja -DZEPHYR_FIRMWARE=ON -DBOARD=apollo_stm32h743 -S . -B build-firmware`，随后 `cmake --build build-firmware -j`。
- 验证：真实固件链接通过；ELF 的 `rom_start` 位于 `0x08000000`；Flash 区域为 2MB；生成 DTS 无分区与 code-partition，LED 引脚符合文档；生成 `compile_commands.json`。
- 未执行烧录和实板启动，HSE 25MHz、480MHz 时钟和 LED 有效电平仍需实板确认。

## 2026-10-25：串口测试程序首版

- 真实固件启用 Zephyr UART Shell 和 Shell 日志后端；独立 UART 日志后端关闭，避免破坏提示符。
- FCT 使用 OSAL 工作任务、互斥量和消息队列，GPIO/I2C 使用 HAL；Shell 与设备树转换集中在平台适配中。
- 引脚逐项按 hardware.md 核对：LED PB1/PB0；按键 PA0/PC13/PH2/PH3；I2C PH4/PH5。PH4/PH5 的 AF4 另与 H743IIT6 pinctrl 源码核对。
- 文档未说明按键有效电平和偏置：先使用浮空输入采集稳定释放电平，以变化和恢复判定；不能据此宣称有效电平已验证。
- LED 各闪三次，须人工确认；按键依次完成按下与释放；自动项只验证 GPIO 和 I2C 控制器就绪。
- OSAL 模板硬编码 Linux errno，Zephyr ETIMEDOUT/ENOTSUP 数值不同；改为目标 libc 的负 errno 常量，调用方不得依赖数值。
- 真实后端与骨架通过 CMake 选文件分离；runtime 仅支持一个长期任务，不提供非空任务删除。真实后端尚未进行实板时序验证。
- ARM GCC 首次高并行构建在 picolibc 数学库内部崩溃，降低至 -j2 后完成；主机 GCC 也曾出现临时系统头解析失败，-j1 重编通过，源文件未因此修改。
- 已有主机逻辑测试覆盖忙状态、批量过滤、错误后继续、人工确认、超时、取消、LED 清理、按键消抖、32位时间回绕和探测地址边界。
- 尚未进行烧录、串口 Shell 实际输入、LED/按键实测、I2C ACK 波形、真实 OSAL 等待/调度时序验证。
