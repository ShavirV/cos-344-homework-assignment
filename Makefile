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
	make clean
	g++ -g supplementary/outline/shader.cpp supplementary/outline/Outline.cpp supplementary/outline/Triangle.cpp supplementary/outline/Circle.cpp supplementary/outline/Rectangle.cpp -Iinclude -lglfw -pthread -lGLEW -ldl -lGL -o outline
	./outline