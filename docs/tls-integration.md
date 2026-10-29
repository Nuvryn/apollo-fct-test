# Zephyr 原生 TLS 集成

当前使用 Zephyr4.4.2 原生 Mbed TLS **4.1.0**，以及其配套的 **TF-PSA-Crypto**。
模块和版本均由Zephyr west清单管理，未修改外部库源码。
已移除此前独立构建Mbed TLS3.6.6的CMake、配置和私有适配；3.6模块可继续留在工作区，
但不参与本工程固件编译。TLS相关配置集中在 `prj.conf`。

## 编译与配置

- `CONFIG_MBEDTLS=y`、`CONFIG_MBEDTLS_BUILTIN=y`，使用Zephyr原生库配置与平台适配。
- `CONFIG_PSA_CRYPTO=y`，由配套TF-PSA-Crypto提供密码算法。
- TLS1.2和TLS1.3均启用；配置ECDHE-RSA/ECDHE-ECDSA、临时密钥交换、AES-GCM、
  SHA256/SHA384、RSA签名及P-256/P-384。具体套件由编译配置与对端协商决定。
- Zephyr通用TLS配置同时编译客户端/服务端支持，当前自检只创建客户端。
- `CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG=y`，使用Zephyr RNG设备提供随机数据，
  禁止回退到非安全测试随机源。硬件RNG仍使用已有设备树配置。
- Zephyr负责启动时初始化PSA、专用堆和毫秒时钟；应用不再实现entropy、calloc或计时钩子。
- Mbed TLS专用堆64KiB，系统堆恢复32KiB；TLS收发内容缓冲区各16KiB。
  这些是初始容量配置，真实握手峰值仍需测量。
- 保留证书日期验证；真正联网前需提供可信日历时间、CA证书及服务器名称。
- `CONFIG_NETWORKING=n`保持不变，IP仍由lwIP负责；启用TLS库不等于切换IP协议栈。

## 自检

```text
fct run tls
fct results
```

使用库版本API检查头文件与链接库一致，通过PSA计算SHA256("abc")并比较已知结果，
再调用PSA随机数API并创建要求证书验证的TLS客户端配置与上下文，结束时释放客户端资源。
Zephyr管理的全局PSA状态继续保留。客户端上下文由FCT单worker独占，避免占用4KiB任务栈。

本阶段未绑定lwIP BIO收发回调，没有真实证书解析或网络握手。
未进行实板运行、TLS1.2/1.3服务器互通或随机源统计质量测试。
后续联网还需实现短读写/超时适配、CA与域名验证、可信时间和握手内存测量。

## 验证范围

- 真实Zephyr固件编译与链接检查，确认来源为 `modules/crypto/mbedtls` 和
  `modules/crypto/tf-psa-crypto`，不含 `mbedtls-3.6` 编译项。
- Zephyr/Linux离线骨架编译。
- CTest保留5项；FCT主机测试验证tls命令的调度、成功与失败结果。
  此前针对独立3.6库的第6项测试随该实现一起移除，不作为4.x测试结果。
- 真实原生库及PSA自检需要烧录后运行 `fct run tls`。

当前固件Flash142184B、内部RAM202904B，SDRAM显存仍为1228800B。
