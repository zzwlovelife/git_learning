/****************************************************************************
** Meta object code from reading C++ file 'example_threads.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../btemplate/include/bthread/example_threads.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'example_threads.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_bthread__DataAcquireThread_t {
    uint offsetsAndSizes[2];
    char stringdata0[27];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_bthread__DataAcquireThread_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_bthread__DataAcquireThread_t qt_meta_stringdata_bthread__DataAcquireThread = {
    {
        QT_MOC_LITERAL(0, 26)   // "bthread::DataAcquireThread"
    },
    "bthread::DataAcquireThread"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_bthread__DataAcquireThread[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject bthread::DataAcquireThread::staticMetaObject = { {
    QMetaObject::SuperData::link<BusinessThread::staticMetaObject>(),
    qt_meta_stringdata_bthread__DataAcquireThread.offsetsAndSizes,
    qt_meta_data_bthread__DataAcquireThread,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_bthread__DataAcquireThread_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DataAcquireThread, std::true_type>
    >,
    nullptr
} };

void bthread::DataAcquireThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *bthread::DataAcquireThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *bthread::DataAcquireThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_bthread__DataAcquireThread.stringdata0))
        return static_cast<void*>(this);
    return BusinessThread::qt_metacast(_clname);
}

int bthread::DataAcquireThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BusinessThread::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_stringdata_bthread__DataProcessThread_t {
    uint offsetsAndSizes[2];
    char stringdata0[27];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_bthread__DataProcessThread_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_bthread__DataProcessThread_t qt_meta_stringdata_bthread__DataProcessThread = {
    {
        QT_MOC_LITERAL(0, 26)   // "bthread::DataProcessThread"
    },
    "bthread::DataProcessThread"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_bthread__DataProcessThread[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject bthread::DataProcessThread::staticMetaObject = { {
    QMetaObject::SuperData::link<BusinessThread::staticMetaObject>(),
    qt_meta_stringdata_bthread__DataProcessThread.offsetsAndSizes,
    qt_meta_data_bthread__DataProcessThread,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_bthread__DataProcessThread_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DataProcessThread, std::true_type>
    >,
    nullptr
} };

void bthread::DataProcessThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *bthread::DataProcessThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *bthread::DataProcessThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_bthread__DataProcessThread.stringdata0))
        return static_cast<void*>(this);
    return BusinessThread::qt_metacast(_clname);
}

int bthread::DataProcessThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BusinessThread::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_stringdata_bthread__DataDistributeThread_t {
    uint offsetsAndSizes[2];
    char stringdata0[30];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_bthread__DataDistributeThread_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_bthread__DataDistributeThread_t qt_meta_stringdata_bthread__DataDistributeThread = {
    {
        QT_MOC_LITERAL(0, 29)   // "bthread::DataDistributeThread"
    },
    "bthread::DataDistributeThread"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_bthread__DataDistributeThread[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject bthread::DataDistributeThread::staticMetaObject = { {
    QMetaObject::SuperData::link<BusinessThread::staticMetaObject>(),
    qt_meta_stringdata_bthread__DataDistributeThread.offsetsAndSizes,
    qt_meta_data_bthread__DataDistributeThread,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_bthread__DataDistributeThread_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DataDistributeThread, std::true_type>
    >,
    nullptr
} };

void bthread::DataDistributeThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *bthread::DataDistributeThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *bthread::DataDistributeThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_bthread__DataDistributeThread.stringdata0))
        return static_cast<void*>(this);
    return BusinessThread::qt_metacast(_clname);
}

int bthread::DataDistributeThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BusinessThread::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
