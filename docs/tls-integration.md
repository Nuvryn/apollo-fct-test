# Mbed TLS 3.6 集成

本工程单独编译 Zephyr 工作区的 Mbed TLS **3.6.6 LTS**，未修改模块源码。
来源：Zephyr west 清单 `mbedtls-3.6`，路径 `modules/crypto/mbedtls-3.6`。
默认跟随该模块；可用 `APOLLO_MBEDTLS_SOURCE_DIR` 指定路径，配置与C编译均检查3.6.6版本。
缺失时运行 `west update mbedtls-3.6`。
库采用其上游 Apache-2.0 OR GPL-2.0-or-later 双许可，许可证保留在工作区源码中。
`third_party/mbedtls` 中只有本工程自写的构建与配置，不是复制的第三方源码。

## 与 Zephyr 的关系

Zephyr4.4.2原生Mbed TLS适配面向4.x。因此本工程保持 `CONFIG_MBEDTLS=n`，
从 `third_party/mbedtls/CMakeLists.txt` 独立添加3.6.6的上游CMake构建，
生成 `apollo36_mbedcrypto`、`apollo36_mbedx509`、`apollo36_mbedtls` 静态库并链接app。
该Kconfig为n不意味着没有TLS库，而是没有启用Zephyr原生4.x适配。
禁止与Zephyr原生Mbed TLS/PSA同时启用，避免重名符号与版本混用。
不使用Zephyr原生TLS sockets，也尚未绑定lwIP BIO收发回调。

## 功能配置

- TLS1.2、TLS1.3客户端；不启用TLS服务端、DTLS、文件系统或原生socket适配。
- TLS1.2保留ECDHE-RSA/ECDHE-ECDSA；TLS1.3使用证书与临时密钥交换。
- AES-GCM、SHA256、SHA512、RSA、P-256/P-384/Curve25519等软件算法，
  具体可协商的密码套件仍由算法配置与对端共同决定。
- 3.6内部PSA Crypto实现用于TLS1.3，不链接4.x的独立TF-PSA-Crypto模块。
- Entropy硬件回调直接读取Zephyr RNG设备，不使用固定种子或测试随机源。
- 使用 `k_calloc/k_free` 分配；系统堆从32KiB增为96KiB，TLS输入/输出内容缓冲区各16KiB。
- `mbedtls_ms_time` 使用Zephyr单调毫秒时钟；证书日期检查保持启用。
  真正联网前需要设置可信的日历时间、CA证书及服务器名称，不能用uptime代替证书验证时间。
- 当前所有TLS自检只在FCT单worker执行，未启用线程安全封装；以后并发连接需要重新设计锁。

## 自检

```text
fct run tls
fct results
```

验证库版本、SHA256("abc")已知结果、硬件熵源/CTR-DRBG取数、PSA初始化，
并创建要求证书验证的TLS客户端配置与上下文，随后释放临时资源。
PSA库保持初始化状态，供后续重复调用；该测试不解析真实证书，不发起握手。
上下文为worker独占静态对象，避免占满4KiB任务栈。

主机测试编译真实3.6库与相同后端，采用私有Zephyr shim及仅测试用的确定性熵源，
验证重复初始化、分配清理、随机源缺失/失败及内存不足后的恢复。
主机确定性源不会进入固件，也不用于网络安全。

验证结果：固件编译、Zephyr/Linux骨架、CTest6/6通过。
最终Flash158704B、内部RAM204960B，SDRAM显存仍为1228800B。
`compile_commands.json` 已导出；本阶段未实板运行或进行TLS1.2/1.3网络互通测试。
网络阶段还需完善lwIP实际以太网连接、超时/短读写回调、CA与域名验证、可信时间和握手内存测量。
