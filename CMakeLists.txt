cmake_minimum_required(VERSION 3.1)
project(SkinningAnimation)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Debug Logging Level
add_definitions(-D MAX_LOG_LEVEL=3)

set(LINK_LIBS)
# OpenCV Dependence
find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/thirdparty/glm
    ${PROJECT_SOURCE_DIR}/thirdparty/glfw/include
    ${PROJECT_SOURCE_DIR}/thirdparty/gl3w/include
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui
    ${PROJECT_SOURCE_DIR}/thirdparty/filesystem
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo)

list(APPEND LINK_LIBS 
    ${OpenCV_LIBS}
    ${PROJECT_SOURCE_DIR}/lib/win64/glfw3.lib)

file(GLOB_RECURSE SA_SRCS "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE SA_HDRS "src/*.h" "src/*.hpp")


set(THIRDPARTY_HDRS
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imconfig.h
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui.h
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_internal.h
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imstb_rectpack.h
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imstb_textedit.h
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imstb_truetype.h
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImCurveEdit.h
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImGradient.h
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImGuizmo.h
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImSequencer.h)

set(THIRDPARTY_SRCS
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/gl3w/src/gl3w.c
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImCurveEdit.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImGradient.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImGuizmo.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/ImGuizmo/ImSequencer.cpp)

source_group("Include" FILES ${SA_HDRS})
source_group("Source" FILES ${SA_SRCS})
source_group("ThirdPartySource" FILES ${THIRDPARTY_SRCS})
source_group("ThirdPartyHeader" FILES ${THIRDPARTY_HDRS})

add_library(sa_utils ${SA_HDRS} ${SA_SRCS} ${THIRDPARTY_SRCS} ${THIRDPARTY_HDRS})
target_link_libraries(sa_utils ${LINK_LIBS})

list(APPEND LINK_LIBS sa_utils)

# imgui demo
add_executable(demo 
    ${PROJECT_SOURCE_DIR}/main/demo.cpp)
target_link_libraries(demo ${LINK_LIBS})

# skinning animation
add_executable(sa
    ${PROJECT_SOURCE_DIR}/main/skinning_animation.cpp)
target_link_libraries(sa ${LINK_LIBS})