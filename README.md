# STM32 CMake + GCC 工程编写指南

> 基于本工程（STM32F103C8T6 + 标准外设库）的 `CMakeLists.txt` 与 `toolchain.cmake` 总结的**通用编写流程**，可复用于其他 STM32 / 嵌入式工程的编译与调试。

---

## 一、构建系统组成

一个可编译、可调试的 CMake 嵌入式工程由以下文件组成：

| 文件 | 作用 | 需要关注 |
| --- | --- | --- |
| `CMakeLists.txt` | 构建脚本：定义目标、源文件、编译/链接选项 | 每次工程必写 |
| `toolchain.cmake` | 交叉编译工具链：指定 ARM GCC 编译器路径 | 每次工程必写 |
| `*.ld`（链接脚本） | 内存布局：Flash/RAM 地址、段分配 | 随芯片型号选择 |
| `startup_*.s`（启动文件） | 中断向量表、复位入口、data/bss 初始化 | 必须用 **GCC 版**（GNU 汇编语法） |
| `.vscode/launch.json` | VS Code 调试配置（烧录 + Debug） | 可选但强烈推荐 |
| `.vscode/tasks.json` | VS Code 构建任务 | 可选 |

**构建整体流程：**

```
CMakeLists.txt ──配置(cmake -S . -B build)──▶ 生成 Ninja/Make 构建文件
                                                    │
toolchain.cmake（交叉编译器）                        ▼
                                            编译(arm-none-eabi-gcc) → .o
                                                    │
                                        链接(ld + 链接脚本.ld) → .elf
                                                    │
                          POST_BUILD(objcopy/size) → .hex / .bin / .map
```

---

## 二、编写流程总览

```
① 声明版本与策略  →  ② 设置目标平台  →  ③ 指定工具链  →  ④ project() 声明语言
        ↓
⑤ 构建类型判断(Debug/Release)  →  ⑥ 芯片宏定义  →  ⑦ 头文件路径  →  ⑧ 收集源文件
        ↓
⑨ 输出路径与文件名  →  ⑩ 启动文件 + 链接脚本  →  ⑪ 编译选项  →  ⑫ 链接选项
        ↓
⑬ add_executable()  →  ⑭ POST_BUILD 生成 hex/bin/map
```

下面按 `toolchain.cmake` → `CMakeLists.txt` 的顺序详细讲解。

---

## 三、toolchain.cmake 详解

交叉编译的关键：让 CMake 使用 **ARM 交叉编译器** 而不是本机编译器。

```cmake
# 编译目标设置为静态库：避免配置阶段 test 项目尝试链接可执行文件
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 只在本机查找编译程序，库/头文件只在工具链根目录下查找（不污染本机环境）
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 开关：是否启用 ARM GCC 工具链（可配合 CMake -DUSE_ARMGCC=OFF 关闭）
option(USE_ARMGCC "USE ARMGCC" ON)

if(USE_ARMGCC)
    # ★★★ 唯一需要按本机修改的地方：工具链安装目录 ★★★
    set(COMPILE_ROOT_PATH E:/Project/Compiler/arm-gnu-toolchain-15.3.rel1-mingw-w64-i686-arm-none-eabi/bin)

    # C 编译器
    set(CMAKE_C_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    # 汇编器（.s 启动文件用它编译）
    set(CMAKE_ASM_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    # 生成 hex/bin 的转换工具
    set(CMAKE_OBJCOPY ${COMPILE_ROOT_PATH}/arm-none-eabi-objcopy.exe)
    # 查看固件大小
    set(CMAKE_SIZE ${COMPILE_ROOT_PATH}/arm-none-eabi-size.exe)
endif()
```

### 各命令的作用

| 命令 | 作用 | 说明 |
| --- | --- | --- |
| `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` | 跳过可执行文件链接测试 | 无 C 库的裸机工程必须，否则配置阶段报链接错误 |
| `CMAKE_C_COMPILER` | 指定 C 交叉编译器 | 必须 |
| `CMAKE_ASM_COMPILER` | 指定汇编器 | 必须，否则 `.s` 启动文件无法编译 |
| `CMAKE_OBJCOPY` | 生成 `.hex/.bin` | 配合 POST_BUILD 使用 |
| `CMAKE_SIZE` | 打印 Flash/RAM 占用 | 配合 POST_BUILD 使用 |

