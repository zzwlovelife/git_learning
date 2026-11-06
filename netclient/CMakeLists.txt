cmake_minimum_required(VERSION 3.16)
project(netclient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets)

add_library(netclient SHARED
    src/http_client.cpp
    src/websocket_client.cpp
    src/mqtt_client.cpp
    src/rpc_client.cpp
    src/net_client.cpp
    include/netclient/netclient_global.h
    include/netclient/http_client.h
    include/netclient/websocket_client.h
    include/netclient/mqtt_client.h
    include/netclient/rpc_client.h
    include/netclient/net_client.h
)

target_compile_definitions(netclient PRIVATE NETCLIENT_LIB)
target_include_directories(netclient
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(netclient PUBLIC Qt6::Core Qt6::Network Qt6::WebSockets)

set_target_properties(netclient PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    AUTOMOC ON
)

# 示例
add_executable(netclient_demo examples/netclient_demo.cpp)
target_link_libraries(netclient_demo PRIVATE netclient Qt6::Core)
set_target_properties(netclient_demo PROPERTIES AUTOMOC ON)
