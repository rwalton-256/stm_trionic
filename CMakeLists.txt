cmake_minimum_required( VERSION 3.10.0 )

project( lesser_nighthawk C CXX ASM )

set( CMAKE_C_COMPILER "arm-none-eabi-gcc" )
set( CMAKE_CXX_COMPILER "arm-none-eabi-g++" )
set( CMAKE_ASM_COMPILER "arm-none-eabi-gcc" )

set(
    CMAKE_C_FLAGS
    "-g -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mno-thumb-interwork -specs=nano.specs -specs=nosys.specs"
)

set(
    CMAKE_CXX_FLAGS
    "-g -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mno-thumb-interwork -specs=nano.specs -specs=nosys.specs"
)

add_subdirectory( libs )

file(
    GLOB
    c_src
    src/*.c
)

file(
    GLOB
    cpp_src
    src/*.cpp
)

add_executable(
    stm_ecu
    ${c_src}
    ${cpp_src}
)

target_link_options(
    stm_ecu
    PRIVATE
    -T ${CMAKE_SOURCE_DIR}/src/memory.ld
    -u _printf_float
    -Xlinker -Map=${CMAKE_SOURCE_DIR}/foo.map
)

target_link_libraries(
    stm_ecu
    PUBLIC
    stm32f4xx_hal
    freertos_kernel
)

target_include_directories(
    stm_ecu
    PRIVATE
    include
)
