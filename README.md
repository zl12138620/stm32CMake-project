# stm32CMake-project

这是一个使用 **CMake + GCC + OPENOCD** 开发 STM32 的例程，基于 **STM32F103C8T6**（Cortex-M3）标准外设库构建。

---

## 硬件平台

| 项目 | 说明 |
| --- | --- |
| MCU | STM32F103C8T6（ARM Cortex-M3） |
| Flash | 64 KB |
| RAM | 20 KB |
| 启动文件 | `Startup/startup_stm32f10x_md.s` |
| 链接脚本 | `Startup/STM32F103C8Tx_FLASH.ld` |

## 目录结构

```
STM32_CMake_Test/
├── CMakeLists.txt              # CMake 构建配置
├── toolchain.cmake             # ARM GCC 交叉编译工具链配置
├── Libraries/
│   ├── CMSIS/                  # CMSIS 内核与设备支持文件
│   └── STM32F10x_StdPeriph_Driver/   # STM32F10x 标准外设库
├── Startup/                    # 启动文件与链接脚本
└── User/                       # 用户代码
```

## 环境要求

| 工具 | 版本要求 |
| --- | --- |
| CMake | ≥ 3.28.1 |
| GNU ARM 工具链 | `arm-none-eabi-gcc`（本项目使用 v15.3） |
| 构建工具 | Make / Ninja |
| OpenOCD（可选） | 烧录与调试 |
| pyOCD（可选） | CMSIS-DAP 调试器烧录与调试 |

> **注意**：`toolchain.cmake` 中的编译器路径 `COMPILE_ROOT_PATH` 需要根据你本机的工具链安装位置修改：

```cmake
set(COMPILE_ROOT_PATH E:/Project/Compiler/arm-gnu-toolchain-15.3.rel1-mingw-w64-i686-arm-none-eabi/bin)
```

## 编译步骤

### 1. 配置

```bash
cmake -S . -B build
```

- `-S .` 指定源码目录为当前目录
- `-B build` 指定构建目录为 `build`（该目录已在 `.gitignore` 中忽略，不会提交到仓库）

### 2. 编译

```bash
cmake --build build
```

编译完成后，产物（`.hex`、`.bin`、`.map` 等）输出在 `build/output/` 目录下。

### 3. 常用 CMake 选项

指定构建类型（Debug 默认启用 `-Og -g`，Release 启用 `-Os` 优化）：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 烧录

### 调试器硬件连接

本项目使用 **FIRE FireDAP（CMSIS-DAP）** 调试器，通过 SWD 接口与 STM32F103C8T6 连接：

| FireDAP 引脚 | STM32F103C8T6 |
| --- | --- |
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3V | 3.3V |

连接完成后可用 `pyocd list` 确认调试器被识别（应显示 `FIRE FireDAP CMSIS-DAP`）。

### 方法一：VS Code 一键烧录 + 调试（推荐）

1. 安装 **Cortex-Debug** 扩展
2. 按 **F5**（或点击"运行和调试"→ 选择 **DAP Debug (pyOCD)**）
3. 自动执行：编译 → 烧录 → 停在 `main()` 进入调试

### 方法二：pyOCD 命令行烧录

```bash
# 烧录 HEX 文件
pyocd flash -t stm32f103c8 build/output/TEST.hex

# 烧录 ELF 文件并复位运行
pyocd flash -t stm32f103c8 build/output/TEST.elf --reset
```

辅助命令：

```bash
pyocd list                        # 查看调试器连接状态
pyocd reset -t stm32f103c8        # 复位芯片
pyocd erase -t stm32f103c8 --chip # 整片擦除 Flash
```

### 方法三：OpenOCD 命令行烧录

```bash
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg -c "program build/output/TEST.hex verify reset exit"
```

### 常见问题

| 现象 | 解决方法 |
| --- | --- |
| `Error: No probe found` | 检查 FireDAP 是否插入 USB，运行 `pyocd list` 确认 |
| `Target not found` | 检查 SWDIO/SWCLK 接线及芯片供电（3.3V） |
| 烧录的是旧固件 | 先执行 `cmake --build build` 重新编译 |
| Flash 编程失败 | 确认目标型号为 `stm32f103c8`（不能用 `stm32f103rc`） |

## 注意事项

- 构建目录 `build/`、`output/` 及编译产物已被 `.gitignore` 忽略，请勿手动提交。
- 修改 `CMakeLists.txt` 或新增源文件后，建议重新执行配置步骤再编译。
- 本项目当前仍在开发中，`CMakeLists.txt` 中的构建逻辑可能随开发进度调整。
