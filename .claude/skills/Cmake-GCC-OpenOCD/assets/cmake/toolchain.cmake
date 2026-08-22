# =============================================================================
# toolchain.cmake — Arm GNU 交叉编译工具链模板
# 使用前把 COMPILE_ROOT_PATH 改成你本机的工具链安装目录
# =============================================================================

# 编译目标设为静态库，跳过配置阶段的链接测试（裸机工程必须）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 程序只在宿主机查找；库/头文件只在工具链根目录查找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 开关：启用 ARM GCC 工具链（cmake -DUSE_ARMGCC=OFF 可关闭）
option(USE_ARMGCC "USE ARMGCC" ON)

if(USE_ARMGCC)
    # ★★★ 按本机修改：GNU Arm 工具链 bin 目录 ★★★
    set(COMPILE_ROOT_PATH @COMPILE_ROOT_PATH@)

    set(CMAKE_C_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_ASM_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_ARM_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_OBJCOPY ${COMPILE_ROOT_PATH}/arm-none-eabi-objcopy.exe)
    set(CMAKE_SIZE ${COMPILE_ROOT_PATH}/arm-none-eabi-size.exe)
endif()
