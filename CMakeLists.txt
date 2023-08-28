cmake_minimum_required(VERSION 3.18)
project(itslib)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_find")

find_package(openssl)

list(APPEND ITSLIBSRC debug.cpp stringutils.cpp utfcvutils.cpp vectorutils.cpp FileFunctions.cpp)

list(TRANSFORM ITSLIBSRC PREPEND src/)
add_library(itslib STATIC ${ITSLIBSRC})
target_link_libraries(itslib PUBLIC OpenSSL::Crypto)
target_include_directories(itslib PUBLIC include/itslib)
target_compile_definitions(itslib PUBLIC _NO_RAPI _NO_WINDOWS)
if(NOT WIN32)
    # NOTE: silence this warning because we still use std::iterator<...>
    # in util/chariterators.h and util/endianutil.h
    target_compile_options(itslib PUBLIC -Wno-deprecated-declarations)
    target_compile_definitions(itslib PUBLIC _UNIX)
endif()

