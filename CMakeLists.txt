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

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/thirdparty/glfw/include
    ${PROJECT_SOURCE_DIR}/thirdparty/gl3w/include
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/examples)

list(APPEND LINK_LIBS 
    ${OpenCV_LIBS}
    ${PROJECT_SOURCE_DIR}/lib/win64/glfw3.lib)

file(GLOB_RECURSE SA_SRCS "src/*.cpp")

set(SA_SRCS ${SA_SRCS}
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/gl3w/src/gl3w.c)

add_executable(demo ${PROJECT_SOURCE_DIR}/src/main.cpp ${SA_SRCS})
target_link_libraries(demo ${LINK_LIBS})