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

使用 OpenOCD 连接 ST-Link / J-Link 烧录固件：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/output/TEST.hex verify reset exit"
```

（按实际调试器接口和输出文件名调整。）

## 注意事项

- 构建目录 `build/`、`output/` 及编译产物已被 `.gitignore` 忽略，请勿手动提交。
- 修改 `CMakeLists.txt` 或新增源文件后，建议重新执行配置步骤再编译。
- 本项目当前仍在开发中，`CMakeLists.txt` 中的构建逻辑可能随开发进度调整。
