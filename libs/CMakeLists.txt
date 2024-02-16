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

include(FetchContent)

#FetchContent_Declare(
#    freertos_kernel
#    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
#    GIT_TAG        main #Note: Best practice to use specific git-hash or tagged version
#)

add_library(freertos_config INTERFACE)

target_include_directories(
    freertos_config
    SYSTEM
    INTERFACE
    include
    SystemView/Sample/FreeRTOSV10.4
    SystemView/SEGGER
    SystemView/Config
)

target_compile_definitions(
    freertos_config
    INTERFACE
    projCOVERAGE_TEST=0
)

set( FREERTOS_HEAP ${CMAKE_CURRENT_SOURCE_DIR}/sys_heap.c CACHE STRING "" FORCE)
set( FREERTOS_PORT "GCC_ARM_CM4F" CACHE STRING "" FORCE)

#FetchContent_MakeAvailable(
#    freertos_kernel
#)

#add_subdirectory(
#    FreeRTOS-Kernel
#)

FetchContent_Declare(
    freertos_plus_tcp
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git
    GIT_TAG        main #Note: Best practice to use specific git-hash or tagged version
    GIT_SUBMODULES "" # Don't grab any submodules since not latest
)

set( FREERTOS_PLUS_TCP_NETWORK_IF "STM32FXX" CACHE STRING "" FORCE )



add_subdirectory(
    FreeRTOS-Plus-TCP
)

#FetchContent_MakeAvailable(
#    freertos_plus_tcp
#)

#
# Removes the specified compile flag from the specified target.
#   _target     - The target to remove the compile flag from
#   _flag       - The compile flag to remove
#
# Pre: apply_global_cxx_flags_to_all_targets() must be invoked.
#
macro(remove_flag_from_target _target _flag)
    get_target_property(_target_cxx_flags ${_target} COMPILE_OPTIONS)
    if(_target_cxx_flags)
        list(REMOVE_ITEM _target_cxx_flags ${_flag})
        set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${_target_cxx_flags}")
    endif()
endmacro()

target_compile_definitions(
    freertos_plus_tcp_network_if
    PRIVATE
    STM32F407xx
    STM32F4xx
)

target_include_directories(
    freertos_plus_tcp_network_if
    PUBLIC
    cmsis_device_f4/Include
    stm32f4xx_hal_driver/Inc
    cmsis_core/Include
)

add_library(
    segger
    STATIC
    SystemView/SEGGER/SEGGER_RTT.c
    SystemView/SEGGER/SEGGER_SYSVIEW.c
    SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c
    SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c
    #SystemView/SEGGER/SEGGER_RTT_ASM_ARMv7M.S
)

target_include_directories(
    segger
    PUBLIC
    SystemView/SEGGER
    SystemView/Config
    SystemView/Sample/FreeRTOSV10.4
)

target_link_libraries(
    segger
    LINK_PUBLIC
    freertos_kernel
)
