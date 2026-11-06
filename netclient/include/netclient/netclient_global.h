#ifndef NETCLIENT_GLOBAL_H
#define NETCLIENT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(NETCLIENT_LIB)
#  define NETCLIENT_API Q_DECL_EXPORT
#else
#  define NETCLIENT_API Q_DECL_IMPORT
#endif

#endif // NETCLIENT_GLOBAL_H
