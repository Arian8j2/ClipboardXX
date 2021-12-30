cmake_minimum_required(VERSION 3.16)

project(ClipboardXX)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

install(FILES include/clipboardxx.hpp DESTINATION include)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} INTERFACE X11 pthread)
endif()