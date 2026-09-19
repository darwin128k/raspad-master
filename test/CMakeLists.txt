# Parent still has raspad-master; this file's project() overwrites PROJECT_NAME.
set(RASPAD_MASTER_CORE_TARGET raspad_master)
project(raspad_master_test)

set(RASPAD_GTEST_PATH "${RASPAD_LH_ROOT}/test/lib/googletest")
if (NOT EXISTS "${RASPAD_GTEST_PATH}/CMakeLists.txt")
    message(WARNING "googletest not found at ${RASPAD_GTEST_PATH} — tests disabled")
    return()
endif ()

if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()

add_subdirectory(${RASPAD_GTEST_PATH} googletest)

set(RASPAD_TEST_SOURCES
        src/main.cpp
        src/raspad/master/registry.cpp
        src/raspad/master/query.cpp
        src/raspad/master/flood.cpp
        src/raspad/master/http.cpp
        src/raspad/master/udp.cpp
)

add_executable(${PROJECT_NAME} ${RASPAD_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${RASPAD_MASTER_CORE_TARGET} gtest)

add_test(NAME ${PROJECT_NAME} COMMAND "$<TARGET_FILE:${PROJECT_NAME}>")