> 💡 **换工程注意事项**：换工具链版本只需改 `COMPILE_ROOT_PATH`；Windows 用 `;` 分隔、路径用正斜杠 `/` 或双反斜杠 `\\`。

---

## 四、CMakeLists.txt 分步讲解

### 步骤 ① 声明版本与策略

```cmake
cmake_minimum_required(VERSION 3.28.1)   # 最低 CMake 版本（本项目用 Ninja 需 ≥3.20）
cmake_policy(SET CMP0123 NEW)            # ARMClang 的 -mcpu 由项目自己指定，不让 CMake 自动加
```

- `cmake_minimum_required` 必须放在第一行。
- `CMP0123`：CMake 3.21+ 的策略——默认不再为 ARMClang 自动添加 `-mcpu=`，改由项目显式指定（本项目就是手动加的 `-mcpu=cortex-m3`）。对 GNU GCC 工具链无影响，但建议保留。

### 步骤 ② 设置目标平台

```cmake
set(CMAKE_SYSTEM_NAME Generic)     # 交叉编译固定写法：通用系统（非 Windows/Linux）
set(CPU_CORE cortex-m3)            # 芯片内核：STM32F1 为 cortex-m3
set(CMAKE_SYSTEM_PROCESSOR ${CPU_CORE})
```

- 定义了 `CPU_CORE` 变量，后续编译选项直接引用，换芯片只改一处。
- 常见内核：`cortex-m0`（F0）、`cortex-m3`（F1）、`cortex-m4`（F3/F4）、`cortex-m7`（F7/H7）。

### 步骤 ③ 指定工具链文件

```cmake
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)
```

- `CMAKE_CURRENT_LIST_DIR` 是当前脚本所在目录，用相对路径引用同目录下的 `toolchain.cmake`。

### 步骤 ④ project() 声明语言

```cmake
project(TEST C ASM)
```

- `project(名称 语言...)`：**必须声明 `ASM`**，否则 `.s` 启动文件不会被编译。
- 项目名 `TEST` 会成为 `PROJECT_NAME` 变量，后面输出文件名都引用它。

### 步骤 ⑤ 构建类型判断（Debug / Release）

```cmake
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
if(BUILD_TYPE_UPPER STREQUAL "RELEASE")
    add_compile_options(-Os)      # Release：尺寸优化
else()
    add_compile_options(-O0 -g)   # Debug（默认）：不优化 + 调试信息
endif()
```

- 用 `cmake -DCMAKE_BUILD_TYPE=Release` 控制，不传则走 `else()`（Debug）。
- 注意正确的比较写法是 `if(BUILD_TYPE_UPPER STREQUAL "RELEASE")`，`elseif()` 空语句是错误写法。

### 步骤 ⑥ 芯片宏定义（决定编译哪款芯片/是否启用外设库）

```cmake
add_compile_definitions(STM32F10X_MD USE_STDPERIPH_DRIVER)
```

- `STM32F10X_MD`：芯片型号宏。**必须在 `stm32f10x.h` 中有一个匹配的宏**，否则会报 `#error "Please select first the target STM32F10x device..."`。
  - 常用对照：LD=低密度、MD=中等密度(C8/CB)、HD=高密度(ZE/VC)、XL=超密度。
- `USE_STDPERIPH_DRIVER`：启用标准外设库，让 `stm32f10x.h` 自动包含 `stm32f10x_conf.h`（否则报 `assert_param` 未定义）。

#### 深入：`USE_STDPERIPH_DRIVER` 宏的完整引入指南

**1. 这个宏到底是干什么的？**

`stm32f10x.h` 的最后（约第 8341 行）有一段条件编译：

```c
#ifdef USE_STDPERIPH_DRIVER
  #include "stm32f10x_conf.h"   // 只有定义了 USE_STDPERIPH_DRIVER，才会自动包含外设库配置文件
#endif
```

注意：`USE_STDPERIPH_DRIVER` 本身**不会在 `stm32f10x.h` 里被定义**（该文件第 104 行默认是注释状态 `/*#define USE_STDPERIPH_DRIVER*/`），它只能由外部传入。定义后：

