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
include libs/CMakeFiles/segger.dir/depend.make

# Include the progress variables for this target.
include libs/CMakeFiles/segger.dir/progress.make

# Include the compile flags for this target's objects.
include libs/CMakeFiles/segger.dir/flags.make

libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.o: libs/CMakeFiles/segger.dir/flags.make
libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.o: ../libs/SystemView/SEGGER/SEGGER_RTT.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.o"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.o   -c /home/waltor/stm_ecu/libs/SystemView/SEGGER/SEGGER_RTT.c

libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.i"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/SystemView/SEGGER/SEGGER_RTT.c > CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.i

libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.s"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/SystemView/SEGGER/SEGGER_RTT.c -o CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.s

libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.o: libs/CMakeFiles/segger.dir/flags.make
libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.o: ../libs/SystemView/SEGGER/SEGGER_SYSVIEW.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.o"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.o   -c /home/waltor/stm_ecu/libs/SystemView/SEGGER/SEGGER_SYSVIEW.c

libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.i"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/SystemView/SEGGER/SEGGER_SYSVIEW.c > CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.i

libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.s"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/SystemView/SEGGER/SEGGER_SYSVIEW.c -o CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.s

libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.o: libs/CMakeFiles/segger.dir/flags.make
libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.o: ../libs/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.o"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.o   -c /home/waltor/stm_ecu/libs/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c

libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.i"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c > CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.i

libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.s"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c -o CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.s

libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.o: libs/CMakeFiles/segger.dir/flags.make
libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.o: ../libs/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.o"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.o   -c /home/waltor/stm_ecu/libs/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c

libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.i"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/waltor/stm_ecu/libs/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c > CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.i

libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.s"
	cd /home/waltor/stm_ecu/build/libs && /usr/bin/arm-none-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/waltor/stm_ecu/libs/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c -o CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.s

# Object files for target segger
segger_OBJECTS = \
"CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.o" \
"CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.o" \
"CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.o" \
"CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.o"

# External object files for target segger
segger_EXTERNAL_OBJECTS =

libs/libsegger.a: libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_RTT.c.o
libs/libsegger.a: libs/CMakeFiles/segger.dir/SystemView/SEGGER/SEGGER_SYSVIEW.c.o
libs/libsegger.a: libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c.o
libs/libsegger.a: libs/CMakeFiles/segger.dir/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c.o
libs/libsegger.a: libs/CMakeFiles/segger.dir/build.make
libs/libsegger.a: libs/CMakeFiles/segger.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/waltor/stm_ecu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking C static library libsegger.a"
	cd /home/waltor/stm_ecu/build/libs && $(CMAKE_COMMAND) -P CMakeFiles/segger.dir/cmake_clean_target.cmake
	cd /home/waltor/stm_ecu/build/libs && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/segger.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
libs/CMakeFiles/segger.dir/build: libs/libsegger.a

.PHONY : libs/CMakeFiles/segger.dir/build

libs/CMakeFiles/segger.dir/clean:
	cd /home/waltor/stm_ecu/build/libs && $(CMAKE_COMMAND) -P CMakeFiles/segger.dir/cmake_clean.cmake
.PHONY : libs/CMakeFiles/segger.dir/clean

libs/CMakeFiles/segger.dir/depend:
	cd /home/waltor/stm_ecu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/waltor/stm_ecu /home/waltor/stm_ecu/libs /home/waltor/stm_ecu/build /home/waltor/stm_ecu/build/libs /home/waltor/stm_ecu/build/libs/CMakeFiles/segger.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : libs/CMakeFiles/segger.dir/depend

