cmake_minimum_required(VERSION 3.1)
project(SkinningAnimation)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

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

file(GLOB_RECURSE SA_SRCS "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE SA_HDRS "src/*.h" "src/*.hpp")

set(THIRDPARTY_SRCS
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/gl3w/src/gl3w.c)

source_group("Include" FILES ${SA_HDRS})
source_group("Source" FILES ${SA_SRCS})
source_group("ThirdParty" FILES ${THIRDPARTY_SRCS})

add_library(salib ${SA_HDRS} ${SA_SRCS} ${THIRDPARTY_SRCS})
target_link_libraries(salib ${LINK_LIBS})

list(APPEND LINK_LIBS salib)

# imgui demo
add_executable(imgui_demo 
    ${PROJECT_SOURCE_DIR}/main/imgui_demo.cpp)
target_link_libraries(imgui_demo ${LINK_LIBS})

# skinning animation
add_executable(sa
    ${PROJECT_SOURCE_DIR}/main/skinning_animation.cpp)
target_link_libraries(sa ${LINK_LIBS})