- `stm32f10x.h` 自动包含 `stm32f10x_conf.h` → 再包含全部外设头文件（GPIO / RCC / EXTI / NVIC...）；
- 外设库源码里的 `assert_param()` 才有定义，否则报 `implicit declaration of 'assert_param'`。

**2. 三种引入方式对比**

| 方式 | 写法 | 优缺点 |
| --- | --- | --- |
| ① **CMake 编译器宏**（推荐） | `add_compile_definitions(STM32F10X_MD USE_STDPERIPH_DRIVER)` | 全局生效、可随构建类型切换；等价于每个文件编译时加 `-D` 参数 |
| ② 改库文件 | 取消 `stm32f10x.h` 第 104 行注释：`#define USE_STDPERIPH_DRIVER` | ❌ 不推荐：污染库文件；工程里可能有多份 `stm32f10x.h` 拷贝，容易漏改 |
| ③ 源码里 define | 在 `#include "stm32f10x.h"` **之前**写 `#define USE_STDPERIPH_DRIVER` | 可行但每个用到外设库的 `.c` 都得写，繁琐易漏 |

**3. 推荐做法（本项目采用）与验证**

```cmake
# CMakeLists.txt —— 宏对 add_executable 收集的所有源文件生效
add_compile_definitions(STM32F10X_MD USE_STDPERIPH_DRIVER)
```

编译时等价于给每个文件加参数：

```bash
arm-none-eabi-gcc ... -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER ... main.c
```

验证宏是否真的传给了编译器（两种方法任选）：

```bash
# 方法一：看 build.ninja 里的 DEFINES（无需额外配置）
Select-String -Path build/build.ninja -Pattern 'DEFINES' | Select-Object -First 1
# 输出应为：DEFINES = -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER

# 方法二：看 compile_commands.json（需先在 CMakeLists.txt 加 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)）
Select-String -Path build/compile_commands.json -Pattern 'USE_STDPERIPH_DRIVER'
```

**4. 编译过了 ≠ 编辑器不报错（IntelliSense 排查）**

如果 `cmake --build` 编译通过，但 VS Code 里 `EXTI9_5_IRQn`、`assert_param` 等仍划红色波浪线——**那是编辑器智能感知没拿到宏，不是编译错误**。因为 `EXTI9_5_IRQn` 定义在 `stm32f10x.h` 的 `#ifdef STM32F10X_MD` 块内（约第 246~252 行），宏缺失时整个枚举被预处理器剔除。

解决（任选其一，建议都做）：

```cmake
# ① CMakeLists.txt 开启导出编译命令，重新 cmake 配置后
#    VS Code C/C++ 扩展会自动读取 build/compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

```jsonc
// ② 或手动创建 .vscode/c_cpp_properties.json，在 defines 里补上宏
{
    "configurations": [{
        "name": "STM32",
        "defines": ["STM32F10X_MD", "USE_STDPERIPH_DRIVER"]
        // ... 还要配 includePath，见步骤⑦
    }]
}
```

改完按 `Ctrl+Shift+P` → `C/C++: Reset IntelliSense Database` 重载即可。

**5. 相关报错速查**

| 现象 | 原因 |
| --- | --- |
| `#error "Please select first the target STM32F10x device..."` | 没定义芯片型号宏（`STM32F10X_MD`） |
| `implicit declaration of 'assert_param'` | 没加 `USE_STDPERIPH_DRIVER` 宏 |
| 编译通过但编辑器红色波浪线 `EXTI9_5_IRQn` | IntelliSense 缺宏（见第 4 点） |

### 步骤 ⑦ 头文件路径

```cmake
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x   # ★ stm32f10x.h 所在目录
    ${CMAKE_CURRENT_LIST_DIR}/User
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/inc
)
```

- 每个头文件所在目录都必须加进来，**漏掉任何一层都会 `fatal error: xxx.h: No such file or directory`**。
- 标准外设库至少要包含：`CoreSupport`（core_cm3.h）、`DeviceSupport/ST/STM32F10x`（stm32f10x.h）、外设驱动 `inc`、用户目录 `User`。

### 步骤 ⑧ 收集源文件

