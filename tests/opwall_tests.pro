QT += core gui qml quick quickcontrols2 testlib
CONFIG += c++17 testcase
TARGET = opwall_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/generator.h ../src/graphitem.h ../src/theme.h ../src/translator.h
SOURCES += opwall_tests.cpp ../src/backend.cpp ../src/generator.cpp ../src/graphitem.cpp ../src/theme.cpp ../src/translator.cpp
