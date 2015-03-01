TEMPLATE = app
CONFIG += console c++11
CONFIG -= qt app_bundle

SOURCES += main.cpp \
    lightscript.cpp \
    tokenizer.cpp \
    exprast.cpp \
    codegen.cpp \
    mcjithelper.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    lightscript.h \
    tokenizer.h \
    exprast.h \
    codegen.h \
    mcjithelper.h

QMAKE_CXXFLAGS += $$system(llvm-config --cxxflags)
LIBS += $$system(llvm-config --ldflags --system-libs --libs core mcjit native ipo)