```cmake
file(GLOB SOURCE_FILE
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/User/*.c
)
```

- 用通配符自动收集所有 `.c` 文件到变量 `SOURCE_FILE`。
- 注意命令是 `file(GLOB ...)`，拼成 `GOLB` 会报 "Unknown CMake command"。
- 💡 局限：新增文件后需要重新运行 `cmake` 配置才会生效（Ninja 增量构建不会自动感知 GLOB 变化）。

### 步骤 ⑨ 输出路径与文件名

```cmake
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output)   # 输出到 build/output/
set(HEX_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.hex)
set(BIN_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.bin)
set(MAP_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.map)
```

- `CMAKE_BINARY_DIR` = `build` 目录。用变量统一管理输出路径，后续 `set_target_properties` 和 POST_BUILD 都引用它。
- ⚠️ 变量引用必须写成 `${变量}`，写成 `&{变量}` 会被原样当字符串，导致路径错误。

### 步骤 ⑩ 启动文件与链接脚本

```cmake
if(USE_ARMGCC)
    SET(STARTUP_FILE ${CMAKE_CURRENT_LIST_DIR}/Startup/startup_stm32f10x_md.s)
    SET(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/Startup/STM32F103C8Tx_FLASH.ld)
```

- 启动文件 `.s` 和链接脚本 `.ld` 都是**按芯片型号选**的（md = Medium Density）。
- `USE_ARMGCC` 开关来自 `toolchain.cmake` 的 `option()`。

### 步骤 ⑪ 编译选项

```cmake
    add_compile_options(
        -mcpu=${CPU_CORE}     # 内核型号（引用步骤②变量）
        -mthumb               # 使用 Thumb 指令集
        -ffunction-sections   # 每个函数单独成段（配合 gc-sections 裁剪）
        -fdata-sections       # 每个数据单独成段
        -fno-common
        -fmessage-length=0
    )
```

- `-ffunction-sections -fdata-sections` 与链接选项 `-Wl,--gc-sections` 成对使用，可剔除未用代码，显著减小固件。

### 步骤 ⑫ 链接选项

```cmake
    add_link_options(
        -Wl,-Map=${MAP_FILE}           # 生成 .map 内存映射文件
        -Wl,--print-memory-usage       # 链接时打印 Flash/RAM 占用
        -Wl,--gc-sections              # 删除未使用段
        -T ${LINKER_SCRIPT}            # ★ 指定链接脚本
    )
endif()
```

- `-T` 指定链接脚本，缺了会按默认布局链接，程序无法运行。
- `--print-memory-usage` 会在链接完成后打印：

```
Memory region         Used Size  Region Size  %age Used
             RAM:        1976 B        20 KB      9.65%
           FLASH:        1496 B        64 KB      2.28%
```

### 步骤 ⑬ 生成可执行目标

```cmake
add_executable(${PROJECT_NAME} ${SOURCE_FILE} ${STARTUP_FILE})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}   # 输出目录
    OUTPUT_NAME ${PROJECT_NAME}.elf                      # 生成 TEST.elf
)
```

- `add_executable(目标名 源文件列表)`：把 C 源文件 + 启动文件一起编进目标。
- `OUTPUT_NAME` 让输出文件带 `.elf` 后缀（调试器识别更准确）。

### 步骤 ⑭ POST_BUILD 生成 hex/bin/map

```cmake
if(USE_ARMGCC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${BIN_FILE}
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    )
endif()
```

- `add_custom_command(TARGET ... POST_BUILD)`：**链接完成后自动执行**，把 `.elf` 转换为 `.hex`（Intel HEX，烧录用）和 `.bin`（裸二进制），并打印固件大小。
- `$<TARGET_FILE:${PROJECT_NAME}>` 是生成器表达式，自动展开为 elf 的完整路径（**注意必须闭合尖括号** `>`）。
- ⚠️ 语法坑：`POST_BUILD` 是 `add_custom_command` 的选项，不能用在 `add_custom_target` 里。

---

## 五、配套文件：启动文件与链接脚本

### 启动文件（最容易踩的坑）

