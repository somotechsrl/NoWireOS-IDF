# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/localuser/.espressif/v6.0/esp-idf/components/bootloader/subproject"
  "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader"
  "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix"
  "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix/tmp"
  "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix/src/bootloader-stamp"
  "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix/src"
  "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/localuser/Documents/PlatFormIO/Projects/NoWireOS-IDF/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
