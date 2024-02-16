# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.16.3)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(freertos_plus_tcp-populate NONE)

include(ExternalProject)
ExternalProject_Add(freertos_plus_tcp-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git" "GIT_TAG" "main" "GIT_SUBMODULES"
                    SOURCE_DIR          "/home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-src"
                    BINARY_DIR          "/home/waltor/stm_ecu/build/_deps/freertos_plus_tcp-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)
