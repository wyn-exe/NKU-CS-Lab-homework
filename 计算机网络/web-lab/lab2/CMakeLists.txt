cmake_minimum_required(VERSION 3.12)
project(reliable_udp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rudp
    src/reliable_udp.cpp
)
target_include_directories(rudp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(rudp PRIVATE /utf-8)
endif()

if(WIN32)
    target_link_libraries(rudp PUBLIC ws2_32)
endif()

add_executable(rudp_sender src/sender_main.cpp)
target_link_libraries(rudp_sender PRIVATE rudp)

add_executable(rudp_receiver src/receiver_main.cpp)
target_link_libraries(rudp_receiver PRIVATE rudp)

if(MSVC)
    target_compile_options(rudp_sender PRIVATE /utf-8)
    target_compile_options(rudp_receiver PRIVATE /utf-8)
endif()
