cmake_minimum_required(VERSION 3.29)
option(BUILD_STEAMVR_DRIVER "Build native SteamVR controller driver" OFF)
option(NO_3DPREVIEW "Build without the 3D hand preview" OFF)
set(UDCAP_CORE_SOURCE_DIR "" CACHE PATH "Path to the UdCapCommunityDriverCore source root")
if(BUILD_STEAMVR_DRIVER)
    list(APPEND VCPKG_MANIFEST_FEATURES steamvr)
endif()
if(NOT NO_3DPREVIEW)
    list(APPEND VCPKG_MANIFEST_FEATURES preview3d)
else()
    list(REMOVE_ITEM VCPKG_MANIFEST_FEATURES preview3d)
endif()
list(REMOVE_DUPLICATES VCPKG_MANIFEST_FEATURES)
project(UdCapCommunityDriverUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUILD_STEAMVR_DRIVER)
    find_path(OPENVR_DRIVER_INCLUDE_DIR openvr_driver.h
            HINTS "${OPENVR_SDK_ROOT}" "$ENV{OPENVR_SDK_ROOT}"
            PATH_SUFFIXES headers include)
    if(NOT OPENVR_DRIVER_INCLUDE_DIR)
        message(FATAL_ERROR "OpenVR SDK headers not found. Set OPENVR_SDK_ROOT to the OpenVR checkout.")
    endif()
    add_library(driver_udcapc SHARED steamvr/driver_udcap.cpp)
    set_target_properties(driver_udcapc PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    target_include_directories(driver_udcapc PRIVATE "${OPENVR_DRIVER_INCLUDE_DIR}")
    if(WIN32)
        target_link_libraries(driver_udcapc PRIVATE ws2_32)
        set(STEAMVR_PLATFORM_DIR win64)
    elseif(APPLE)
        set(STEAMVR_PLATFORM_DIR osx64)
    else()
        set(STEAMVR_PLATFORM_DIR linux64)
    endif()
    set(STEAMVR_BUNDLE_DIR "${CMAKE_BINARY_DIR}/steamvr/udcapc")
    set_target_properties(driver_udcapc PROPERTIES PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${STEAMVR_BUNDLE_DIR}/bin/${STEAMVR_PLATFORM_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${STEAMVR_BUNDLE_DIR}/bin/${STEAMVR_PLATFORM_DIR}")
    foreach(CONFIG_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(driver_udcapc PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${STEAMVR_BUNDLE_DIR}/bin/${STEAMVR_PLATFORM_DIR}"
                LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${STEAMVR_BUNDLE_DIR}/bin/${STEAMVR_PLATFORM_DIR}")
    endforeach()
    file(MAKE_DIRECTORY "${STEAMVR_BUNDLE_DIR}/resources/input")
    configure_file(steamvr/udcap/driver.vrdrivermanifest
            "${STEAMVR_BUNDLE_DIR}/driver.vrdrivermanifest" COPYONLY)
    file(GLOB_RECURSE STEAMVR_RESOURCE_FILES CONFIGURE_DEPENDS
            RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/steamvr/udcap/resources"
            "${CMAKE_CURRENT_SOURCE_DIR}/steamvr/udcap/resources/*")
    foreach(RESOURCE IN LISTS STEAMVR_RESOURCE_FILES)
        configure_file("steamvr/udcap/resources/${RESOURCE}"
                "${STEAMVR_BUNDLE_DIR}/resources/${RESOURCE}" COPYONLY)
    endforeach()
endif()


file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM_VARS REQUIRED IMPORTED_TARGET gtkmm-4.0)
if(WIN32)
    # cairomm 1.19.1 from vcpkg installs headers under include/cairomm, while
    # its pkg-config file still advertises two cairomm-1.16 include directories.
    get_target_property(UDCAP_GTKMM_INCLUDE_DIRS PkgConfig::GTKMM_VARS INTERFACE_INCLUDE_DIRECTORIES)
    set(UDCAP_GTKMM_VALID_INCLUDE_DIRS)
    set(UDCAP_GTKMM_STALE_CAIROMM_DIRS FALSE)
    foreach(UDCAP_GTKMM_INCLUDE_DIR IN LISTS UDCAP_GTKMM_INCLUDE_DIRS)
        if(IS_DIRECTORY "${UDCAP_GTKMM_INCLUDE_DIR}")
            list(APPEND UDCAP_GTKMM_VALID_INCLUDE_DIRS "${UDCAP_GTKMM_INCLUDE_DIR}")
        elseif(UDCAP_GTKMM_INCLUDE_DIR MATCHES "cairomm-1[.]16")
            set(UDCAP_GTKMM_STALE_CAIROMM_DIRS TRUE)
        else()
            message(FATAL_ERROR "gtkmm pkg-config references a missing include directory: ${UDCAP_GTKMM_INCLUDE_DIR}")
        endif()
    endforeach()
    if(UDCAP_GTKMM_STALE_CAIROMM_DIRS)
        find_path(UDCAP_CAIROMM_HEADER_DIR NAMES cairomm/cairomm.h
                HINTS ${UDCAP_GTKMM_VALID_INCLUDE_DIRS} NO_DEFAULT_PATH NO_CACHE)
        if(NOT UDCAP_CAIROMM_HEADER_DIR)
            message(FATAL_ERROR "cairomm headers are missing from the gtkmm installation.")
        endif()
        set_property(TARGET PkgConfig::GTKMM_VARS PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                "${UDCAP_GTKMM_VALID_INCLUDE_DIRS}")
    endif()
endif()

if(NOT NO_3DPREVIEW)
    find_package(assimp CONFIG REQUIRED)
    CPMAddPackage(
            NAME threepp
            GITHUB_REPOSITORY cubesky/threepp
            GIT_TAG 0d65ba94f5ddade0f42b64f1d4ed802dbc41d0b1
            OPTIONS "THREEPP_BUILD_EXAMPLES OFF" "THREEPP_BUILD_TESTS OFF" "THREEPP_WITH_GLFW OFF"
    )
endif()
CPMAddPackage(
        NAME oscpp
        GITHUB_REPOSITORY cubesky/oscpp
        GIT_TAG 55d300e01cea195a750fe1ced08c403aa5c24826
        DOWNLOAD_ONLY ON
)
CPMAddPackage(
        NAME platformdirs
        GIT_TAG 2dad8b066dd22b960705cbfa809761931b42a7ac
        GITHUB_REPOSITORY jcbhmr/platformdirs-cpp
)
IF (platformdirs_ADDED)
    target_include_directories(platformdirs PUBLIC ${platformdirs_SOURCE_DIR}/include)
ENDIF ()
find_package(RapidJSON CONFIG REQUIRED)
find_package(nng CONFIG REQUIRED)
include_directories(${oscpp_SOURCE_DIR}/include)
if(NOT NO_3DPREVIEW)
    find_package(OpenGL REQUIRED)
endif()
set(COPY_HIDAPI_DLL OFF)
if(UDCAP_CORE_SOURCE_DIR)
    get_filename_component(UDCAP_CORE_SOURCE_DIR "${UDCAP_CORE_SOURCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/udcap-community-handdriver-core/CMakeLists.txt")
    set(UDCAP_CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/udcap-community-handdriver-core")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../UdCap-Community-HandDriver-Core/CMakeLists.txt")
    set(UDCAP_CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../UdCap-Community-HandDriver-Core")
else()
    message(FATAL_ERROR "Core source is missing. Set UDCAP_CORE_SOURCE_DIR to the Core repository root, initialize the submodule, or place Core beside this repository.")
endif()
if(NOT EXISTS "${UDCAP_CORE_SOURCE_DIR}/CMakeLists.txt" OR
   NOT EXISTS "${UDCAP_CORE_SOURCE_DIR}/src/UdCapV1Core.h")
    message(FATAL_ERROR "UDCAP_CORE_SOURCE_DIR must point to the Core repository root: '${UDCAP_CORE_SOURCE_DIR}'.")
endif()
add_subdirectory("${UDCAP_CORE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/udcap-community-handdriver-core")
file(GLOB_RECURSE SRC_FILES
        src/*.cpp
        src/*.h
        src/*.hpp
)
if(NO_3DPREVIEW)
    list(FILTER SRC_FILES EXCLUDE REGEX "[/\\\\]HandThreeppContext[.](cpp|h)$")
endif()
include_directories(src)
IF (WIN32)
    add_executable(UdCapCommunityDriverUI WIN32 ${SRC_FILES})
ELSE ()
    add_executable(UdCapCommunityDriverUI ${SRC_FILES})
ENDIF ()
target_link_libraries(UdCapCommunityDriverUI PRIVATE PkgConfig::GTKMM_VARS UdCapCommunityDriverCore SerialReborn RapidJSON rapidjson platformdirs::platformdirs nng::nng)
if(WIN32 AND TARGET hidapi::hidapi)
    get_target_property(UDCAP_HIDAPI_TARGET hidapi::hidapi ALIASED_TARGET)
    get_target_property(UDCAP_HIDAPI_TYPE ${UDCAP_HIDAPI_TARGET} TYPE)
    if(UDCAP_HIDAPI_TYPE STREQUAL "SHARED_LIBRARY")
        # HIDAPI is built by Core through CPM, so vcpkg's applocal step does not copy it.
        add_custom_command(TARGET UdCapCommunityDriverUI POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:hidapi::hidapi>"
                    "$<TARGET_FILE_DIR:UdCapCommunityDriverUI>"
                VERBATIM)
    endif()
endif()
if(NO_3DPREVIEW)
    target_compile_definitions(UdCapCommunityDriverUI PRIVATE NO_3DPREVIEW=1)
else()
    target_link_libraries(UdCapCommunityDriverUI PRIVATE OpenGL::GL OpenGL::GLU threepp assimp::assimp)
endif()
if(MSVC)
    target_compile_options(UdCapCommunityDriverUI PRIVATE /utf-8)
endif()
find_program(UDCAP_MSGFMT_EXECUTABLE NAMES msgfmt msgfmt.exe REQUIRED)
set(UDCAP_TRANSLATION_MO
        "${CMAKE_CURRENT_BINARY_DIR}/translations/zh_CN/LC_MESSAGES/udcap-community-driver-ui.mo")
add_custom_command(OUTPUT "${UDCAP_TRANSLATION_MO}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/translations/zh_CN/LC_MESSAGES"
        COMMAND "${UDCAP_MSGFMT_EXECUTABLE}" --check --check-format
            -o "${UDCAP_TRANSLATION_MO}"
            "${CMAKE_CURRENT_SOURCE_DIR}/po/zh_CN.po"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/po/zh_CN.po"
        VERBATIM)
add_custom_target(udcap_translations ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:UdCapCommunityDriverUI>/locale/zh_CN/LC_MESSAGES"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${UDCAP_TRANSLATION_MO}"
            "$<TARGET_FILE_DIR:UdCapCommunityDriverUI>/locale/zh_CN/LC_MESSAGES/udcap-community-driver-ui.mo"
        DEPENDS "${UDCAP_TRANSLATION_MO}"
        VERBATIM)
add_dependencies(UdCapCommunityDriverUI udcap_translations)
if(BUILD_STEAMVR_DRIVER)
    add_dependencies(UdCapCommunityDriverUI driver_udcapc)
endif()
find_path(OPENVR_CLIENT_INCLUDE_DIR openvr.h
        HINTS "${OPENVR_SDK_ROOT}" "$ENV{OPENVR_SDK_ROOT}" "${OPENVR_DRIVER_INCLUDE_DIR}"
        PATH_SUFFIXES headers include)
find_library(OPENVR_API_LIBRARY NAMES openvr_api
        HINTS "${OPENVR_SDK_ROOT}" "$ENV{OPENVR_SDK_ROOT}"
        PATH_SUFFIXES lib lib/win64 lib/linux64)
if(OPENVR_CLIENT_INCLUDE_DIR AND OPENVR_API_LIBRARY)
    target_include_directories(UdCapCommunityDriverUI PRIVATE "${OPENVR_CLIENT_INCLUDE_DIR}")
    target_link_libraries(UdCapCommunityDriverUI PRIVATE "${OPENVR_API_LIBRARY}")
    target_compile_definitions(UdCapCommunityDriverUI PRIVATE UDCAP_HAVE_OPENVR_CLIENT=1)
    if(WIN32)
        get_filename_component(OPENVR_LIBRARY_DIR "${OPENVR_API_LIBRARY}" DIRECTORY)
        get_filename_component(OPENVR_LIBRARY_ROOT "${OPENVR_LIBRARY_DIR}" DIRECTORY)
        if(EXISTS "${OPENVR_LIBRARY_ROOT}/bin/openvr_api.dll")
            add_custom_command(TARGET UdCapCommunityDriverUI POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${OPENVR_LIBRARY_ROOT}/bin/openvr_api.dll"
                    "$<TARGET_FILE_DIR:UdCapCommunityDriverUI>")
        endif()
    endif()
endif()

option(BUILD_SENDER_IDLE_TEST "Build the idle network sender regression test" OFF)
if(BUILD_SENDER_IDLE_TEST)
    enable_testing()
    add_executable(UdCapSenderIdleTest
            tests/SenderIdleTest.cpp
            src/components/VMCSender.cpp
            src/components/OSCSender.cpp)
    target_link_libraries(UdCapSenderIdleTest PRIVATE UdCapCommunityDriverCore SerialReborn)
    target_include_directories(UdCapSenderIdleTest PRIVATE src)
    if(MSVC)
        target_compile_options(UdCapSenderIdleTest PRIVATE /utf-8)
    endif()
    add_test(NAME UdCapSenderIdleTest COMMAND UdCapSenderIdleTest)
endif()

option(BUILD_OFFICIAL_POSE_TEST "Build the official hand pose mapping regression test" OFF)
if(BUILD_OFFICIAL_POSE_TEST)
    enable_testing()
    add_executable(UdCapOfficialPoseTest tests/OfficialPoseTest.cpp)
    target_link_libraries(UdCapOfficialPoseTest PRIVATE UdCapCommunityDriverCore)
    if(MSVC)
        target_compile_options(UdCapOfficialPoseTest PRIVATE /utf-8)
    endif()
    add_test(NAME UdCapOfficialPoseTest COMMAND UdCapOfficialPoseTest)
    if(BUILD_STEAMVR_DRIVER)
        add_executable(UdCapSteamVRDriverTest tests/SteamVRDriverTest.cpp)
        target_include_directories(UdCapSteamVRDriverTest PRIVATE "${OPENVR_DRIVER_INCLUDE_DIR}")
        target_link_libraries(UdCapSteamVRDriverTest PRIVATE nlohmann_json::nlohmann_json)
        if(WIN32)
            target_link_libraries(UdCapSteamVRDriverTest PRIVATE ws2_32)
        endif()
        add_test(NAME UdCapSteamVRDriverTest COMMAND UdCapSteamVRDriverTest "${STEAMVR_BUNDLE_DIR}")
    endif()
endif()
