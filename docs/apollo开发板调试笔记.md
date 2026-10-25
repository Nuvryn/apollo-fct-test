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
