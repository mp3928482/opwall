QT += core gui qml quick quickcontrols2

CONFIG += c++17 release
TARGET = opwall
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/generator.h \
    src/graphitem.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/generator.cpp \
    src/graphitem.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
