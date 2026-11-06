# spdlog 以 header-only 方式集成
# 用法：在需要使用 spdlog 的 .pro 中加入  include($$PWD/third_party/spdlog/spdlog.pri)
# （路径相对该 .pro 所在目录调整）

SPDLOG_PRI_DIR = $$PWD
INCLUDEPATH += $$SPDLOG_PRI_DIR/include

# spdlog 默认即为 header-only 模式（只要不定义 SPDLOG_COMPILED_LIB）：
#   - common.h 会自动 #define SPDLOG_HEADER_ONLY
#   - fmt/fmt.h 会自动使用 spdlog/fmt/bundled/ 下的内置 fmt
# 因此此处无需额外 DEFINES。
