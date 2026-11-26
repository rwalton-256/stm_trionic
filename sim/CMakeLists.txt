# Host simulation of the stm_trionic firmware. Separate from the ARM firmware build.
#
#   make sim                 (repo root) configure, build, run all tests
#   cmake -S sim -B build-sim -G Ninja && cmake --build build-sim && ctest --test-dir build-sim
#
# Third-party sources come from sim/deps.cmake (FetchContent, or offline copies).
cmake_minimum_required(VERSION 3.16)
project(stm_trionic_sim C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
include(deps.cmake)

set(FW ${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${mdspan_SOURCE_DIR}/include)

# ---- FreeRTOS kernel: POSIX port with the tick driven by the simulated clock ----
set(K ${freertos_kernel_SOURCE_DIR})
add_library(freertos_sim STATIC
    ${K}/tasks.c ${K}/queue.c ${K}/list.c ${K}/stream_buffer.c ${K}/event_groups.c
    ${K}/portable/MemMang/heap_3.c
    rtos/port_lockstep.c rtos/lockstep.cpp
    ${K}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c)
target_include_directories(freertos_sim PUBLIC rtos ${K}/include
    ${K}/portable/ThirdParty/GCC/Posix ${K}/portable/ThirdParty/GCC/Posix/utils)
target_link_libraries(freertos_sim PUBLIC Threads::Threads)
set_target_properties(freertos_sim PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- the Python module: firmware tasks + timer model + board, one simulated clock ----
pybind11_add_module(ecusim py/ecusim.cpp
    ${FW}/src/crank.cpp ${FW}/src/outputs.cpp ${FW}/src/fuel.cpp ${FW}/src/fuel_task.cpp
    ${FW}/src/power.cpp ${FW}/src/ts_link.cpp ${FW}/src/settings.cpp
    model/periph.cpp model/board.cpp net/sockets_shim.cpp)
target_include_directories(ecusim PRIVATE net fake model ${FW}/include)
target_link_libraries(ecusim PRIVATE freertos_sim)
target_compile_definitions(ecusim PRIVATE STM_TRIONIC_SIM)

# ---- tests: pytest, one file = one ECU boot ----
enable_testing()
file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.py)
foreach(f ${TEST_FILES})
    get_filename_component(name ${f} NAME_WE)
    add_test(NAME ${name} COMMAND ${Python3_EXECUTABLE} -m pytest -q ${f})
    set_tests_properties(${name} PROPERTIES TIMEOUT 600 ENVIRONMENT
        "PYTHONPATH=$<TARGET_FILE_DIR:ecusim>:${CMAKE_CURRENT_SOURCE_DIR}/engine")
endforeach()

# test_simloop measures real-time pacing, so it must not share the CPU with other sims under -j.
foreach(serial test_simloop test_autodrive test_status)
    if(TEST_FILES MATCHES "${serial}")
        set_tests_properties(${serial} PROPERTIES RUN_SERIAL TRUE)
    endif()
endforeach()
