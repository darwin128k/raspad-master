cmake_minimum_required(VERSION 3.12)
project(raspad-master
        DESCRIPTION "Raspad GoldSrc master (UDP 27010)"
        LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(RASPAD_MASTER_BUILD_TESTS "Build raspad-master tests" ON)
if (RASPAD_MASTER_BUILD_TESTS)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

set(RASPAD_LH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/lh" CACHE PATH "Path to the lh source tree")

if (NOT EXISTS "${RASPAD_LH_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
            "lh not found at RASPAD_LH_ROOT=${RASPAD_LH_ROOT}\n"
            "Run: git submodule update --init --recursive")
endif ()

set(LH_BUILD_SHARED OFF CACHE BOOL "Link lh statically into raspad-master" FORCE)
set(LH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LH_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(LH_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(LH_LIBRARY_OPTION_OS ON CACHE BOOL "" FORCE)
add_subdirectory("${RASPAD_LH_ROOT}" "${CMAKE_BINARY_DIR}/lh")

set(RASPAD_MASTER_LIB_SOURCES
        src/raspad/master/config.c
        src/raspad/master/registry.c
        src/raspad/master/query.c
        src/raspad/master/flood.c
        src/raspad/master/udp.c
        src/raspad/master.c
)

add_library(raspad_master STATIC ${RASPAD_MASTER_LIB_SOURCES})
target_include_directories(raspad_master
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(raspad_master
        PUBLIC
        lh
)

add_executable(raspad-master src/raspad/main.c)
target_link_libraries(raspad-master PRIVATE raspad_master)

if (RASPAD_MASTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()
