# CMake + GCC + OpenOCD 构建调试指南（详细版）

> 面向 STM32 裸机工程的通用编写流程，基于工程 `CMakeLists.txt` 与 `toolchain.cmake` 总结。

## 一、构建系统组成

| 文件 | 作用 | 关注点 |
| --- | --- | --- |
| `CMakeLists.txt` | 构建脚本：目标、源文件、编译/链接选项 | 必写 |
| `toolchain.cmake` | 交叉编译工具链：ARM GCC 路径 | 必写 |
| `*.ld` | 链接脚本：内存布局 | 随芯片选择 |
| `startup_*.s` | 启动文件：向量表、复位、data/bss 初始化 | **必须 GCC 版** |
| `.vscode/launch.json` | VS Code 调试配置 | 推荐 |
| `.vscode/tasks.json` | VS Code 构建任务 | 推荐 |

构建流程：`cmake 配置` → 生成构建文件 → `arm-none-eabi-gcc` 编译 → `ld` 链接 → POST_BUILD(objcopy/size) 生成 `.hex/.bin/.map`。

## 二、toolchain.cmake 编写

```cmake
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)  # 跳过链接测试（裸机必须）
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

option(USE_ARMGCC "USE ARMGCC" ON)

if(USE_ARMGCC)
    set(COMPILE_ROOT_PATH E:/Project/Compiler/arm-gnu-toolchain-15.3.rel1-mingw-w64-i686-arm-none-eabi/bin)  # ★改这里

    set(CMAKE_C_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_ASM_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)   # .s 启动文件用它编译
    set(CMAKE_OBJCOPY ${COMPILE_ROOT_PATH}/arm-none-eabi-objcopy.exe)
    set(CMAKE_SIZE ${COMPILE_ROOT_PATH}/arm-none-eabi-size.exe)
endif()
```

要点：
- `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`：无 C 库的裸机工程必须，否则配置阶段报链接错误。
- `CMAKE_ASM_COMPILER` 必须设置，否则 `.s` 无法编译。
- 换工具链版本只需改 `COMPILE_ROOT_PATH`。

## 三、CMakeLists.txt 编写（14 步）

### 1. 版本与策略
```cmake
cmake_minimum_required(VERSION 3.28.1)
cmake_policy(SET CMP0123 NEW)   # ARMClang 的 -mcpu 由项目显式指定
```

### 2. 目标平台
```cmake
set(CMAKE_SYSTEM_NAME Generic)      # 交叉编译固定写法
set(CPU_CORE cortex-m3)             # cortex-m0/m3/m4/m7
set(CMAKE_SYSTEM_PROCESSOR ${CPU_CORE})
```

### 3. 指定工具链
```cmake
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)
```

### 4. project 声明语言
```cmake
project(TEST C ASM)   # 必须含 ASM，否则 .s 不编译
```

### 5. 构建类型（Debug/Release）
```cmake
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
if(BUILD_TYPE_UPPER STREQUAL "RELEASE")
    add_compile_options(-Os)
else()
    add_compile_options(-O0 -g)
endif()
```
注意：正确写法是 `if(BUILD_TYPE_UPPER STREQUAL "RELEASE")`；`elseif()` 空语句是错误写法。

### 6. 芯片宏定义（先询问芯片型号，再写宏）

```cmake
# 写在 add_executable 之后，用目标作用域而非全局 add_compile_definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32F10X_MD USE_STDPERIPH_DRIVER)
```

- `STM32F10X_MD`：芯片密度宏（LD/MD/HD/XL），**必须与 stm32f10x.h 匹配**，否则报 `#error "Please select first the target..."`。
- `USE_STDPERIPH_DRIVER`：启用外设库，让 `stm32f10x.h` 自动包含 `stm32f10x_conf.h`（否则 `assert_param` 未定义）。
- **作用域最佳实践**：用 `target_compile_definitions(... PRIVATE ...)` 只作用于本目标；全局 `add_compile_definitions()` 会污染所有目标，多目标工程易踩坑。

**自动选择宏的流程（必须执行）：**

