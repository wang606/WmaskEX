cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
project(WmaskEX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(glbinding CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

macro(add_spine_opengl_library version)
    file(GLOB SPINE_CPP
        "src/spine/spine-cpp-${version}/include/spine/*.h"
        "src/spine/spine-cpp-${version}/src/spine/*.cpp")
    add_library(spine_opengl_${version} SHARED
        ${SPINE_CPP}
        "src/spine/spine-opengl/stb_image.h"
        "src/spine/spine-opengl/spine-opengl.h"
        "src/spine/spine-opengl/spine-opengl.cpp"
        "src/spine/spine-opengl/SpineRuntime.cpp")
    target_include_directories(spine_opengl_${version} PRIVATE "src/spine/spine-cpp-${version}/include")
    target_include_directories(spine_opengl_${version} PRIVATE "src")
    target_include_directories(spine_opengl_${version} PRIVATE "src/spine/spine-opengl")
    target_compile_definitions(spine_opengl_${version} PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 UNICODE _UNICODE)
    target_compile_definitions(spine_opengl_${version} PRIVATE SPINE${version})
    target_link_libraries(spine_opengl_${version} PRIVATE glbinding::glbinding)
endmacro()

add_spine_opengl_library(37)
add_spine_opengl_library(38)
add_spine_opengl_library(40)
add_spine_opengl_library(41)
add_spine_opengl_library(42)

add_executable(WmaskEX WIN32
    src/header.h
    src/main.cpp
    src/ISpineRuntime.h
    src/WmaskEXImage.cpp
    src/WmaskEXSpine.cpp
    src/WmaskEXMainWindow.cpp
    src/WmaskEXUtils.cpp
    src/res/resource.h
    src/res/resources.rc
)
target_link_libraries(WmaskEX
    spine_opengl_37
    spine_opengl_38
    spine_opengl_40
    spine_opengl_41
    spine_opengl_42
)
target_link_libraries(WmaskEX nlohmann_json::nlohmann_json gdiplus OpenGL::GL glbinding::glbinding)
set_target_properties(WmaskEX PROPERTIES WIN32_EXECUTABLE TRUE LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
# set_target_properties(WmaskEX PROPERTIES WIN32_EXECUTABLE TRUE LINK_FLAGS "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
target_compile_definitions(WmaskEX PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 UNICODE _UNICODE)

add_executable(WmaskEXSpineConfigAutoGenerator WIN32
    src/header.h
    src/WmaskEXSpineConfigAutoGenerator.cpp
    src/ISpineRuntime.h
    src/WmaskEXImage.cpp
    src/WmaskEXSpine.cpp
    src/WmaskEXMainWindow.cpp
    src/WmaskEXUtils.cpp
    src/res/resource.h
    src/res/resources.rc
)
target_link_libraries(WmaskEXSpineConfigAutoGenerator
    spine_opengl_37
    spine_opengl_38
    spine_opengl_40
    spine_opengl_41
    spine_opengl_42
)
target_link_libraries(WmaskEXSpineConfigAutoGenerator nlohmann_json::nlohmann_json gdiplus OpenGL::GL glbinding::glbinding)
set_target_properties(WmaskEXSpineConfigAutoGenerator PROPERTIES WIN32_EXECUTABLE TRUE LINK_FLAGS "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
target_compile_definitions(WmaskEXSpineConfigAutoGenerator PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 UNICODE _UNICODE)
target_compile_options(WmaskEXSpineConfigAutoGenerator PRIVATE /utf-8)
