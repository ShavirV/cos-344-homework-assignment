# ─── Detect OS ───────────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)
# ─── Compiler & Flags ────────────────────────────────────────────────────────
CXX      := g++
CXXFLAGS := -std=c++11 -g -Wall \
            -I./include \
            -I./src
# ─── Platform-specific flags ─────────────────────────────────────────────────
ifeq ($(UNAME_S), Darwin)
BREW_PREFIX := $(shell brew --prefix)
CXXFLAGS    += -I$(BREW_PREFIX)/include
LDFLAGS     := -L$(BREW_PREFIX)/lib \
                   -lglfw -lGLEW \
                   -framework OpenGL \
                   -framework Cocoa \
                   -framework IOKit \
                   -framework CoreVideo
else ifeq ($(UNAME_S), Linux)
LDFLAGS := -lglfw -pthread -lGLEW -ldl -lGL
else
# Windows (MinGW)
LDFLAGS := -lglfw3 -lglew32 -lopengl32 -lgdi32
endif
# ─── Sources ─────────────────────────────────────────────────────────────────
SRCS     := $(shell find src/ -name "*.cpp")
MAIN     := src/scene/main.cpp
DEV      := src/scene/dev.cpp
LIB_SRCS := $(filter-out $(MAIN) $(DEV), $(SRCS))
# ─── Objects ─────────────────────────────────────────────────────────────────
LIB_OBJS  := $(LIB_SRCS:.cpp=.o)
MAIN_OBJ  := $(MAIN:.cpp=.o)
DEV_OBJ   := $(DEV:.cpp=.o)
ALL_OBJS  := $(LIB_OBJS) $(MAIN_OBJ)
DEV_OBJS  := $(LIB_OBJS) $(DEV_OBJ)
# ─── Targets ─────────────────────────────────────────────────────────────────
TARGET     := main
DEV_BIN    := dev_bin
# ─── Rules ───────────────────────────────────────────────────────────────────
all: clean $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	./$(TARGET)

dev: clean
	$(MAKE) $(DEV_BIN)
	./$(DEV_BIN)

$(DEV_BIN): $(DEV_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	find src/ -name "*.o" -delete
	rm -f $(TARGET) $(DEV_BIN)

run:
	./$(TARGET)

dev-fast: $(DEV_BIN)
	./$(DEV_BIN)
	
rebuild: clean $(TARGET) run

.PHONY: all dev clean run rebuild