| 版本 | 语法 | 能否被 GNU as 编译 |
| --- | --- | --- |
| Keil/MDK 版 | `AREA`、`EQU`、`SPACE`、`EXPORT`、`DCD` | ❌ 报 `bad instruction` |
| **GCC 版**（推荐） | `.syntax unified`、`.section`、`.word`、`.global` | ✅ |

- 报错特征：`startup_*.s:260: Error: bad instruction 'xxx_irqhandler'` → 说明用的是 **Keil 版**。
- 获取 GCC 版：官方标准外设库 `Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/` 或 `TrueSTUDIO/` 目录下。
- 本项目已将 Keil 版备份为 `startup_stm32f10x_md_keil.s.bak`。

### 链接脚本必须提供的符号（GCC 启动文件依赖）

| 符号 | 含义 |
| --- | --- |
| `_estack` | 栈顶（RAM 末尾） |
| `_sidata` | .data 段在 Flash 的加载地址 |
| `_sdata` / `_edata` | .data 段在 RAM 的起止 |
| `_sbss` / `_ebss` | .bss 段起止 |
| `_Min_Heap_Size` / `_Min_Stack_Size` | 堆栈大小检查 |

CubeMX 生成的 `.ld` 都包含这些符号，可直接使用。

### 旧版 CMSIS 的坑：strexb 寄存器冲突

- 老版本 `core_cm3.c`（如 V1.30）中 `__STREXB/__STREXH/__STREXW` 内联汇编没有寄存器冲突约束，GCC 编译时可能生成 `strexb r0, r0, [r1]` 而报错。
- 修复：把输出约束从 `"=r"` 改为 `"=&r"`（early-clobber），并加 `"memory"`：

```c
__ASM volatile ("strexb %0, %2, [%1]" : "=&r" (result) : "r" (addr), "r" (value) : "memory" );
```

> 升级到新版 CMSIS（4.x/5.x）或使用 STM32Cube 系列即可彻底避免。

---

## 六、编译命令速查

```bash
# 1. 首次配置（Windows 指定 Ninja 生成器，避免默认选到 NMake/MSVC）
cmake -S . -B build -G Ninja

# 2. 编译（可反复执行增量编译）
cmake --build build

# 3. 指定构建类型
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 4. 清理后重新编译（改动很大时用）
cmake --build build --clean-first
# 或彻底重来
Remove-Item -Recurse build

# 5. 关闭/开启 ARM GCC 开关（toolchain.cmake 里的 option）
cmake -S . -B build -G Ninja -DUSE_ARMGCC=ON
```

| 常见报错 | 原因 |
| --- | --- |
| `Generator: execution of make failed... nmake` | 未指定生成器，改用 `-G Ninja` |
| `no such file or directory`（配置阶段） | 工具链路径不对 / 未加 `-G Ninja` |
| 新加 `.c` 文件没编进来 | `file(GLOB)` 需重新运行 cmake 配置 |

---

## 七、调试 / 烧录配置（VS Code + Cortex-Debug）

### 1. 安装扩展

- **Cortex-Debug**（必装）：支持 pyOCD / OpenOCD 调试服务器
- pyOCD（可选，CMSIS-DAP 调试器用）：`pip install pyocd`
- OpenOCD（可选，ST-Link/J-Link/DAP 通用）

### 2. `tasks.json`（构建任务，F5 前自动编译）

```json
{
    "version": "2.0.0",
    "tasks": [{
        "label": "build",
        "type": "shell",
        "command": "cmake",
        "args": ["--build", "build"],
        "options": { "cwd": "${workspaceRoot}" },
        "group": { "kind": "build", "isDefault": true },
        "problemMatcher": ["$gcc"]
    }]
}
```

### 3. `launch.json`（调试配置，F5 一键烧录+调试）

pyOCD 方案（CMSIS-DAP / DAP-Link）：

```json
{
    "version": "0.2.0",
    "configurations": [{
        "name": "DAP Debug (pyOCD)",
        "type": "cortex-debug",
        "request": "launch",
        "servertype": "pyocd",
        "serverpath": "你的pyocd.exe路径",
        "gdbPath": "工具链/bin/arm-none-eabi-gdb.exe",
        "cwd": "${workspaceRoot}",
        "executable": "${workspaceRoot}/build/output/TEST.elf",
        "targetId": "stm32f103c8",        // ★ 必须匹配芯片，不能写错型号
        "runToEntryPoint": "main",
        "preLaunchTask": "build"           // F5 前先构建
    }]
}
```

