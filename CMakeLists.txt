cmake_minimum_required(VERSION 3.16)

project(embedded_ota_bridge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ota_core
    src/crc32.cpp
    src/firmware_receiver.cpp
    src/firmware_sender.cpp
    src/protocol.cpp
    src/simulated_flash.cpp
    src/tcp_socket.cpp
)
target_include_directories(ota_core PUBLIC include)

if(WIN32)
    target_link_libraries(ota_core PUBLIC ws2_32)
    # windows.h defines min/max macros that break std::numeric_limits<T>::max().
    target_compile_definitions(ota_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(MSVC)
    target_compile_options(ota_core PRIVATE /W4 /WX)
else()
    target_compile_options(ota_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(ota_server src/server_main.cpp)
target_link_libraries(ota_server PRIVATE ota_core)

add_executable(ota_client src/client_main.cpp)
target_link_libraries(ota_client PRIVATE ota_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(ota_tests tests/ota_tests.cpp)
    target_link_libraries(ota_tests PRIVATE ota_core)
    add_test(NAME ota_tests COMMAND ota_tests)
endif()
