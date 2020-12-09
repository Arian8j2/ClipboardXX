cmake_minimum_required(VERSION 3.19)

project(ClipboardXX)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include/)

if(NOT WIN32)
    find_package(Qt5 COMPONENTS Gui REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE Qt5::Gui)
    target_include_directories(${PROJECT_NAME} INTERFACE ${Qt5Widgets_INCLUDE_DIRS})
endif()
