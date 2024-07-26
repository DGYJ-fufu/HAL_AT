# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal"
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/1"
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal"
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal/tmp"
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal/src/Terminal+Terminal-stamp"
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal/src"
  "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal/src/Terminal+Terminal-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal/src/Terminal+Terminal-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/24171/Desktop/Terminal/MDK-ARM/tmp/Terminal+Terminal/src/Terminal+Terminal-stamp${cfgdir}") # cfgdir has leading slash
endif()
