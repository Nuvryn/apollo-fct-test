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

## 2026-10-26：lwIP 系统适配与软件回环

- 使用本地 lwip-2.2.1 源码，213 个 vendor 文件逐一比对 SHA256，无源码修改；保留 COPYING，移植文件与源码分离。vendor 禁止 Git 行尾转换以保证来源校验。
- 配置 NO_SYS=0，tcpip 独立线程；OSAL 扩为 4 个固定 4096B 任务槽，新增带句柄启动、当前任务与可嵌套单核 IRQ 临界区，实现信号量。任务槽不回收。
- 处理 lwIP timeout=0 与 OSAL 无等待语义差异，邮箱允许 NULL 消息。系统失败必须返回错误或按上游契约断言；ISR 邮箱投递尚不支持，后续 RX 需任务化。
- 上游 LWIP_PROVIDE_ERRNO 使用 ifdef 判断，不能定义成 0；保持未定义并启用 LWIP_ERRNO_STDINCLUDE，避免覆盖目标 libc errno。
- 原生 Zephyr 网络栈关闭；开启片内 RNG 与 HSI48 提供随机数。硬件资料中的 Ethernet 引脚已核对，但 PHY 地址、复位/strap 和 PG14 文本矛盾尚未核实，不启用 MAC/PHY。
- 新增 fct run lwip 自动项，实际运行 IPv4 UDP 127.0.0.1 软件回环并核对完整载荷；不宣称物理网口通过。
- 主机测试运行真实 lwIP/sys_arch：41 次 netconn UDP 回环、lwip_ 前缀 UDP socket 收发、收包超时、分配失败、满/空邮箱、NULL 消息、信号量等待、嵌套保护与资源回收。FCT 回归包含网络失败不能标记通过。
- Zephyr 固件已链接，未烧录验证；主机 OSAL 仅为 pthread 测试夹具，不代表 Linux 生产后端已实现。RNG 时钟、IRQ 临界区和实际调度还需实板确认。

## 2026-10-26：CAN1 首版测试

- 以 hardware.md 为准，overlay 从评估板 PD0/PD1 改为 PA11 RX、PA12 TX，AF9 已与 H743IIT6 pinctrl 核对；P10 选择 CAN，与 USB OTG 数据线互斥。USART1 USB 转串口继续作为管理通道。
- FDCAN 域时钟显式选择现有 25MHz HSE，clk-divider=1，默认经典 CAN 500kbit/s；该速率是测试默认，不是厂家硬件规定。关闭 CAN FD，开启 RTR 接收以匹配 HAL 帧契约。
- 内部回环使用 M_CAN LOOPBACK|LISTENONLY 组合，验证标准/扩展帧的 ID、格式、长度及变化载荷，不把结果当作收发器或物理总线通过。
- 外部 TX/RX 需要显式命令。TX 等待对端 ACK，RX 正常模式参与 ACK，超时/取消均停止控制器并清理过滤器和待发送项；RX 不把同 ID 的另一格式或 RTR 当作匹配数据帧。
- 新增 HAL 配置/启动/停止与 STOPPED/WARNING 状态；固定异步 TX/RX 上下文，IRQ 回调只更新静态事件，避免使用已经结束的测试栈。排队等待上限 1000ms，FCT 使用 20ms，防止无限排队阻塞停止。
- 主机 FCT 测试覆盖载荷/格式错误、无 ACK/收包超时、取消、bus-off、队列/过滤器错误、清理失败及参数边界。独立编译实际 HAL 后端，以 Zephyr API shim 测试转换、异步完成、池耗尽回收、部分过滤器回滚及旧 token 拒绝。
- 固件编译通过，尚未烧录实板。主机模拟不证明真实 IRQ 时序、内部回环、P10 跳线、电平、终端或外部对端通过。完整重编译可见上游 lwIP sockets.c 的条件编译 unused-function 警告，未修改上游源码。

## 2026-10-28：RGB LCD 初步接入

- 依据实验15与 hardware.md 配置 RGB565 引脚、1024×600 时序和 PB5 背光。
- FMC 独立 200MHz 内核时钟，SDCLK100MHz；启用 MPU 正常 RAM 属性与外部 NOLOAD 帧缓存。
- 保留厂家刷新计数677，记录其注释算术错误；此计数比64ms目标更保守。
- 新增 `fct run lcd` 七幅图案、人工确认、停止/超时及背光清理。
- 主机模拟不等于真实硬件结果；需接屏烧录检查颜色、时序、边框及稳定性。
- 验证：真实 Zephyr 固件、Zephyr/Linux 骨架编译、CTest 4/4 通过。
- 最终占用：Flash 130004B，内部 RAM 129840B，SDRAM 显存1228800B@0xC0000000；入口0x08000000。
- 全部新增 AF12/AF14 引脚已与 STM32H743IITx pinctrl 核对；保留上游 lwIP sockets.c 原有 unused-function 警告。
