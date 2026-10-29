# Hardware test program

## 第一版范围

USART1（PA9/PA10）作为管理口，115200、8N1、无流控。使用支持 ANSI 的串口终端，关闭本地回显，看到 `apollo:~$` 后执行命令。Shell 同时承载测试输出和 Zephyr 日志；USART1 的独立通信测试尚未实现。

```text
fct list
fct info led
fct run all
fct results
fct run led
fct confirm pass
fct run keys
fct status
fct stop
fct i2c probe 0x50
```

最后一条仅为地址语法示例，不代表已核实板上器件地址。探测只发送地址和 STOP，无数据字节；非零结果表示无应答或传输错误，不能直接推断器件损坏。

| ID | 执行方式 | 验证范围与判定 |
|---|---|---|
| `gpio` | 自动 | LED、按键所属 GPIO 控制器就绪；不验证实际电平和线路 |
| `i2c` | 自动 | I2C2 控制器就绪；不验证总线电气状态、从机或器件功能 |
| `led` | 交互 | DS0、DS1 各闪烁三次，结束后人工确认，30 秒内未确认则失败 |
| `keys` | 交互 | 先释放所有键，随后按提示依次按下、释放四键；40ms 采样窗口消抖，总超时 60 秒 |
| `lwip` | 自动 | lwIP UDP/IPv4 127.0.0.1 软件回环收发与载荷校验；不测试物理网口 |
| `can-loopback` | 自动 | CAN1 内部标准/扩展帧回环，不测试收发器和外部总线 |
| `can-tx` / `can-rx` | 显式命令 | 通过 `fct can tx/rx` 指定波特率、格式等参数，外部通信条件见 `can-tests.md` |

`run all` 只运行自动项，交互项记录 SKIP。人工确认只在 LED 测试提示后有效；提前、重复确认都会被拒绝。单项运行只重置该项结果，批量运行重置全部结果；结果只存在 RAM 中，重启丢失。忙时拒绝新测试与手动探测。停止是协作取消：轮询约 10ms，I2C 需等待驱动当前传输结束（驱动超时配置 100ms）。停止后 LED 尝试全部关闭，清理失败会单独打印。

## 硬件依据

所有配置依据 `hardware.md`，不以本机 Zephyr 板文件为硬件事实源。

| 功能 | 文档引脚 | 当前处理 |
|---|---|---|
| DS0 / DS1 | PB1 / PB0 | overlay 修正映射，低有效沿用现有板定义，等待实板核对 |
| WK_UP / KEY2 / KEY1 / KEY0 | PA0 / PC13 / PH2 / PH3 | GPIO 浮空输入，不假设有效电平；先采集稳定释放电平再检测相反电平及恢复 |
| I2C2 SCL / SDA | PH4 / PH5 | 按 H743IIT6 pinctrl 核对 AF4，开漏、100kHz；文档已有外部 4.7kΩ 上拉 |
| Shell USART1 | PA9 / PA10 | 保留管理通道，不与独立 UART 数据测试并行 |

按键默认使用板上现有偏置，不擅自增加上拉/下拉；基线不稳定返回失败。必须在基线采集时释放所有按键。该方法验证状态变化，不证明按键实际有效电平。

## 分层和扩展

```text
Zephyr Shell adapter -> FCT core / test cases -> HAL + OSAL
                                                   |
                                            Zephyr runtime backends
Device tree -> test_fixture adapter -> pure-value pin descriptions
```

`app/fct/shell.c` 和入口是 Zephyr 交互适配；调度与硬件测试不包含 Zephyr 头。`support/test_fixture.h` 是仅供测试的硬件描述契约，不自建 BSP、设备模型或启动编排层。Shell 和 fixture 适配是相对模板架构的明确平台边界。

真实固件由 CMake 选择 `*_runtime.c`，离线骨架校验仍选择原有文件，两者不会同时链接；禁止源内按平台切换行为。现有抽象头不泄漏 Zephyr 类型。

