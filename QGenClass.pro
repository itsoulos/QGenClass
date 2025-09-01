QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
QMAKE_CXXFLAGS_RELEASE += -O3 -fopenmp -unroll-loops -omit-frame-pointer -Winline -unsafe-math-optimizations -mtune=native  -pthread
QMAKE_CFLAGS_RELEASE += -O3 -march=native -fopenmp -unroll-loops -omit-frame-pointer -Winline -unsafe-math-optimizations -mtune=native -pthread

QMAKE_CXXFLAGS += -O3  -fopenmp -unroll-loops -omit-frame-pointer -Winline -unsafe-math-optimizations -mtune=native  -pthread
QMAKE_CFLAGS += -O3 -fopenmp -unroll-loops -omit-frame-pointer -Winline -unsafe-math-optimizations -mtune=native  -pthread

QMAKE_LFLAGS += -O3
SOURCES += \
        classprogram.cc \
        cprogram.cc \
        dataset.cpp \
        doublestack.cc \
        fparser.cc \
        fpoptimizer.cc \
        integeranneal.cpp \
        main.cpp \
        parameter.cpp \
        parameterlist.cpp \
        population.cc \
        problem.cpp \
        program.cc \
        rule.cc \
        symbol.cc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    classprogram.h \
    cprogram.h \
    dataset.h \
    doublestack.h \
    fparser.hh \
    fpconfig.hh \
    fptypes.hh \
    integeranneal.h \
    parameter.h \
    parameterlist.h \
    population.h \
    problem.h \
    program.h \
    rule.h \
    symbol.h