1. **先询问用户芯片完整型号**，例如 `STM32F103C8T6`。不要凭猜测或默认值写入宏。
2. 把型号换算成密度宏并**向用户复述确认**，再写入 `target_compile_definitions`。

型号解读示例（`STM32F103C8T6`）：`103`=主流 F1 系列；`C`=48 引脚；`8`=Flash 64KB（密度关键字母）；`T`=LQFP 封装。密度字母对照：`4`=16KB、`6`=32KB、`8`=64KB、`B`=128KB、`C`=256KB、`D`=384KB、`E`=512KB、`F`=768KB、`G`=1024KB。

**STM32F10x 常见型号速查表：**

| 芯片型号（示例） | 密度 / 产品线 | 设备宏 |
| --- | --- | --- |
| STM32F101/102/103x4、x6 | 低密度（16–32KB） | `STM32F10X_LD` |
| STM32F101/102/103x8、xB（C8T6、CBT6、R8T6、T8U6…） | 中等密度（64–128KB） | `STM32F10X_MD` |
| STM32F101/103xC、xD、xE（RCT6、RET6、VET6、ZET6…） | 高密度（256–512KB） | `STM32F10X_HD` |
| STM32F101/103xF、xG | XL 超高密度（512–1024KB） | `STM32F10X_XL` |
| STM32F105xx、STM32F107xx（R8T6、RBT6、VCT6…） | 互联型 | `STM32F10X_CL` |
| STM32F100x4、x6 | 值线低密度（16–32KB） | `STM32F10X_LD_VL` |
| STM32F100x8、xB（C8T6、RBT6…） | 值线中等密度（64–128KB） | `STM32F10X_MD_VL` |
| STM32F100xC、xD、xE | 值线高密度（256–512KB） | `STM32F10X_HD_VL` |

> 非 F1 系列（F0/F3/F4/F7/H7/L1/L4 等）：询问用户工程使用的 CMSIS 头文件期望的宏（如 `STM32F407xx`、`STM32F0XX`、`USE_HAL_DRIVER`），不要自行编造。

### 7. 头文件路径（写在 add_executable 之后）
```cmake
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x   # stm32f10x.h 所在目录
    ${CMAKE_CURRENT_LIST_DIR}/User
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/inc
)
```
- 漏掉任何一层都会 `fatal error: xxx.h: No such file or directory`。
- **作用域最佳实践**：`target_include_directories(... PRIVATE ...)` 只作用于本目标；`include_directories()` 是目录级全局指令，会让目录下所有目标（含子目录）都继承这些路径，作用域不清晰。

### 8. 收集源文件
```cmake
file(GLOB SOURCE_FILE CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/User/*.c
)
```
- 命令是 `file(GLOB ...)`，不是 `GOLB`。
- **加 `CONFIGURE_DEPENDS`**：Ninja 每次构建会重新检查 glob 目录，新增/删除 `.c` 后自动感知（无需手动重新配置），CMake ≥ 3.12。

### 9. 输出路径与文件名
```cmake
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output)
set(HEX_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.hex)
set(BIN_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.bin)
set(MAP_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.map)
```
⚠️ 变量引用必须写 `${变量}`，写成 `&{变量}` 会被当字符串。

### 10. 启动文件与链接脚本
```cmake
if(USE_ARMGCC)
    set(STARTUP_FILE ${CMAKE_CURRENT_LIST_DIR}/Startup/startup_stm32f10x_md.s)
    set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/Startup/STM32F103C8Tx_FLASH.ld)
```

### 11. 编译选项
```cmake
    add_compile_options(
        -mcpu=${CPU_CORE}
        -mthumb
        -ffunction-sections
        -fdata-sections
        -fno-common
        -fmessage-length=0
    )
    add_compile_options(-Wall -Wextra)   # 所有构建类型都开警告，提前暴露隐患
```
- `-ffunction-sections/-fdata-sections` 配合链接 `-Wl,--gc-sections` 剔除未用代码。
- `-Wall -Wextra` 能提前暴露"空循环被优化"、未用变量等问题（例如非 `volatile` 的延时循环在 `-Os` 下会被 GCC 整个删掉）。

