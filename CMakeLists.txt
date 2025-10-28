cmake_minimum_required(VERSION 3.16)
project(SerialGuiTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

# Nếu bạn dùng Qt5, chỉ cần đổi 6 -> 5 ở hai dòng dưới
find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort)

add_executable(${PROJECT_NAME}
    main.cpp
    main_window.cpp
    main_window.h
    serial_worker.cpp
    serial_worker.h
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt5::Widgets Qt5::SerialPort
)
