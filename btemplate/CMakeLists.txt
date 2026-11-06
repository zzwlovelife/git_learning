cmake_minimum_required(VERSION 3.16)
project(btemplate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

# spdlog header-only（复用项目已有的 third_party/spdlog）
set(SPDLOG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/spdlog/include)

add_executable(btemplate
    src/main.cpp
    src/business_thread.cpp
    src/thread_manager.cpp
    include/bthread/message.h
    include/bthread/message_bus.h
    include/bthread/business_thread.h
    include/bthread/thread_manager.h
    include/bthread/example_threads.h
)

target_include_directories(btemplate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SPDLOG_INCLUDE}
)
target_link_libraries(btemplate PRIVATE Qt6::Core)
set_target_properties(btemplate PROPERTIES AUTOMOC ON)
