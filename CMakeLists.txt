cmake_minimum_required(VERSION 3.31)
project(UdCapCommunityDriverUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)


find_package(assimp CONFIG REQUIRED)
find_package(wxWidgets CONFIG REQUIRED)

CPMAddPackage(
        NAME threepp
        GITHUB_REPOSITORY cubesky/threepp
        GIT_TAG master
        OPTIONS "THREEPP_BUILD_EXAMPLES=OFF" "THREEPP_BUILD_TESTS=OFF" "THREEPP_WITH_GLFW=OFF"
)
find_package(OpenGL REQUIRED)
file(GLOB_RECURSE SRC_FILES
        src/*.cpp
        src/*.h
        src/*.hpp
        ui/*.h
        ui/*.cpp
)
include_directories(src)
include_directories(ui)
IF (WIN32)
    add_executable(UdCapCommunityDriverUI WIN32 ${SRC_FILES})
ELSE ()
    add_executable(UdCapCommunityDriverUI ${SRC_FILES})
ENDIF ()
target_link_libraries(UdCapCommunityDriverUI PRIVATE OpenGL::GL OpenGL::GLU wx::core wx::base wx::adv wx::gl threepp assimp::assimp)