首版真实后端只提供 GPIO 轮询、I2C 事务，以及测试需要的 OSAL 任务创建、互斥量、队列、时间和内存。GPIO 中断返回不支持；CAN1 已补齐停止时配置、启动、停止、异步 TX、RX 过滤器与错误状态，回调可能在 ISR 中执行，FCT 只使用静态事件和短临界区。I2C 单次最多 8 条消息、同一 7 位地址，支持寄存器读的 repeated START；10 位地址返回不支持，最后一条必须 STOP。

OSAL 使用四个固定 4096 字节任务槽，FCT 和 lwIP 各占一个；新增 `osal_task_start` 返回任务句柄，`osal_task_create` 兼容旧调用。任务返回或中止后槽不回收；lwIP 临界区保存/恢复单核中断状态。互斥量、信号量和消息队列从 Zephyr 堆分配；网络自检会临时创建连接及同步资源并在结束后释放。OSAL 错误常量随目标 libc errno 定义，调用方不可写死 Linux 数值。网络集成细节见 `lwip-integration.md`。

新增测试在 FCT 表登记 ID、自动/交互属性、超时、说明和执行函数；执行函数使用 HAL/OSAL，有限等待并检查取消，退出前清理自身输出状态。器件协议与 HAL 后端分离，新增能力按需求扩展契约。

## 覆盖清单与后续工作

| 硬件/接口 | 当前状态 | 后续验证条件 |
|---|---|---|
| LED、四键 | 首版实现，待实板 | 观察 LED 和按提示操作四键 |
| I2C、AP3216C、PCF8574、AT24C02 | 总线诊断实现；器件功能待实现 | 核对地址、寄存器、IO 连接及写保护 |
| MPU9250、WM8978（引脚表列出） | 待确认实装与实现 | 原理图或器件资料，音频输入输出连接 |
| QSPI W25Q256、FMC NAND、SDRAM | 待实现 | 接线、容量及区域核对；存储写测试须显式指定范围 |
| ADC、DAC、触摸检测 | 待实现 | 测试电压或回接条件、量程及通过阈值 |
| USART2/3、RS232/485 | 待实现 | P8/P9 跳线、回环线或外部设备 |
| CAN / USB | CAN 首版实现，USB 待实现 | CAN 已按文档改为 PA11/PA12；P10 与 USB OTG 互斥，详见 `can-tests.md` |
| 以太网 | 待实现 | 与串口等复用关系核对、网线与对端 |
| SD 卡 | 待实现 | 插卡，默认只读，写入需另行显式选择 |
| LCD、触摸、摄像头、扩展模块 | 待确认模块与实现 | 实际模块型号、接线和显示/采集判定方法 |
| RTC、蜂鸣器、红外、单总线等 | 待逐项核对 | 器件参数与必要外部激励；PB12 单总线与 PCF8574 INT 冲突 |

覆盖清单不表示已验证实装；硬件文档中的全部引脚和资源仍需按模块继续细化。首版没有自动擦写任何非易失存储。现有基线的 25MHz HSE、480MHz 系统时钟和 LED 极性仍需实板验证。

## 验证

```powershell
cmake -G Ninja -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

主机测试使用假时间和假 HAL/OSAL，覆盖调度、批量过滤、错误隔离、人工确认、超时、取消、LED 清理、按键消抖、时钟回绕及地址探测边界；不代替真实 Zephyr 后端和实板验证。

固件、两平台骨架分别按 README 构建。检查生成设备树、ELF 起始地址和编译数据库。尚未进行烧录、串口交互、真实队列超时及 I2C 波形验证。

LCD 人工测试：`fct run lcd`，自动批次跳过；详见 [LCD 测试](lcd-tests.md)。

DMA2D：`dma2d` 自动填充/复制并CPU全屏读回，覆盖显存且背光关闭；`lcd-dma2d` 为人工图案测试。

TLS库自检：`fct run tls`，版本、SHA256、RNG及客户端上下文；尚无网络握手。
