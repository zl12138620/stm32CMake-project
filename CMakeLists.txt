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
    add_compile_options(-Og -g)
endif()

#定义目标芯片型号（STM32F103C8T6 为 Medium-density）并启用标准外设库
add_compile_definitions(STM32F10X_MD USE_STDPERIPH_DRIVER)

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