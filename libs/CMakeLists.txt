cmake_minimum_required( VERSION 3.10.0 )

file(
    GLOB
    HAL_src
    "stm32f4xx_hal_driver/Src/*.c"
)

# Exclude **template.c files
foreach (TMP_PATH ${HAL_src})
    string (FIND ${TMP_PATH} "template.c" EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list (REMOVE_ITEM HAL_src ${TMP_PATH})
    endif ()
endforeach(TMP_PATH)

add_library(
    stm32f4xx_hal
    STATIC
    ${HAL_src}
    cmsis_device_f4/Source/Templates/gcc/startup_stm32f407xx.s
    cmsis_device_f4/Source/Templates/system_stm32f4xx.c
)

target_include_directories(
    stm32f4xx_hal
    PUBLIC
    include
    stm32f4xx_hal_driver/Inc
    cmsis_device_f4/Include
    cmsis_core/Core/Include
)

target_compile_definitions(
    stm32f4xx_hal
    PUBLIC
    STM32F407xx
)

target_link_options(
    stm32f4xx_hal
    PRIVATE
    -Wl,-whole-archive
)

add_library(freertos_config INTERFACE)

target_include_directories(
    freertos_config
    SYSTEM
    INTERFACE
    include
)

target_compile_definitions(
    freertos_config
    INTERFACE
    projCOVERAGE_TEST=0
)

set( FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set( FREERTOS_PORT "GCC_ARM_CM4F" CACHE STRING "" FORCE)

add_subdirectory(
    FreeRTOS-Kernel
)
