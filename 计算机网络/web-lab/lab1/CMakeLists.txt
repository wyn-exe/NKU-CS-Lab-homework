cmake_minimum_required(VERSION 3.16)
project(NetworkLabs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(/utf-8)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(chat_common ${SRC_DIR}/common/chat_protocol.cpp)
target_include_directories(chat_common PUBLIC ${SRC_DIR})
target_link_libraries(chat_common PUBLIC ws2_32)

add_executable(chat_server ${SRC_DIR}/server/main.cpp)
target_link_libraries(chat_server PRIVATE chat_common ws2_32)

add_executable(chat_client ${SRC_DIR}/client/main.cpp)
target_link_libraries(chat_client PRIVATE chat_common ws2_32)

add_executable(chat_client_gui WIN32 ${SRC_DIR}/client_gui/main.cpp)
target_link_libraries(chat_client_gui PRIVATE chat_common ws2_32)

add_library(rdt_common ${SRC_DIR}/rdt_common/rdt_protocol.cpp)
target_include_directories(rdt_common PUBLIC ${SRC_DIR})
target_link_libraries(rdt_common PUBLIC ws2_32)

add_executable(rdt_server ${SRC_DIR}/rdt_server/main.cpp)
target_link_libraries(rdt_server PRIVATE rdt_common ws2_32)

add_executable(rdt_client ${SRC_DIR}/rdt_client/main.cpp)
target_link_libraries(rdt_client PRIVATE rdt_common ws2_32)
