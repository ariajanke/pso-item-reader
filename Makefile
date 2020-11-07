CXX = g++
LD = g++

#SOURCES  = $(shell ls src | grep '[.]cpp\b' | awk '{print "src/"$$0}')
SOURCES  = $(shell find src | grep '[.]cpp\b')
CXXFLAGS = -std=c++17 -O3 -I./inc -Ilib/cul/inc -Wall -pedantic -Werror -DMACRO_PLATFORM_LINUX

#.PHONY: apir
#apir: SOURCES += $(shell ls src/pso | grep '[.]cpp\b' | awk '{print "src/pso/"$$0}')
#apir: CXXFLAGS += -DMACRO_BUILDING_PSOITEMREADER

OBJECTS_DIR = .debug-build
OBJECTS = $(addprefix $(OBJECTS_DIR)/,$(SOURCES:%.cpp=%.o))

$(OBJECTS_DIR)/%.o: | $(OBJECTS_DIR)/src
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@

.PHONY: default
default: $(OBJECTS)
	g++ $(OBJECTS) -Llib/cul -lncurses -lcap -lcommon-d -O3 -o apir

$(OBJECTS_DIR)/src:
	mkdir -p $(OBJECTS_DIR)/src
	mkdir -p $(OBJECTS_DIR)/src/pso

.PHONY: clean
clean:
	rm -rf $(OBJECTS_DIR)

#apir: APIROBJECTS = $(addprefix $(OBJECTS_DIR)/,$(SOURCES:%.cpp=%.o))
#apir: $(APIROBJECTS)
#	g++ $(OBJECTS) -Llib/cul -lncurses -lcap -lcommon-d -O3 -o apir


