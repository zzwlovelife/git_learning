cmake_minimum_required(VERSION 3.16)

# 数据库客户端动态库：基于 Qt QSqlDatabase + QODBC + unixODBC + DM ODBC
add_library(dbclient SHARED
    src/db_client.cpp
    src/db_client_c.cpp
    src/connection_pool.cpp
    src/scoped_connection.cpp
    include/dbclient/dbclient_global.h
    include/dbclient/db_config.h
    include/dbclient/db_result.h
    include/dbclient/db_client.h
    include/dbclient/db_client_c.h
)

target_compile_definitions(dbclient PRIVATE DBCLIENT_LIB)
target_compile_features(dbclient PUBLIC cxx_std_17)

target_include_directories(dbclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(Qt6 REQUIRED COMPONENTS Core Sql)
target_link_libraries(dbclient PUBLIC Qt6::Core Qt6::Sql)

# 自动设置 Qt 插件路径，确保运行时能找到 QODBC 插件
set_target_properties(dbclient PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# 示例：C++ API，日志回调接入 spdlog
add_executable(db_example_cpp examples/db_example_cpp.cpp)
target_link_libraries(db_example_cpp PRIVATE dbclient spdlog::spdlog)
set_target_properties(db_example_cpp PROPERTIES AUTOMOC ON)

# 示例：C API（C++ 调用方，演示 C ABI 接口，日志接 spdlog）
add_executable(db_example_c examples/db_example_c.cpp)
target_link_libraries(db_example_c PRIVATE dbclient spdlog::spdlog)
set_target_properties(db_example_c PROPERTIES AUTOMOC ON)
