# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/waltor/stm_ecu

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/waltor/stm_ecu/build

# Include any dependencies generated for this target.
include _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/depend.make

# Include the progress variables for this target.
include _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/progress.make

# Include the compile flags for this target's objects.
include _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/flags.make

_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.o: _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/flags.make
_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.o: _deps/freertos_plus_tcp-src/test/build-combination/Common/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.o"
	cd /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.o   -c /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-src/test/build-combination/Common/main.c

_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.i"
	cd /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-src/test/build-combination/Common/main.c > CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.i

_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.s"
	cd /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-src/test/build-combination/Common/main.c -o CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.s

# Object files for target freertos_plus_tcp_build_test
freertos_plus_tcp_build_test_OBJECTS = \
"CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.o"

# External object files for target freertos_plus_tcp_build_test
freertos_plus_tcp_build_test_EXTERNAL_OBJECTS =

_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/Common/main.c.o
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/build.make
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/libfreertos_plus_tcp.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_kernel-build/libfreertos_kernel.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/portable/libfreertos_plus_tcp_port.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/portable/NetworkInterface/STM32Fxx/libfreertos_plus_tcp_network_if.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/tools/libfreertos_plus_tcp_utilities.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/portable/NetworkInterface/libfreertos_plus_tcp_network_if_common.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/libfreertos_plus_tcp.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/portable/libfreertos_plus_tcp_port.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/portable/NetworkInterface/STM32Fxx/libfreertos_plus_tcp_network_if.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/tools/libfreertos_plus_tcp_utilities.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/source/portable/NetworkInterface/libfreertos_plus_tcp_network_if_common.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_kernel-build/libfreertos_kernel.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_kernel-build/portable/libfreertos_kernel_port.a
_deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test: _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable freertos_plus_tcp_build_test"
	cd /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/freertos_plus_tcp_build_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/build: _deps/freertos_plus_tcp-build/test/build-combination/freertos_plus_tcp_build_test

.PHONY : _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/build

_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/clean:
	cd /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination && $(CMAKE_COMMAND) -P CMakeFiles/freertos_plus_tcp_build_test.dir/cmake_clean.cmake
.PHONY : _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/clean

_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/depend:
	cd /home/waltor/stm_ecu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/waltor/stm_ecu /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-src/test/build-combination /home/waltor/stm_ecu/build /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination /home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : _deps/freertos_plus_tcp-build/test/build-combination/CMakeFiles/freertos_plus_tcp_build_test.dir/depend

