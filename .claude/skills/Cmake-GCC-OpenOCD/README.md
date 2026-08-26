# CMake + GCC + OpenOCD

[English](README.md) | 简体中文

面向 AI 助手的 skill：为 STM32（或其他 Cortex-M）裸机工程搭建、审计或修复 **CMake + Arm GNU GCC** 交叉编译构建，并配置 **OpenOCD / pyOCD** 烧录与 VS Code **Cortex-Debug** 调试。

## 功能

- 从零搭建可复现的 CMake 嵌入式工程（`CMakeLists.txt` + `toolchain.cmake` + 链接脚本 + GCC 启动文件）。
- **自动识别芯片宏定义**：搭建/修复工程时会先**询问芯片型号**（如 `STM32F103C8T6`），按密度对照表解析出正确的设备宏（`STM32F10X_MD` 等）并写入 `add_compile_definitions`，避免 `#error "Please select first the target..."`。
- 诊断并修复常见的构建问题：工具链路径、芯片宏、头文件路径、启动文件语法（Keil vs GCC）、旧 CMSIS 汇编约束等。
- 自动生成 `.elf/.hex/.bin/.map` 固件产物并报告 Flash/RAM 占用。
- 生成 VS Code `tasks.json` / `launch.json` 调试配置（pyOCD / OpenOCD 双方案）。
- 提供环境检查脚本和完整的中英双语构建调试指南。

## 环境要求

- CMake ≥ 3.20（Ninja 生成器）
- Ninja
- Arm GNU 工具链：`arm-none-eabi-gcc`、`objcopy`、`size`、`gdb`
- 可选：OpenOCD 或 pyOCD（烧录调试）、VS Code Cortex-Debug 扩展

缺少构建工具时不阻止生成文件，但结果会标记为 `generated, unverified`（仅生成、未验证）。

## 安装

将 `Cmake-GCC-OpenOCD` 目录放到以下任一位置：

- 个人 skills 目录：`$CODEX_HOME/skills/Cmake-GCC-OpenOCD`（或 `~/.claude/skills/`、`~/.cline/skills/`）
- 项目的 `.agents/skills/Cmake-GCC-OpenOCD` 目录

请保持 `SKILL.md`、`assets/`、`references/` 在一起。

## 使用方法

在项目目录中让助手执行：

```text
使用 Cmake-GCC-OpenOCD 为当前工程搭建 CMake + GCC 构建系统
```

或针对已有工程：

```text
使用 Cmake-GCC-OpenOCD 检查并修复当前工程的 CMake 构建配置
使用 Cmake-GCC-OpenOCD 配置 OpenOCD/pyOCD 烧录与调试
```

## 生成的文件

| 文件 | 用途 |
| --- | --- |
| `CMakeLists.txt` | 主构建定义（14 步流程） |
| `toolchain.cmake` | Arm GNU 工具链选择 |
| `*.ld` | 链接脚本（芯片内存布局） |
| `startup_*.s` | GCC 版启动文件 |
| `check_build_env.py` | 项目内环境检查脚本（仅诊断） |
| `.vscode/launch.json` | pyOCD / OpenOCD 调试配置 |
| `.vscode/tasks.json` | 构建任务（F5 前自动编译） |
| `build/output/*.elf/.hex/.bin/.map` | 固件与分析产物 |
| `README.md` | 中英双语构建调试指南 |

## 内置模板

`assets/cmake/` 目录提供可直接参考的模板：

- `CMakeLists.txt.in` — 完整 14 步 CMakeLists 模板
- `toolchain.cmake` — 工具链文件模板
- `check_build_env.py` — 构建环境检查脚本
- `stm32-flash.ld.in` — 链接脚本模板
- `assets/vscode/launch.json.in`、`tasks.json.in` — VS Code 调试配置模板

## 安全与范围

- 不改动现有 IDE 工程，除非用户明确要求。
- 环境检查脚本仅诊断，不安装软件、不修改 `PATH`。
- 不替换或改写用户的应用代码；如发现编译器移植性问题，保持修改最小并说明原因。
- 烧录操作需用户确认后进行。

详细的中文指南见 [references/cmake-gcc-openocd-guide.md](references/cmake-gcc-openocd-guide.md)，agent 工作流程见 [SKILL.md](SKILL.md)。

## 许可证

MIT License。
