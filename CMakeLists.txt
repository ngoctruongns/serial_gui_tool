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

include_directories(include)

file(GLOB_RECURSE HEADER include/*.h)
file(GLOB_RECURSE SOURCES src/*.cpp)

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
