cmake_minimum_required(VERSION 3.28.1)

cmake_policy(SET CMP0123 NEW)

set(CMAKE_SYSTEM_NAME Generic)
set(CPU_CORE cortex-m3)
set(CMAKE_SYSTEM_PROCESSOR ${CPU_CORE})

#1、指定工具链路径
#链接工具链路径
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)

#设置支持语言为汇编
project(TEST C ASM)

#设置一个字符串用来判断默认是release还是debug
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
if(BUILD_TYPE_UPPER STREQUAL "RELEASE")
    add_compile_options(-Os)
else()
    add_compile_options(-O0 -g)
endif()

#定义目标芯片型号（STM32F103C8T6 为 Medium-density）并启用标准外设库
add_compile_definitions(STM32F10X_MD USE_STDPERIPH_DRIVER)

#导出编译命令（compile_commands.json），让 VS Code C/C++ 扩展的 IntelliSense
#能拿到芯片型号宏/外设库宏和头文件路径，消除误报的“未定义标识符”红色波浪线
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#2、指定头文件路径和源文件路径
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x
    ${CMAKE_CURRENT_LIST_DIR}/User
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/inc
)
file(GLOB SOURCE_FILE
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/User/*.c
)
#指定生成文件类型
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output)
set(HEX_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.hex)
set(BIN_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.bin)
set(MAP_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.map)


#3、指定启动文件路径
if(USE_ARMGCC)
    SET(STARTUP_FILE ${CMAKE_CURRENT_LIST_DIR}/Startup/startup_stm32f10x_md.s)
    SET(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/Startup/STM32F103C8Tx_FLASH.ld)

    add_compile_options(
        -mcpu=${CPU_CORE}
        -mthumb
        -ffunction-sections
        -fdata-sections
        -fno-common
        -fmessage-length=0
    )

    #设置链接选项
    add_link_options(
        -Wl,-Map=${MAP_FILE}
        -Wl,--print-memory-usage
        -Wl,--gc-sections
        -T ${LINKER_SCRIPT}
    )
endif()

#最终生成文件
add_executable(${PROJECT_NAME} ${SOURCE_FILE} ${STARTUP_FILE})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    OUTPUT_NAME ${PROJECT_NAME}.elf
)

#添加自定义命令以生成bin、hex、map文件
if(USE_ARMGCC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${BIN_FILE}
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    )
endif()

# =============================================================================
# 4、生成 VS Code 调试配置（.vscode/launch.json + tasks.json）
# -----------------------------------------------------------------------------
# cortex-debug + OpenOCD 配置规则（重要）：
#   * *.cfg 一律通过 configFiles 数组传入 —— cortex-debug 会自动为每个文件
#     生成一个 -f 参数（configFiles: [...] ≡ openocd -f ... -f ...）
#   * 禁止在 serverArgs 里传 -f：cortex-debug 不保证 serverArgs 与内置参数
#     的拼接顺序，属不稳定用法；serverArgs 只用于 -c 等非 -f 附加参数
#   * searchDir 必须显式指定：xPack OpenOCD 脚本位于 openocd/scripts
#     （非标准 share/openocd/scripts），cortex-debug 无法自动推断
# =============================================================================
option(VSCODE_REGENERATE_DEBUG_CONFIG "每次 configure 重新生成 .vscode/launch.json 与 tasks.json" ON)

if(VSCODE_REGENERATE_DEBUG_CONFIG)
    # 本机调试工具路径 —— 修改后重新执行 cmake 配置即可自动更新 launch.json
    set(OPENOCD_PATH
        "E:/Project/Compiler/xpack-openocd-0.12.0-7/bin/openocd.exe"
        CACHE STRING "OpenOCD 可执行文件路径")
    set(OPENOCD_SCRIPTS_DIR
        "E:/Project/Compiler/xpack-openocd-0.12.0-7/openocd/scripts"
        CACHE STRING "OpenOCD scripts 目录（xPack 布局为 openocd/scripts，非 share/openocd/scripts）")
    set(GDB_PATH
        "E:/Project/Compiler/arm-gnu-toolchain-15.3.rel1-mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-gdb.exe"
        CACHE STRING "arm-none-eabi-gdb 可执行文件路径")
    set(PROBE_INTERFACE_CFG "interface/cmsis-dap.cfg" CACHE STRING "调试器接口配置（stlink.cfg / jlink.cfg / cmsis-dap.cfg）")
    set(TARGET_CFG "target/stm32f1x.cfg" CACHE STRING "目标芯片 OpenOCD 配置")
    set(CMAKE_PRESET_CONFIG_NAME "ninja" CACHE STRING "configure preset 名称（见 CMakePresets.json）")
    set(CMAKE_PRESET_BUILD_NAME "debug" CACHE STRING "build preset 名称（见 CMakePresets.json）")

    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/launch.json.in
        ${CMAKE_CURRENT_LIST_DIR}/.vscode/launch.json
        @ONLY)   # @ONLY：仅替换 @VAR@，保留 ${workspaceRoot} 等 VS Code 变量
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/tasks.json.in
        ${CMAKE_CURRENT_LIST_DIR}/.vscode/tasks.json
        @ONLY)
    message(STATUS "已生成 .vscode/launch.json / tasks.json（OpenOCD 使用 configFiles + searchDir，未用 serverArgs 传 -f）")
endif()