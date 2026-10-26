# lwIP 2.2.1

| 项目 | 内容 |
|---|---|
| 来源 | 本地 `E:/temp_file/lwip-2.2.1`，版本由 `src/include/lwip/init.h` 和上游文件列表确认 |
| 上游 | https://savannah.nongnu.org/projects/lwip/ |
| 许可证 | BSD 3-Clause，见 `vendor/COPYING`；各文件保留原许可证说明 |
| 迁入范围 | `src/include`、`src/core`、`src/api`、`src/netif/ethernet.c` 和 COPYING，共 213 个文件 |
| 修改情况 | `vendor/` 文件未修改；移植代码位于 `port/`、`lwip_port.c` 和工程后端 |
| 校验 | `source-sha256.txt` 记录迁入时逐文件 SHA256；已与本地来源比对 |
| 规范豁免 | 仅 `vendor/` 上游文件豁免自研命名和格式规范，移植代码仍遵守项目规范 |

`.gitattributes` 对 vendor 禁止行尾转换以保留来源字节；格式工具不得处理 vendor。上游 `Filelists.cmake` 会在源码目录生成文件和额外目标，本工程不运行它；使用独立 STATIC 库，只选 core/IPv4、API、Ethernet 通用层编译，不编译 IPv6、应用协议、PPP 等实现。完整头文件保留以保持上游 include 关系。

升级方式：替换上述迁入文件集，更新版本、许可证、SHA256 清单和移植配置，运行固件及主机测试。校验清单用于复现本地迁入内容，不声称已独立验证下载来源或发布签名。

配置为 `NO_SYS=0`，`sys_arch` 经 OSAL 使用线程、邮箱、信号量、互斥量、时间和短临界区。`lwIP timeout=0` 映射为 OSAL 永久等待；尝试操作使用 OSAL 无等待。邮箱保存指针值，包括 NULL；失败创建返回错误，阻塞操作的非预期系统失败触发断言，不能假装成功。

`SYS_LIGHTWEIGHT_PROT` 使用保存/恢复中断状态的可嵌套临界区，当前仅支持单核 Zephyr。邮箱 ISR 投递明确返回 `ERR_IF`，未来网卡 RX 应在任务中投递。关闭全局 TCPIP 核心锁，raw API 只能由 tcpip 线程调用；应用通过 netconn/socket 或 tcpip callback 通信。

IPv4、ARP、ICMP、TCP、UDP、netconn 与带 `lwip_` 前缀的 socket 实现纳入库；实际固件按符号引用裁剪。关闭 BSD 名称别名，避免覆盖 Zephyr/libc；errno 使用目标 libc，不启用上游的 `LWIP_PROVIDE_ERRNO`。堆 32KiB、pbuf pool 16x1536B，tcpip 邮箱 16 项，默认不启用 DHCP/DNS/IPv6。

当前只有 `127.0.0.1` 软件接口，没有 MAC/PHY 的 netif，不会产生物理网口流量。Zephyr 原生网络栈保持关闭。Zephyr STM32 MAC 驱动经 `net_recv_data()` 接入原生网络框架；后续必须单独实现并验证链路层桥接，不能直接将 `net_pkt` 当作 lwIP `pbuf`。
