#ifndef DBCLIENT_GLOBAL_H
#define DBCLIENT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DBCLIENT_LIB)
#  define DBCLIENT_API Q_DECL_EXPORT
#else
#  define DBCLIENT_API Q_DECL_IMPORT
#endif

#endif // DBCLIENT_GLOBAL_H
