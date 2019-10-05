cmake_minimum_required(VERSION 3.1)
project(SkinningAnimation)

option(SA_BUILD_SHARED_LIB "build shared lib" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

if (MSVC)
    # Use static library /MT
    set(CompilerFlags
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    foreach(CompilerFlag ${CompilerFlags})
        if(${CompilerFlag} MATCHES "/MD")
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        endif()
    endforeach()
endif()

set(LINK_LIBS)

# OpenCV Dependence
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
list(APPEND LINK_LIBS ${OpenCV_LIBS})

file(GLOB_RECURSE SA_SRCS "src/*.cpp")