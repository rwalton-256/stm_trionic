# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/waltor/stm_ecu/build/_deps/cmock-src"
  "/home/waltor/stm_ecu/build/_deps/cmock-build"
  "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix"
  "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix/tmp"
  "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix/src/cmock-populate-stamp"
  "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix/src"
  "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix/src/cmock-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix/src/cmock-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/waltor/stm_ecu/build/_deps/cmock-subbuild/cmock-populate-prefix/src/cmock-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
