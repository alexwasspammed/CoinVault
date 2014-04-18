#######################################
# coinvault.pro
#
# Copyright (c) 2013 Eric Lombrozo
#
# All Rights Reserved.

# Application icons for windows
RC_FILE = res/coinvault.rc

# Application icons for mac
ICON = res/icons/app_icons/osx.icns

DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE DATABASE_SQLITE
CONFIG += c++11 rtti thread

QT += widgets network

QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas


INCLUDEPATH += \
    sysroot/include \
    /usr/local/include

LIBS += \
    -Lsysroot/lib \
    -lCoinDB \
    -lCoinQ \
    -lCoinCore \
    -llogger

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
} else {
    DESTDIR = build/release
}

OBJECTS_DIR = $$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui

FORMS = \
    src/acceptlicensedialog.ui \
    src/requestpaymentdialog.ui

HEADERS = \
    src/settings.h \
    src/filesystem.h \
    src/versioninfo.h \
    src/copyrightinfo.h \
    src/coinparams.h \
    src/severitylogger.h \
    src/splashscreen.h \
    src/acceptlicensedialog.h \
    src/mainwindow.h \
    src/commandserver.h \
    src/paymentrequest.h \
    src/numberformats.h \
    src/accountmodel.h \
    src/accountview.h \
    src/keychainmodel.h \
    src/keychainview.h \
    src/quicknewaccountdialog.h \
    src/newaccountdialog.h \
    src/newkeychaindialog.h \
    src/rawtxdialog.h \
    src/createtxdialog.h \
    src/txmodel.h \
    src/txview.h \
    src/accounthistorydialog.h \
    src/txactions.h \
    src/scriptmodel.h \
    src/scriptview.h \
    src/scriptdialog.h \
    src/requestpaymentdialog.h \
    src/networksettingsdialog.h \
    src/keychainbackupdialog.h \
    src/passphrasedialog.h \
    src/resyncdialog.h
                
SOURCES = \
    src/settings.cpp \
    src/filesystem.cpp \
    src/versioninfo.cpp \
    src/coinparams.cpp \
    src/main.cpp \
    src/splashscreen.cpp \
    src/acceptlicensedialog.cpp \
    src/mainwindow.cpp \
    src/commandserver.cpp \
    src/paymentrequest.cpp \
    src/numberformats.cpp \
    src/accountmodel.cpp \
    src/accountview.cpp \
    src/keychainmodel.cpp \
    src/keychainview.cpp \
    src/quicknewaccountdialog.cpp \
    src/newaccountdialog.cpp \
    src/newkeychaindialog.cpp \
    src/rawtxdialog.cpp \
    src/createtxdialog.cpp \
    src/txmodel.cpp \
    src/txview.cpp \
    src/accounthistorydialog.cpp \
    src/txactions.cpp \
    src/scriptmodel.cpp \
    src/scriptview.cpp \
    src/scriptdialog.cpp \
    src/requestpaymentdialog.cpp \
    src/networksettingsdialog.cpp \
    src/keychainbackupdialog.cpp \
    src/passphrasedialog.cpp \
    src/resyncdialog.cpp

RESOURCES = \
    res/coinvault.qrc \
    docs/docs.qrc

# install
target.path = build 
INSTALLS += target

win32 {
    BOOST_LIB_SUFFIX = -mt-s
    BOOST_THREAD_LIB_SUFFIX = _win32

    LIBS += \
        -L/usr/x86_64-w64-mingw32/plugins/platforms \
        -static-libgcc -static-libstdc++ \
        -lws2_32 \
        -lmswsock
}

macx {
    isEmpty(BOOST_LIB_PATH) {
        BOOST_LIB_PATH = /usr/local/lib
    }

    exists($$BOOST_LIB_PATH/libboost_system-mt*) {
        BOOST_LIB_SUFFIX = -mt
    }

    LIBS += \
        -L$$BOOST_LIB_PATH
}

LIBS += \
    -lboost_system$$BOOST_LIB_SUFFIX \
    -lboost_filesystem$$BOOST_LIB_SUFFIX \
    -lboost_regex$$BOOST_LIB_SUFFIX \
    -lboost_thread$$BOOST_THREAD_LIB_SUFFIX$$BOOST_LIB_SUFFIX \
    -lboost_serialization$$BOOST_LIB_SUFFIX \
    -lcrypto \
    -lodb-sqlite \
    -lodb