### 12. 链接选项
```cmake
    add_link_options(
        -Wl,-Map=${MAP_FILE}
        -Wl,--print-memory-usage
        -Wl,--gc-sections
        -T ${LINKER_SCRIPT}
    )
endif()
```
`--print-memory-usage` 会在链接后打印 Flash/RAM 占用。

### 13. 生成目标（target_* 指令必须放在这里之后）
```cmake
add_executable(${PROJECT_NAME} ${SOURCE_FILE} ${STARTUP_FILE})

# 芯片宏 + 头文件路径写在 add_executable 之后（见第 6/7 步）
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32F10X_MD USE_STDPERIPH_DRIVER)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x
    ${CMAKE_CURRENT_LIST_DIR}/User
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/inc
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    OUTPUT_NAME ${PROJECT_NAME}.elf
)
```
- `target_*` 系列指令必须在 `add_executable` 之后调用，否则 CMake 报 "target not created" 错误。

### 14. POST_BUILD 生成 hex/bin
```cmake
if(USE_ARMGCC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${BIN_FILE}
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    )
endif()
```
`$<TARGET_FILE:...>` 生成器表达式，注意闭合尖括号 `>`。`POST_BUILD` 是 `add_custom_command` 的选项，不能用于 `add_custom_target`。

## 四、启动文件与链接脚本注意事项

### 启动文件（常见坑）

| 版本 | 语法 | GNU as 能否编译 |
| --- | --- | --- |
| Keil/MDK 版 | `AREA`、`EQU`、`SPACE`、`EXPORT`、`DCD` | ❌ `bad instruction` |
| **GCC 版** | `.syntax unified`、`.section`、`.word`、`.global` | ✅ |

报错 `startup_*.s: Error: bad instruction` → 启动文件是 Keil 版。GCC 版可在标准外设库 `startup/gcc_ride7/` 或 `TrueSTUDIO/` 目录获取。

### 链接脚本必需符号

`_estack`、`_sidata`、`_sdata/_edata`、`_sbss/_ebss`、`_Min_Heap_Size`、`_Min_Stack_Size`（CubeMX 生成的 `.ld` 均包含）。

### 旧版 CMSIS 的坑：strexb 寄存器冲突

老版 `core_cm3.c` 的 `__STREXB/__STREXH/__STREXW` 可能生成 `strexb r0, r0, [r1]` 报错。修复：

```c
__ASM volatile ("strexb %0, %2, [%1]" : "=&r" (result) : "r" (addr), "r" (value) : "memory" );
```

或升级新版 CMSIS。

## 五、编译命令速查

```bash
cmake -S . -B build -G Ninja                       # 首次配置（Windows 显式指定生成器）
cmake --build build                                 # 增量编译
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # Release
cmake --build build --clean-first                   # 清理重建
python check_build_env.py                           # 环境诊断
```

| 报错 | 解决 |
| --- | --- |
| `Generator ... nmake` | 未指定生成器，改用 `-G Ninja` |
| 新加 `.c` 没编进来 | 重新运行 cmake 配置 |

## 六、调试 / 烧录配置

### 1. 安装扩展
- **Cortex-Debug**（必装）
- pyOCD：`pip install pyocd`；OpenOCD：xPack 版本

### 2. tasks.json（构建任务）
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

### 3. launch.json（pyOCD 方案）
```json
{
    "version": "0.2.0",
    "configurations": [{
        "name": "DAP Debug (pyOCD)",
        "type": "cortex-debug",
        "request": "launch",
        "servertype": "pyocd",
        "serverpath": "C:/.../Scripts/pyocd.exe",
        "gdbPath": "E:/.../arm-none-eabi-gdb.exe",
        "cwd": "${workspaceRoot}",
        "executable": "${workspaceRoot}/build/output/TEST.elf",
        "targetId": "stm32f103c8",
        "runToEntryPoint": "main",
        "preLaunchTask": "build"
    }]
}
```