OpenOCD 方案（通用，ST-Link/J-Link 也可用）：

```json
{
    "name": "DAP Debug (OpenOCD)",
    "type": "cortex-debug",
    "request": "launch",
    "servertype": "openocd",
    "serverpath": "你的openocd.exe路径",
    "searchDir": ["你的openocd/scripts目录"],
    "configFiles": [
        "interface/cmsis-dap.cfg",        // 按调试器换：stlink.cfg / jlink.cfg
        "target/stm32f1x.cfg"             // 按芯片换：stm32f4x.cfg 等
    ],
    "gdbPath": "工具链/bin/arm-none-eabi-gdb.exe",
    "executable": "${workspaceRoot}/build/output/TEST.elf",
    "runToEntryPoint": "main",
    "preLaunchTask": "build"
}
```

### 4. 命令行烧录

```bash
# pyOCD
pyocd flash -t stm32f103c8 build/output/TEST.hex --reset
pyocd list                          # 查看调试器
pyocd erase -t stm32f103c8 --chip   # 整片擦除

# OpenOCD
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
        -c "program build/output/TEST.hex verify reset exit"
```

> ⚠️ `targetId` 与 `-t` 参数必须与实际芯片**精确匹配**（如 `stm32f103c8`）。型号写错（如把 c8 写成 rc）会导致 Flash 页大小不匹配、烧录失败。

### 5. 调试器接线（SWD）

| 调试器 | 目标 MCU |
| --- | --- |
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3V | 3.3V（目标板供电） |

---

## 八、移植到新工程的检查清单

换一个芯片/工程时，按这份清单核对，基本都能一次编译通过：

- [ ] **工具链**：`toolchain.cmake` 的 `COMPILE_ROOT_PATH` 指向本机实际路径
- [ ] **芯片宏**：`add_compile_definitions` 中的型号宏（`STM32F10X_HD` 等）与 `.h` 匹配
- [ ] **启动文件**：`.s` 用 GCC 版，且密度与芯片匹配（md/hd/ld）
- [ ] **链接脚本**：`.ld` 对应芯片的 Flash/RAM 大小，含 `_sidata/_sdata/_sbss` 等符号
- [ ] **CPU 内核**：`CPU_CORE`（cortex-m0/m3/m4...）
- [ ] **头文件路径**：`include_directories` 覆盖所有用到的头文件目录
- [ ] **源文件**：`file(GLOB)` 路径包含新增的 `.c`
- [ ] **语言**：`project(... C ASM)` 包含 `ASM`
- [ ] **生成器**：Windows 下用 `cmake -S . -B build -G Ninja`
- [ ] **调试目标**：launch.json 的 `targetId` / OpenOCD 的 `target/xxx.cfg` 与芯片一致

---

## 九、常见问题速查

| 错误现象 | 原因与解决 |
| --- | --- |
| `fatal error: stm32f10x.h: No such file` | include_directories 少了 `DeviceSupport/ST/STM32F10x` |
| `#error "Please select first the target..."` | 没定义芯片型号宏（`STM32F10X_MD`） |
| `implicit declaration of 'assert_param'` | 没加 `USE_STDPERIPH_DRIVER` 宏 |
| `Error: bad instruction 'xxx'`（.s 文件） | 启动文件是 Keil 版，换 GCC 版 |
| `registers may not be the same -- strexb r0,r0,[r1]` | 旧 CMSIS 汇编约束问题，用 `"=&r"` 修复或升级 CMSIS |
| `Unknown CMake command "GOLB"` | `file(GLOB ...)` 拼写错误 |
| `Generator ... nmake` | Windows 未指定生成器，用 `-G Ninja` |
| 链接报 `undefined reference to main` | 缺少 `main.c` |
| `LOAD segment with RWX permissions` | 链接警告，可忽略（不影响烧录） |
| 烧录 `No probe found` | 调试器未连接 / USB 线问题，`pyocd list` 检查 |
| 烧录 `Target not found` | SWD 接线错误或芯片未供电 |



