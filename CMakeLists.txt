cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

option(TESTS "compile tests" OFF)

project(ClipboardXX)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

# install header files
install(FILES include/clipboardxx.hpp DESTINATION include)

# dependencies
if(UNIX AND NOT APPLE)
    # TODO: better find xcb package
    target_link_libraries(${PROJECT_NAME} INTERFACE xcb pthread)
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
