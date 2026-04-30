files = 
main: main.cpp glad.c
	g++ -g shader.cpp $(files) main.cpp glad.c -lglfw -pthread -lGLEW -ldl -lGL -o main
clean:
	rm -rf *.o main outline
	
run:
	./main
all:
	make clean
	make
	make run

outline: 
	g++ -g supplementary/outline/shader.cpp supplementary/outline/Outline.cpp supplementary/outline/Triangle.cpp supplementary/outline/Square.cpp supplementary/outline/Shape.cpp supplementary/outline/Matrix.cpp supplementary/outline/Vector.cpp -Iinclude -lglfw -pthread -lGLEW -ldl -lGL -o outline
	./outline