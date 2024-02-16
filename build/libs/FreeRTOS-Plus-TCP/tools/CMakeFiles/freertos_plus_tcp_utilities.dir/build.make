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
include libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/depend.make

# Include the progress variables for this target.
include libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/progress.make

# Include the compile flags for this target's objects.
include libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/flags.make

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.o: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/flags.make
libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.o: ../libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_dump_packets.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.o"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.o   -c /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_dump_packets.c

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.i"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_dump_packets.c > CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.i

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.s"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_dump_packets.c -o CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.s

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.o: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/flags.make
libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.o: ../libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_mem_stats.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.o"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.o   -c /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_mem_stats.c

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.i"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_mem_stats.c > CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.i

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.s"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_mem_stats.c -o CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.s

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.o: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/flags.make
libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.o: ../libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_netstat.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.o"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.o   -c /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_netstat.c

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.i"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_netstat.c > CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.i

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.s"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools/tcp_utilities/tcp_netstat.c -o CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.s

# Object files for target freertos_plus_tcp_utilities
freertos_plus_tcp_utilities_OBJECTS = \
"CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.o" \
"CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.o" \
"CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.o"

# External object files for target freertos_plus_tcp_utilities
freertos_plus_tcp_utilities_EXTERNAL_OBJECTS =

libs/FreeRTOS-Plus-TCP/tools/libfreertos_plus_tcp_utilities.a: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_dump_packets.c.o
libs/FreeRTOS-Plus-TCP/tools/libfreertos_plus_tcp_utilities.a: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_mem_stats.c.o
libs/FreeRTOS-Plus-TCP/tools/libfreertos_plus_tcp_utilities.a: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/tcp_utilities/tcp_netstat.c.o
libs/FreeRTOS-Plus-TCP/tools/libfreertos_plus_tcp_utilities.a: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/build.make
libs/FreeRTOS-Plus-TCP/tools/libfreertos_plus_tcp_utilities.a: libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C static library libfreertos_plus_tcp_utilities.a"
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && $(CMAKE_COMMAND) -P CMakeFiles/freertos_plus_tcp_utilities.dir/cmake_clean_target.cmake
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/freertos_plus_tcp_utilities.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/build: libs/FreeRTOS-Plus-TCP/tools/libfreertos_plus_tcp_utilities.a

.PHONY : libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/build

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/clean:
	cd /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools && $(CMAKE_COMMAND) -P CMakeFiles/freertos_plus_tcp_utilities.dir/cmake_clean.cmake
.PHONY : libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/clean

libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/depend:
	cd /home/waltor/stm_ecu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/waltor/stm_ecu /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/tools /home/waltor/stm_ecu/build /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools /home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : libs/FreeRTOS-Plus-TCP/tools/CMakeFiles/freertos_plus_tcp_utilities.dir/depend

