QT      -= core gui
CONFIG  -= c++11

QMAKE_CXXFLAGS += -std=c++17 -pedantic -Wall -DMACRO_USE_MT_INITIALIZATION -DMACRO_BUILDING_MEMR -DMACRO_BUILDING_PSOITEMREADER
QMAKE_LFLAGS   += -std=c++17
INCLUDEPATH    += ../lib/cul/inc 
#                 have to use absolute file paths
LIBS           += "-L$$PWD/../lib/cul" 
LIBS           += -lcap -lcommon-d -lncurses
                  

DEFINES += MACRO_COMPILER_GCC

SOURCES += \
    ../src/main.cpp \
    ../src/Defs.cpp \
    ../src/AppStateDefs.cpp \
    ../src/NCursesGrid.cpp \
    ../src/MemoryReader.cpp \
    \ # PSO Item Reader
    ../src/pso/ItemDb.cpp \
    ../src/pso/Item.cpp \
    ../src/pso/ItemReader.cpp \
    ../src/pso/ItemReaderBaseState.cpp \
    ../src/pso/ItemReaderStates.cpp \
    ../src/pso/ProcessWatcher.cpp


HEADERS += \
    ../src/Defs.hpp \
    ../src/AppStateDefs.hpp \
    ../src/NCursesGrid.hpp \
    ../src/MemoryReader.hpp \
    \ # PSO Item Reader
    ../src/pso/ItemDb.hpp \
    ../src/pso/Item.hpp \
    ../src/pso/ItemReader.hpp \
    ../src/pso/ItemReaderBaseState.hpp \
    ../src/pso/ItemReaderStates.hpp \
    ../src/pso/ProcessWatcher.hpp
