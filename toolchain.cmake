set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) #编译目标设置为静态

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

option(USE_ARMGCC "USE ARMGCC" ON)

if(USE_ARMGCC)
    #指定编译器目录
    set(COMPILE_ROOT_PATH E:/Project/Compiler/arm-gnu-toolchain-15.3.rel1-mingw-w64-i686-arm-none-eabi/bin)
    
    set(CMAKE_C_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_ASM_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_ARM_COMPILER ${COMPILE_ROOT_PATH}/arm-none-eabi-gcc.exe)
    set(CMAKE_OBJCOPY ${COMPILE_ROOT_PATH}/arm-none-eabi-objcopy.exe)
    set(CMAKE_SIZE ${COMPILE_ROOT_PATH}/arm-none-eabi-size.exe)
endif()






