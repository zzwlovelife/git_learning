/****************************************************************************
** Meta object code from reading C++ file 'business_thread.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../btemplate/include/bthread/business_thread.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'business_thread.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_bthread__BusinessThread_t {
    uint offsetsAndSizes[36];
    char stringdata0[24];
    char stringdata1[8];
    char stringdata2[1];
    char stringdata3[5];
    char stringdata4[8];
    char stringdata5[6];
    char stringdata6[4];
    char stringdata7[11];
    char stringdata8[6];
    char stringdata9[4];
    char stringdata10[15];
    char stringdata11[17];
    char stringdata12[10];
    char stringdata13[9];
    char stringdata14[6];
    char stringdata15[5];
    char stringdata16[5];
    char stringdata17[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_bthread__BusinessThread_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_bthread__BusinessThread_t qt_meta_stringdata_bthread__BusinessThread = {
    {
        QT_MOC_LITERAL(0, 23),  // "bthread::BusinessThread"
        QT_MOC_LITERAL(24, 7),  // "started"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 4),  // "name"
        QT_MOC_LITERAL(38, 7),  // "stopped"
        QT_MOC_LITERAL(46, 5),  // "error"
        QT_MOC_LITERAL(52, 3),  // "err"
        QT_MOC_LITERAL(56, 10),  // "logMessage"
        QT_MOC_LITERAL(67, 5),  // "level"
        QT_MOC_LITERAL(73, 3),  // "msg"
        QT_MOC_LITERAL(77, 14),  // "_messagePosted"
        QT_MOC_LITERAL(92, 16),  // "bthread::Message"
        QT_MOC_LITERAL(109, 9),  // "onMessage"
        QT_MOC_LITERAL(119, 8),  // "LogLevel"
        QT_MOC_LITERAL(128, 5),  // "Debug"
        QT_MOC_LITERAL(134, 4),  // "Info"
        QT_MOC_LITERAL(139, 4),  // "Warn"
        QT_MOC_LITERAL(144, 5)   // "Error"
    },
    "bthread::BusinessThread",
    "started",
    "",
    "name",
    "stopped",
    "error",
    "err",
    "logMessage",
    "level",
    "msg",
    "_messagePosted",
    "bthread::Message",
    "onMessage",
    "LogLevel",
    "Debug",
    "Info",
    "Warn",
    "Error"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_bthread__BusinessThread[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       1,   74, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       4,    1,   53,    2, 0x06,    3 /* Public */,
       5,    2,   56,    2, 0x06,    5 /* Public */,
       7,    3,   61,    2, 0x06,    8 /* Public */,
      10,    1,   68,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    1,   71,    2, 0x0a,   14 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    6,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString,    8,    3,    9,
    QMetaType::Void, 0x80000000 | 11,    9,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 11,    9,

 // enums: name, alias, flags, count, data
      13,   13, 0x0,    4,   79,

 // enum data: key, value
      14, uint(bthread::BusinessThread::Debug),
      15, uint(bthread::BusinessThread::Info),
      16, uint(bthread::BusinessThread::Warn),
      17, uint(bthread::BusinessThread::Error),

       0        // eod
};

Q_CONSTINIT const QMetaObject bthread::BusinessThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_bthread__BusinessThread.offsetsAndSizes,
    qt_meta_data_bthread__BusinessThread,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_bthread__BusinessThread_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BusinessThread, std::true_type>,
        // method 'started'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'stopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'error'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method '_messagePosted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const bthread::Message &, std::false_type>,
        // method 'onMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const bthread::Message &, std::false_type>
    >,
    nullptr
} };

void bthread::BusinessThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BusinessThread *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->started((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->stopped((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->error((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->logMessage((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 4: _t->_messagePosted((*reinterpret_cast< std::add_pointer_t<bthread::Message>>(_a[1]))); break;
        case 5: _t->onMessage((*reinterpret_cast< std::add_pointer_t<bthread::Message>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< bthread::Message >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< bthread::Message >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BusinessThread::*)(const QString & );
            if (_t _q_method = &BusinessThread::started; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BusinessThread::*)(const QString & );
            if (_t _q_method = &BusinessThread::stopped; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BusinessThread::*)(const QString & , const QString & );
            if (_t _q_method = &BusinessThread::error; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BusinessThread::*)(int , const QString & , const QString & );
            if (_t _q_method = &BusinessThread::logMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (BusinessThread::*)(const bthread::Message & );
            if (_t _q_method = &BusinessThread::_messagePosted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *bthread::BusinessThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *bthread::BusinessThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_bthread__BusinessThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int bthread::BusinessThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void bthread::BusinessThread::started(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void bthread::BusinessThread::stopped(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void bthread::BusinessThread::error(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void bthread::BusinessThread::logMessage(int _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void bthread::BusinessThread::_messagePosted(const bthread::Message & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
