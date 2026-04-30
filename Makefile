files = 
main: main.cpp glad.c
	g++ -g shader.cpp $(files) main.cpp glad.c -lglfw -pthread -lGLEW -ldl -lGL -o main
clean:
	rm -f *.o main
run:
	./main
all:
	make clean
	make
	make run

outline: 
	g++ -g src/core/shader.cpp src/objects/Triangle.cpp src/objects/Rectangle.cpp src/objects/Circle.cpp src/scene/Outline.cpp -Iinclude -lglfw -pthread -lGLEW -ldl -lGL -o outline
	./outline