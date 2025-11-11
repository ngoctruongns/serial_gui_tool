cmake_minimum_required(VERSION 3.16)
project(SerialGuiTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

if (WIN32)
    message(STATUS "Building on Windows: disable ASan flags")
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_LINKER_FLAGS "")
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort Charts)

set(HEADER
    main_window.h
    serial_worker.h
    plot_widget.h
)

set(SOURCES
    main.cpp
    main_window.cpp
    main_window_ui.cpp
    search_helper.cpp
    serial_worker.cpp
    plot_widget.cpp
)

add_executable(${PROJECT_NAME}
    ${HEADER}
    ${SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt5::Widgets
    Qt5::SerialPort
    Qt5::Charts
)
