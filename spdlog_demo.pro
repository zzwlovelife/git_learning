# 顶层 qmake 工程：subdirs 模板，统一构建所有库与示例
TEMPLATE = subdirs

# ---- 共享库 ----
sub_logger.file = lib/logger_shared.pro
sub_dbclient.file = dbclient/dbclient.pro

# ---- spdlog 示例 ----
sub_demo.file = examples/demo.pro
sub_ex_basic.file = examples/ex_basic.pro
sub_ex_async_rotating.file = examples/ex_async_rotating.pro
sub_ex_custom_sink.file = examples/ex_custom_sink.pro
sub_ex_header_only.file = examples/ex_header_only.pro
sub_ex_dynamic_lib.file = examples/ex_dynamic_lib.pro

# ---- dbclient 示例 ----
sub_db_example_cpp.file = dbclient/examples/db_example_cpp.pro
sub_db_example_c.file = dbclient/examples/db_example_c.pro

SUBDIRS += \
    sub_logger \
    sub_dbclient \
    sub_demo \
    sub_ex_basic \
    sub_ex_async_rotating \
    sub_ex_custom_sink \
    sub_ex_header_only \
    sub_ex_dynamic_lib \
    sub_db_example_cpp \
    sub_db_example_c

# ---- 构建依赖顺序 ----
sub_ex_dynamic_lib.depends = sub_logger
sub_db_example_cpp.depends = sub_dbclient
sub_db_example_c.depends = sub_dbclient

# 按 SUBDIRS 顺序构建（与 .depends 配合，确保依赖项先构建）
CONFIG += ordered