### 4. launch.json（OpenOCD 方案）
```json
{
    "name": "DAP Debug (OpenOCD)",
    "type": "cortex-debug",
    "request": "launch",
    "servertype": "openocd",
    "serverpath": "E:/.../openocd.exe",
    "searchDir": ["E:/.../openocd/scripts"],
    "configFiles": ["interface/cmsis-dap.cfg", "target/stm32f1x.cfg"],
    "gdbPath": "E:/.../arm-none-eabi-gdb.exe",
    "executable": "${workspaceRoot}/build/output/TEST.elf",
    "runToEntryPoint": "main",
    "preLaunchTask": "build"
}
```

⚠️ **OpenOCD 配置文件的正确传参方式（重要）：**
- ✅ 用 `configFiles` 数组传 `*.cfg`，cortex-debug 会为每个文件**自动生成一个 `-f` 参数**，等价于命令行 `openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg`。
- ❌ **不要**把 `-f` 写进 `serverArgs`（如 `"serverArgs": ["-f", "interface/cmsis-dap.cfg", ...]`）。cortex-debug 不保证 `serverArgs` 中参数与它自动拼接的 `-f`/`-s` 参数之间的顺序，这是**不稳定用法**，可能导致配置加载失败或行为异常。
- `serverArgs` 只适合传非 `-f` 的附加选项，例如 `"-c", "adapter speed 1000"`。
- `searchDir` 必须**显式指定**：xPack 版 OpenOCD 是特殊目录布局，脚本位于 `openocd/scripts`，不是标准安装的 `share/openocd/scripts`，cortex-debug **无法自动推断**搜索路径。不设 `searchDir` 会报 `Can't find interface/xxx.cfg`。
- 若旧配置把 `-f` 写在了 `serverArgs`，请迁移到 `configFiles`。

### 5. 命令行烧录
```bash
pyocd flash -t stm32f103c8 build/output/TEST.hex --reset
pyocd list                          # 查看调试器
pyocd erase -t stm32f103c8 --chip   # 整片擦除
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
        -c "program build/output/TEST.hex verify reset exit"
```
⚠️ `targetId`/`-t` 必须与芯片精确匹配（`stm32f103c8`，不能写成 `rc`）。

### 6. SWD 接线
| 调试器 | MCU |
| --- | --- |
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3V | 3.3V |

## 七、移植到新工程检查清单

- [ ] `toolchain.cmake` 的 `COMPILE_ROOT_PATH` 指向本机路径
- [ ] 芯片宏（`STM32F10X_HD` 等）与 `.h` 匹配
- [ ] 启动文件是 GCC 版且密度匹配
- [ ] 链接脚本 Flash/RAM 大小正确，含 `_sidata/_sdata/_sbss` 等符号
- [ ] `CPU_CORE` 正确（cortex-m0/m3/m4...）
- [ ] `include_directories` 覆盖所有头文件目录
- [ ] `file(GLOB)` 包含新增 `.c`
- [ ] `project(... C ASM)` 含 `ASM`
- [ ] Windows 用 `cmake -S . -B build -G Ninja`
- [ ] launch.json `targetId` / `target/xxx.cfg` 与芯片一致

## 八、常见问题速查

| 现象 | 原因与解决 |
| --- | --- |
| `fatal error: stm32f10x.h: No such file` | 少了 `DeviceSupport/ST/STM32F10x` 头文件路径 |
| `#error "Please select first the target..."` | 未定义芯片型号宏 |
| `assert_param` 未定义 | 未加 `USE_STDPERIPH_DRIVER` |
| `bad instruction`（.s 文件） | 启动文件是 Keil 版 |
| `strexb r0,r0,[r1]` | 旧 CMSIS 汇编约束问题 |
| `Unknown CMake command "GOLB"` | `file(GLOB ...)` 拼写错误 |
| `undefined reference to main` | 缺少 `main.c` |
| `LOAD segment with RWX permissions` | 链接警告，可忽略 |
| `No probe found` | 调试器未连接，`pyocd list` 检查 |
| `Target not found` | SWD 接线错误或芯片未供电 |
| `Can't find interface/cmsis-dap.cfg` | OpenOCD 脚本路径未找到：用 `configFiles` 传 `*.cfg`，并显式设置 `searchDir` 指向 scripts（xPack 为 `openocd/scripts`）；不要在 `serverArgs` 里传 `-f` |

