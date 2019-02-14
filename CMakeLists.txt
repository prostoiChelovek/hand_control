cmake_minimum_required(VERSION 3.8)
project(mouseControl)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)
find_package(HandDetector REQUIRED)

if (UNIX)
    find_package(X11 REQUIRED)
endif ()
if (WIN32)
    message(SEND_ERROR "Windows interaction is not implemented")
endif ()

set(SOURCE_FILES main.cpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-sign-compare -Wno-narrowing")

add_executable(mouseControl main.cpp SystemInteraction.hpp cvui.h GUI.hpp)

target_link_libraries(mouseControl ${OpenCV_LIBS})
target_link_libraries(mouseControl ${HandDetector_LIBRARY})
target_link_libraries(mouseControl pthread)
include_directories(${HandDetector_INCLUDE_DIR})

if (UNIX)
    target_link_libraries(mouseControl ${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(mouseControl ${X11_XTest_LIB})
    include_directories(${X11_XTest_INCLUDE_PATH})
endif ()
