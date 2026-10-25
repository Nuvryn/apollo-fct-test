# 项目说明

当前工程为正点原子阿波罗开发板（V1）版本的全功能测试程序，芯片型号为stm32h743iit6

适用平台：zephyr（平台抽象层支持linux，仅保留骨架）

## 功能

- 对板端硬件进行功能测试
- 主要作为zephyr的配置学习

# 编译方式

当前工程采用cmake+ninja+arm none gcc进行构建（使用clangd，切记编译时要生成compile_command.json文件。

```bash
cmake -DPLATFORM=zephyr -S . -B build-zephyr && cmake --build build-zephyr -j   # 骨架校验
```

- `compile_commands.json` 由 `CMakeLists.txt` 的 `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`**默认导出**；上面命令里显式写 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 是冗余保险，clangd 索引依赖该文件。

# 编码规范

参考./docs/embedded-c-coding-spec.md

# 提交规范

每个独立功能、缺陷修复或有意义的开发阶段完成并验证后提交，保持改动可追溯，以功能分类。
同一任务内的多次编辑、修补、必要格式化和测试调整可以合并为一次提交，无需每次小改动立即 commit，也不要求每轮对话都提交。
提交前检查差异，避免混入无关改动；独立缺陷或具有独立验证价值的阶段成果可单独提交。用户明确要求提交或暂不提交时，按用户要求执行。

参考./docs/commit-message-spec.md

# 其他文档

硬件使用情况详见./docs/hardware.md

文档命名采用英文+横划线

调试笔记写入./docs/apollo开发板调试笔记
