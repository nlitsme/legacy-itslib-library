cmake_minimum_required(VERSION 3.18)
project(itslib)
set(CMAKE_CXX_STANDARD 17)

list(APPEND ITSLIBSRC debug.cpp stringutils.cpp utfcvutils.cpp vectorutils.cpp FileFunctions.cpp)

list(TRANSFORM ITSLIBSRC PREPEND src/)
add_library(itslib STATIC ${ITSLIBSRC})
target_include_directories(itslib PUBLIC include/itslib)
target_compile_definitions(itslib PUBLIC _NO_RAPI _NO_WINDOWS)
if(NOT WIN32)
    target_compile_definitions(itslib PUBLIC _UNIX)
endif()

