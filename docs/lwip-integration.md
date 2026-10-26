# lwIP integration

## 当前阶段

采用 lwIP 2.2.1；Zephyr 负责调度、同步、时间、诊断和内部 RNG。串口 Shell 继续作为管理入口。原生 Zephyr IP/socket 网络栈关闭，已有外设测试保持可用。

```text
fct run lwip
fct results
```

`lwip` 是自动项目，也会由 `fct run all` 执行。测试创建两个 UDP netconn，通过 IPv4 `127.0.0.1` 回环发送并核对完整载荷，随后释放 netbuf、连接和关联同步资源。收包等待 1000ms，FCT 记录 3000ms 协作超时；阻塞的内核/协议栈 API 不会被强制中断，取消在当前操作返回后处理。

**PASS 仅证明软件协议栈回环测试通过，不证明网口、PHY、网线或外部网络可用。** 板端启动和串口测试尚未实测。TCP、ICMP 代码已纳入构建选项但尚未功能验证；带 lwip_ 前缀的 UDP socket 收发另由主机测试验证。

## 系统适配

- OSAL 新增带输出句柄的任务启动、当前任务身份、可嵌套短临界区；Zephyr 从一个任务槽扩为四个永久槽，每槽 4096 字节。任务优先级仍按 0 最低、31 最高映射，返回/中止后槽不回收，不能拿永久槽反复创建短任务。
- 补齐真实 Zephyr 信号量后端，邮箱按指针大小拷贝。lwIP 的超时语义与 OSAL 显式转换；上游 raw API 线程断言保持启用。
- `lwip_port.c` 通过 OSAL 使用系统能力，不包含 Zephyr 头；`lwip_platform.c` 提供 Zephyr 日志、硬件支持的随机数和断言停机。
- `support/network_stack.h` 只暴露初始化及软件自检，不泄漏 lwIP 或 Zephyr 类型；自检要求单调用者，由 FCT 串行化。
- 先启动 FCT，再初始化 tcpip 线程并等待就绪最多 2 秒；异步回调状态使用静态存储，超时后不会访问已释放的等待对象。不可重复启动 tcpip。可返回的初始化/等待错误通过 Shell 日志记录，不停掉已有测试任务；若异步回调稍后完成，自检可以恢复可用。lwIP 内部必需资源创建失败则按上游要求断言停机，不伪装成功。
- lwIP 堆与 Zephyr 系统堆独立，分别 32KiB；pbuf pool 等为额外静态内存。不要将 sys_arch 分配映射回 lwIP 自身内存分配器，避免初始化递归。
- 为随机数开启片内 RNG 与 HSI48，不使用 GPIO，也不启用测试伪随机数发生器。RNG 初始化和时钟仍需实板验证。
- 离线 Zephyr/Linux 骨架不运行 lwIP；新增启动返回不支持，当前任务为 NULL，临界区返回 UINTPTR_MAX 且不提供互斥。

编译命令与原固件相同，`compile_commands.json` 自动包含第三方库与移植文件；上游文件字节保留，来源和规范豁免见 `third_party/lwip/README.md`。

## 物理网口待接入

按 `hardware.md` 核对的 LAN8720 信号：

| 信号 | 引脚 | 注意事项 |
|---|---|---|
| REF_CLK | PA1 | 文档说明由 PHY 输出 50MHz |
| MDIO / MDC | PA2 / PC1 | PA2 同时连接 USART2，需核对 P8 接线 |
| CRS_DV | PA7 | 文档说明连接 PHY |
| RXD0 / RXD1 | PC4 / PC5 | 文档说明连接 PHY |
| TX_EN | PB11 | 与 USART3_RX/P9 复用 |
| TXD0 / TXD1 | PG13 / PG14 | PG14 资源列为 TXD1，但说明文字误写 TXD0，需要原理图或实板复核 |

文档未明确 PHY 地址/strap 和复位连接，当前不编造这些设备树属性。下一阶段需从原理图、厂家例程或实板 MDIO 探测确定，并核对跳线设置。

STM32 Zephyr MAC 驱动依赖网络接口、Ethernet L2 和 `net_pkt`。当前关闭原生网络不等于 MAC 已接给 lwIP；需要确定独立的 L2 收发适配，明确帧复制、所有权、DMA 可访问区、缓存一致性和链路状态通知。当前未修改 Zephyr SDK 或 MAC 驱动，也未启用任何物理 Ethernet 节点。

网卡接好后按顺序验证 PHY ID/链路、ARP、静态 IPv4 Ping、UDP 和 TCP；地址在接入阶段显式配置，不能把软件回环当作这些步骤的通过依据。

## 验证

```powershell
cmake -G Ninja -S tests -B build-tests
cmake --build build-tests -j 1
ctest --test-dir build-tests --output-on-failure
```

主机 `lwip_tests` 使用真实上游协议栈、同一份 sys_arch 和回环实现；pthread OSAL 只用于测试，不取代 Linux 骨架。覆盖有限/无限等待、满/空邮箱、NULL 消息、分配失败、嵌套保护、41 次 netconn UDP 回环、lwip_ 前缀 UDP socket 收发、收包超时和系统资源回收。主机测试不验证 Zephyr 的 IRQ、任务调度、驱动或网口波形。
