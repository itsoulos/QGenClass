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
        CORE/dataset.cpp \
        CORE/parameter.cpp \
        CORE/parameterlist.cpp \
        CORE/problem.cpp \
        GE/classprogram.cc \
        GE/cprogram.cc \
        GE/doublestack.cc \
        GE/fparser.cc \
        GE/fpoptimizer.cc \
        GE/integeranneal.cpp \
        GE/integerhill.cpp \
        GE/population.cc \
        GE/program.cc \
        GE/rule.cc \
        GE/symbol.cc \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    CORE/dataset.h \
    CORE/parameter.h \
    CORE/parameterlist.h \
    CORE/problem.h \
    GE/classprogram.h \
    GE/cprogram.h \
    GE/doublestack.h \
    GE/fparser.hh \
    GE/fpconfig.hh \
    GE/fptypes.hh \
    GE/integeranneal.h \
    GE/integerhill.h \
    GE/population.h \
    GE/program.h \
    GE/rule.h \
    GE/symbol.h
