# apollo-fct-test

正点原子阿波罗 V1（STM32H743IIT6）硬件功能测试与 Zephyr 学习工程。当前已接入串口 Shell、统一测试调度、LED/按键交互测试、GPIO/I2C 控制器诊断及经典 CAN 内部/外部测试。全部硬件覆盖仍在逐项实现中。

## 固件构建

环境变量：`ZEPHYR_BASE` 指向源码；`ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb`；`GNUARMEMB_TOOLCHAIN_PATH` 指向 ARM GCC；`ZEPHYR_SDK_INSTALL_DIR` 提供 SDK 主机工具。本机验证环境为 Zephyr 4.4.2、ARM GCC 14.3.1、SDK 1.0.1。

```powershell
cmake -G Ninja -DZEPHYR_FIRMWARE=ON -DBOARD=apollo_stm32h743 -S . -B build-firmware
cmake --build build-firmware -j 2
```

本机 Zephyr 源码需要已有 `boards/alientek/apollo_stm32h743`。工程 overlay 根据 `docs/hardware.md` 修正其引脚和分区；无 boot，应用从内部 Flash `0x08000000` 直接启动。固件位于 `build-firmware/zephyr/zephyr.elf`、`zephyr.hex`、`zephyr.bin`；bin 烧录地址为 `0x08000000`。

默认导出 `build-firmware/compile_commands.json`，`.clangd` 使用此目录。真实固件不使用假 SDK 头。编译成功不代表实板测试通过。

## 使用

USART1 PA9/PA10，115200、8N1、无流控；串口终端关闭本地回显。进入 `apollo:~$` 后：

```text
fct list
fct run all
fct results
fct run led
fct confirm pass
fct run keys
fct run lwip
fct run can-loopback
fct stop
```

批量执行只运行自动项；LED 需要观察确认，按键需要先释放全部按键再按提示操作。当前自动项只证明控制器就绪，不证明器件和线路功能正常。

命令、接线依据、覆盖边界和测试方法见 [test-program.md](docs/test-program.md)，调试过程见 [调试笔记](docs/apollo开发板调试笔记.md)。尚未烧录验证。

网络协议栈采用 lwIP 2.2.1，已实现基于 OSAL 的系统适配和 UDP/IPv4 软件回环自检；`fct run lwip` 不验证 MAC/PHY 或网线。Zephyr 原生 IP 栈保持关闭，物理网卡收发尚未接入。来源、配置与后续硬件核对项见 [lwip-integration.md](docs/lwip-integration.md)。

CAN1 已按文档使用 PA11/PA12（P10），当前固件不可同时使用这些引脚的 USB OTG；USART1 USB 转串口不受影响。`fct run can-loopback` 只验证控制器内部标准/扩展帧；外部收发通过 `fct can tx/rx` 显式指定参数，不会自动执行。命令、接线及判定范围见 [can-tests.md](docs/can-tests.md)，尚未实板验证。

## 分层

接口来源：`D:/BaiduSyncdisk/coding-test/platform-framwork-template`。

- `osal/`、`hal/`、`support/` 为公共契约，不含 Zephyr 类型。
- 测试调度与测试步骤通过 HAL/OSAL 访问硬件和系统能力。
- `app/fct/shell.c` 使用 Zephyr 原生 Shell；fixture 适配将设备树转换为普通引脚描述。
- 真实固件通过 CMake 选择 `*_runtime.c` 后端；未覆盖的能力继续保留骨架，Linux 不提供真实测试运行环境。

架构细节及相对模板的适配边界见测试文档；模板架构、编码和提交规范位于 `docs/`。

## 离线骨架校验

以下产物为三个静态库，不是固件。未实现接口返回不支持、NULL 或文档说明的空操作，不能用于真实硬件测试。

```powershell
cmake -G Ninja -DPLATFORM=zephyr '-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake' -S . -B build-zephyr
cmake --build build-zephyr -j 2
cmake -G Ninja -DPLATFORM=linux -S . -B build-linux
cmake --build build-linux -j 2
```

每个构建目录均导出 `compile_commands.json`；切换工具链时必须使用不同目录。Zephyr 离线校验使用 `cmake/zephyr_stubs/`，不可将其加入真实固件。

## 主机逻辑测试

```powershell
cmake -G Ninja -S tests -B build-tests
cmake --build build-tests -j 1
ctest --test-dir build-tests --output-on-failure
```

主机测试使用假时间和假硬件，只验证调度、状态及测试序列，不替代实板验证。

RGB LCD（7 寸 1024×600）：`fct run lcd`，详见 [LCD 测试](docs/lcd-tests.md)。

DMA2D：`fct run dma2d` 自动读回校验，`fct run lcd-dma2d` 显示图案并人工确认，详见 [LCD 测试](docs/lcd-tests.md)。

Zephyr 原生 Mbed TLS 4.1.0：`fct run tls` 验证库初始化，详见 [TLS 集成](docs/tls-integration.md)。
