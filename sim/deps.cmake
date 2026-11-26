# Every third-party dependency of the host simulation, in one place.
# Pinned versions; CMake downloads them at configure time (FetchContent).
#
# Offline (e.g. a sandbox without internet): point each one at an unpacked copy instead,
#   -DFETCHCONTENT_SOURCE_DIR_FREERTOS_KERNEL=/path/to/FreeRTOS-Kernel
#   -DFETCHCONTENT_SOURCE_DIR_MDSPAN=/path/to/mdspan
#   -DFETCHCONTENT_SOURCE_DIR_PYBIND11=/path/to/pybind11
# `make sim-deps` (repo root) downloads all of them into sim-deps.zip for exactly that.
include(FetchContent)

FetchContent_Declare(freertos_kernel                      # scheduler + POSIX port
    URL https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V11.3.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_Declare(mdspan                               # Kokkos reference std::mdspan (C++17)
    URL https://github.com/kokkos/mdspan/archive/refs/tags/mdspan-0.6.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_Declare(pybind11                             # Python bindings
    URL https://github.com/pybind/pybind11/archive/refs/tags/v3.1.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# The kernel and mdspan are used as plain source trees (we build the kernel ourselves with the
# sim's FreeRTOSConfig.h); pybind11 is added as a CMake project.
FetchContent_GetProperties(freertos_kernel)
if(NOT freertos_kernel_POPULATED)
    FetchContent_Populate(freertos_kernel)
endif()
FetchContent_GetProperties(mdspan)
if(NOT mdspan_POPULATED)
    FetchContent_Populate(mdspan)
endif()
FetchContent_MakeAvailable(pybind11)
