#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <GL/glew.h>

#include <cmath>

class Rectangle;
class Circle;

class Triangle 
{
    friend class Rectangle;
    friend class Circle;

    private:
        GLuint VAO; //vertex array object
        GLuint VBO; //vertex buffer object

        float vertices [6]; // xy xy xy coords

        float r,g, b, a; //rgba values

        float cx, cy;

        bool select = false;

        void setRGBA(float r, float g, float b, float a); // colors are set directly so these values become new colors

    public:
        Triangle (float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a);
        ~Triangle();
        void draw (GLuint shader, bool wireframe);
        void selected(bool selected);
        bool isSelected();
        void translate(float x, float y);
        bool inBounds (float dx, float dy);
        void rotateAround (float angle, float cx, float cy);
        void rotate(float angle);
        void scaleAround(float factor, float cx, float cy);
        void scale(float factor);
        
};

#endif