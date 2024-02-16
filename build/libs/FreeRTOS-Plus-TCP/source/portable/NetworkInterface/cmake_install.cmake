# Install script for directory: /home/waltor/stm_ecu/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/ATSAM4E/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/ATSAME5x/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/DriverSAM/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/esp32/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/ksz8851snl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/libslirp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/linux/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/LPC17xx/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/LPC18xx/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/LPC54018/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/M487/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/MPS2_AN385/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/mw300_rd/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/pic32mzef/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/RX/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/SH2A/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/STM32Fxx/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/STM32Hxx/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/ThirdParty/MSP432/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/TM4C/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/WinPCap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/xilinx_ultrascale/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/waltor/stm_ecu/build/libs/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/Zynq/cmake_install.cmake")
endif()

