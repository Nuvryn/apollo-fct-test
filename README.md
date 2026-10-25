# apollo-fct-test

正点原子阿波罗 V1（STM32H743IIT6）全功能测试工程，当前包含平台抽象层骨架和真实 Zephyr 最小启动程序。

## 抽象层

接口移植自 `D:/BaiduSyncdisk/coding-test/platform-framwork-template`：

- `osal/`：任务、互斥量、信号量、消息队列、延时和内存契约。
- `hal/`：I2C、SPI、UART、CAN、GPIO、ADC、PWM、Flash、RNG 契约。
- `support/`：控制台、启动参数、配置来源、CAN 辅助接口。
- `backends/`：Zephyr 骨架及 Linux 骨架（OSAL 目录按 API 家族命名为 `posix`）。

本次保留示例接口，不迁入 Linux 实际驱动、示例业务和第三方库。Linux 骨架由 Zephyr 骨架裁剪平台头依赖得到。Zephyr 使用示例的 `cmake/zephyr_stubs/` 做离线编译检查，尚未接入真实内核、设备树或板端驱动。

未实现的返回值接口返回 `-ENOSYS`（OSAL 为 `OSAL_ERR_NOTSUP`）或 `NULL`。沿用示例的 void 接口无法返回错误，销毁、延时、调度、输出目前为空操作；tick 返回错误码转换后的无符号值，不能用作时间。启动参数提供空默认值；配置加载返回 `-ENOSYS`，不会加载示例 JSON。控制台格式化返回 `-ENOSYS`，没有实际输出。

## 构建

使用 CMake、Ninja、ARM none GCC 检查 Cortex-M7 / FPv5-D16 / hard-float 目标：

```powershell
cmake -G Ninja -DPLATFORM=zephyr '-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake' -S . -B build-zephyr
cmake --build build-zephyr -j
```

仅用本机 C 编译器检查 Zephyr 骨架（另选构建目录，不能在已有缓存中切换工具链）：

```powershell
cmake -G Ninja -DPLATFORM=zephyr -S . -B build-zephyr-host
cmake --build build-zephyr-host -j
```

Linux 接口骨架编译检查（也可在 Windows/MinGW 下执行，不验证 Linux 运行时）：

```powershell
cmake -G Ninja -DPLATFORM=linux -S . -B build-linux
cmake --build build-linux -j
```

默认导出每个构建目录中的 `compile_commands.json`；`.clangd` 使用 `build-firmware`。上述骨架模式产物为三个静态库；真实固件模式见下文。

架构约束见 `docs/architecture-spec.md`，其中 Linux 实现、demo 和扩展 support 契约等描述属于模板的完整架构，当前实现范围以本 README 为准。编码与提交规范见 `docs/embedded-c-coding-spec.md` 和 `docs/commit-message-spec.md`。
## 真实 Zephyr 固件

依赖环境变量 `ZEPHYR_BASE`、`GNUARMEMB_TOOLCHAIN_PATH` 和 `ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb`，SDK 主机工具由 `ZEPHYR_SDK_INSTALL_DIR` 提供。本机已使用 Zephyr 4.4.2 / ARM GCC 14.3.1 验证。

```powershell
cmake -G Ninja -DZEPHYR_FIRMWARE=ON -DBOARD=apollo_stm32h743 -S . -B build-firmware
cmake --build build-firmware -j
```

此模式通过 Zephyr 构建系统编译 `app/main.c`，不使用假 SDK 头，也暂不链接尚未实现的抽象层骨架。控制台为 USART1 PA9/PA10、115200 波特率；启动时输出一行提示。

`boards/apollo_stm32h743.overlay` 以 `docs/hardware.md` 为准覆盖本机板级定义：DS0=PB1、DS1=PB0；删除 Flash 分区与 `zephyr,code-partition`。`prj.conf` 关闭 MCUboot，应用从内部 Flash `0x08000000` 直接启动，不预留 boot 空间。CAN 和未初始化的外部 SDRAM 暂不启用。其余外设仍需逐项配置和验证。

产物为 `build-firmware/zephyr/zephyr.elf`、`zephyr.hex`、`zephyr.bin`；裸 bin 烧录地址为 `0x08000000`。编译成功尚不代表板端运行验证通过。板级基线目前来自本机 Zephyr 的 `boards/alientek/apollo_stm32h743`，在其他源码安装中构建时也需要该板定义；overlay 仅保存项目差异。
