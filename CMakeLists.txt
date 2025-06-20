cmake_minimum_required(VERSION 3.29)
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
find_package(PkgConfig)
pkg_check_modules(GTKMM_VARS REQUIRED IMPORTED_TARGET gtkmm-4.0)

find_package(assimp CONFIG REQUIRED)
CPMAddPackage(
        NAME threepp
        GITHUB_REPOSITORY cubesky/threepp
        GIT_TAG master
        OPTIONS "THREEPP_BUILD_EXAMPLES OFF" "THREEPP_BUILD_TESTS OFF" "THREEPP_WITH_GLFW OFF"
)
CPMAddPackage(
        NAME oscpp
        GITHUB_REPOSITORY cubesky/oscpp
        GIT_TAG master
        DOWNLOAD_ONLY ON
)
CPMAddPackage(
        NAME Boost
        VERSION 1.88.0 # Versions less than 1.85.0 may need patches for installation targets.
        URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
        OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
        "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES container\\\;asio\\\;optional\\\;date_time\\\;property_tree" # Note the escapes!
)
CPMAddPackage(
        NAME platformdirs
        GIT_TAG main
        GITHUB_REPOSITORY jcbhmr/platformdirs-cpp
)
IF (platformdirs_ADDED)
    target_include_directories(platformdirs PUBLIC ${platformdirs_SOURCE_DIR}/include)
ENDIF ()
find_package(RapidJSON CONFIG REQUIRED)
include_directories(${oscpp_SOURCE_DIR}/include)
find_package(OpenGL REQUIRED)
set(COPY_HIDAPI_DLL OFF)
add_subdirectory(udcap-community-handdriver-core)
file(GLOB_RECURSE SRC_FILES
        src/*.cpp
        src/*.h
        src/*.hpp
)
include_directories(src)
IF (WIN32)
    add_executable(UdCapCommunityDriverUI WIN32 ${SRC_FILES})
ELSE ()
    add_executable(UdCapCommunityDriverUI ${SRC_FILES})
ENDIF ()
target_link_libraries(UdCapCommunityDriverUI PRIVATE OpenGL::GL OpenGL::GLU PkgConfig::GTKMM_VARS threepp assimp::assimp UdCapCommunityDriverCore SerialReborn RapidJSON rapidjson platformdirs::platformdirs Boost::property_tree)

