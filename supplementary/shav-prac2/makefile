files = shader.cpp Triangle.cpp Square.cpp

CXX = g++
CXXFLAGS = -std=c++11 -g -Wall

LIBS = -lglfw -lGLEW -lGL -ldl -lpthread

main: main.cpp $(files)
	$(CXX) $(CXXFLAGS) main.cpp $(files) $(LIBS) -o main

dinomain: main.cpp $(files)
	$(CXX) $(CXXFLAGS) dinomain.cpp $(files) $(LIBS) -o main

run: main
	./main

clean:
	rm -f *.o main

all:
	$(MAKE) clean
	$(MAKE) main
	$(MAKE) run

dino:
	$(MAKE) clean
	$(MAKE) dinomain
	$(MAKE) run
