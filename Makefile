# ─── Compiler & Flags ────────────────────────────────────────────────────────
CXX      := g++
CXXFLAGS := -std=c++11 -g -Wall \
            -I./include \
            -I./src

LDFLAGS  := -lglfw -pthread -lGLEW -ldl -lGL

# ─── Sources ──────────────────────────────────────────────────────────────────
# Recursively find all .cpp files under src/
SRCS     := $(shell find src/ -name "*.cpp")

# Entry point
MAIN     := src/scene/main.cpp

# All other .cpp files (exclude main.cpp to avoid duplicate main)
LIB_SRCS := $(filter-out $(MAIN), $(SRCS))

# ─── Objects ─────────────────────────────────────────────────────────────────
LIB_OBJS := $(LIB_SRCS:.cpp=.o)
MAIN_OBJ := $(MAIN:.cpp=.o)

ALL_OBJS := $(LIB_OBJS) $(MAIN_OBJ)

# ─── Target ───────────────────────────────────────────────────────────────────
TARGET   := main

# ─── Rules ────────────────────────────────────────────────────────────────────
all: clean $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Compile all .cpp files to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	find src/ -name "*.o" -delete
	rm -f $(TARGET)

run:
	./$(TARGET)

rebuild: clean $(TARGET) run

.PHONY: all clean run rebuild