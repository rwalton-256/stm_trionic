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

FetchContent_Declare(
    freertos_plus_tcp
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git
    GIT_TAG        "V4.0.0"
    GIT_SUBMODULES ""
    PATCH_COMMAND  git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/FreeRTOS-Plus-TCP.patch
    UPDATE_DISCONNECTED 1
)

set( FREERTOS_PLUS_TCP_NETWORK_IF "STM32FXX" CACHE STRING "" FORCE )

FetchContent_MakeAvailable(
    freertos_plus_tcp
)

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

# ---- FatFs (ChaN R0.15) for the microSD card ---------------------------------------------
FetchContent_Declare(
    fatfs
    URL http://elm-chan.org/fsw/ff/arc/ff15.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable( fatfs )          # plain source archive: just downloads and unpacks

set( FATFS_DIR ${fatfs_SOURCE_DIR}/source )

# Adjust the stock ffconf.h in place: long file names, US code page (the default 932 is Japanese).
file( READ ${FATFS_DIR}/ffconf.h FFCONF )
string( REGEX REPLACE "#define[ \t]+FF_USE_LFN[ \t]+[0-9]+"   "#define FF_USE_LFN\t\t1"   FFCONF "${FFCONF}" )
string( REGEX REPLACE "#define[ \t]+FF_CODE_PAGE[ \t]+[0-9]+" "#define FF_CODE_PAGE\t437" FFCONF "${FFCONF}" )
file( WRITE ${FATFS_DIR}/ffconf.h "${FFCONF}" )

add_library(
    fatfs
    STATIC
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffunicode.c
)

target_include_directories(
    fatfs
    PUBLIC
    ${FATFS_DIR}
)