cmake_minimum_required(VERSION 3.16)
project(FoundationGameplayTags)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GODOT_CPP_PATH "/home/loden/Dev/GitHub/Godot-cpp" CACHE PATH "Path to Godot-cpp bindings")
set(GODOT_XXHASH_PATH "/home/loden/Dev/GitHub/Godot-xxHash" CACHE PATH "Path to the Godot-xxHash repo (low-level hashing dependency)")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" TYPE_STRING)

if(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "macos")
else()
    set(PLATFORM "linux")
endif()

set(ARCH "x86_64")

set(XXHASH_SRC_DIR "${GODOT_XXHASH_PATH}/addons/FoundationXxHash/src/thirdparty/xxHash")

include_directories(
    ${GODOT_CPP_PATH}/gen/include
    ${GODOT_CPP_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public
    ${XXHASH_SRC_DIR}
)

message(STATUS "Building: ${CMAKE_BUILD_TYPE} on ${PLATFORM} ${ARCH}")
message(STATUS "GODOT_CPP_PATH: ${GODOT_CPP_PATH}")
message(STATUS "GODOT_XXHASH_PATH: ${GODOT_XXHASH_PATH}")

file(GLOB_RECURSE SOURCES "src/private/*.cpp")
list(APPEND SOURCES "${XXHASH_SRC_DIR}/xxhash.c")

add_library(FoundationGameplayTags SHARED ${SOURCES})

if(WIN32)
    target_link_libraries(FoundationGameplayTags PRIVATE ${GODOT_CPP_PATH}/bin/libgodot-cpp.win.template_release.x86_64${CMAKE_STATIC_LIBRARY_SUFFIX})
elseif(UNIX AND NOT APPLE)
    target_link_libraries(FoundationGameplayTags PRIVATE ${GODOT_CPP_PATH}/bin/libgodot-cpp.linux.template_release.x86_64${CMAKE_STATIC_LIBRARY_SUFFIX})
elseif(APPLE)
    target_link_libraries(FoundationGameplayTags PRIVATE ${GODOT_CPP_PATH}/bin/libgodot-cpp.mac.template_release.x86_64${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

set_target_properties(FoundationGameplayTags
    PROPERTIES
    PREFIX ""
    LINK_FLAGS "-rdynamic"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    OUTPUT_NAME "libFoundationGameplayTags.${PLATFORM}.${TYPE_STRING}.${ARCH}"
)
