cmake_minimum_required(VERSION 3.28.1)

cmake_policy(SET CMP0123 NEW)

set(CMAKE_SYSTEM_NAME Generic)
set(CPU_CORE cortex-m3)
set(CMAKE_SYSTEM_PROCESSOR ${CPU_CORE})

#链接工具链路径
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)

#设置支持语言为汇编
project(TEST C ARM)

#设置一个字符串用来判断默认是release还是debug
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
if(${BUILD_TYPE_UPPER} RELEASE)
    add_compile_options(-Os)
else()
    add_compile_options(-Og -g)
elseif()


include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport
    ${CMAKE_CURRENT_LIST_DIR}/User
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/inc
)

file(GOLB SOURCE_FILE
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/STM32F10x_StdPeriph_Driver/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/CoreSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Libraries/CMSIS/CM3/DeviceSupport/*.c
    ${CMAKE_CURRENT_LIST_DIR}/User/*.c
)

set(EXECUTABLE_OUTPUT_PATH &{CMAKE_BINARY_DIR}/ouput)
set(HEX_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.hex)
set(BIN_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.bin)
set(MAP_FILE ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.map)


if(USE_ARMGCC)
    SET(STARTUP_FILE ${CMAKE_CURRENT_LIST_DIR}/startup_stm32f10x_md.s)