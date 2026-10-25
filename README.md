# apollo-fct-test

正点原子阿波罗 V1（STM32H743IIT6）全功能测试工程，当前阶段为平台抽象层骨架。

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

默认导出每个构建目录中的 `compile_commands.json`；`.clangd` 默认使用 `build-zephyr`。产物为 `apollo_osal`、`apollo_hal`、`apollo_support` 三个静态库，不包含可烧录固件。真实 Zephyr 应用后续需要接入 Zephyr 构建系统、板级定义及驱动实现，不能把这些假 SDK 头加入真实固件。

架构约束见 `docs/architecture-spec.md`，其中 Linux 实现、demo 和扩展 support 契约等描述属于模板的完整架构，当前实现范围以本 README 为准。编码与提交规范见 `docs/embedded-c-coding-spec.md` 和 `docs/commit-message-spec.md`。
