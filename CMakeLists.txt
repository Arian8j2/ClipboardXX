cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TESTS "compile tests" OFF)

project(ClipboardXX)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

# install header files
install(FILES include/clipboardxx.hpp DESTINATION include)

# dependencies
if(UNIX AND NOT APPLE)
    find_library(XCB NAMES xcb REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${XCB} pthread)
endif()

# tests
if(TESTS)
    add_subdirectory(lib/googletest)
    add_executable(test test/test.cpp)
    target_link_libraries(test ClipboardXX gtest gtest_main)
endif()

# compile options
if(MSVC)
    set(COMPILE_OPTIONS /W4)
else()
    set(COMPILE_OPTIONS -Werror)
endif()
target_compile_options(${PROJECT_NAME} INTERFACE ${COMPILE_OPTIONS})
