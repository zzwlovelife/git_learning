#ifndef NETCLIENT_GLOBAL_H
#define NETCLIENT_GLOBAL_H

#include <QtCore/qglobal.h>

// =============================================================================
// 动态库导出/导入宏
// -----------------------------------------------------------------------------
// 编译 netclient 库时定义 NETCLIENT_LIB（在 CMakeLists.txt/.pro 中设置），
// 此时 NETCLIENT_API 展开为 Q_DECL_EXPORT，把符号导出到动态库；
// 使用该库的应用程序不定义 NETCLIENT_LIB，展开为 Q_DECL_IMPORT，从库导入符号。
// =============================================================================
#if defined(NETCLIENT_LIB)
#  define NETCLIENT_API Q_DECL_EXPORT
#else
#  define NETCLIENT_API Q_DECL_IMPORT
#endif

#endif // NETCLIENT_GLOBAL_H
