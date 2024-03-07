cmake_minimum_required(VERSION 3.11)

project(capvicam)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2")

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wcast-qual
    -Wconversion
    -Wctor-dtor-privacy
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
    -Wunused
    -Wunused-result
)

set(SRC_FILES
    src/main.cpp
    src/capture/capture.cpp
    src/mjpeg/mjpeg.cpp
    src/processing/processing.cpp
    src/circular-buffer/circular-buffer.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})