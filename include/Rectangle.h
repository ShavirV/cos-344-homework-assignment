#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "Triangle.h"


class Rectangle
{

    private:
        Triangle* tl;
        Triangle* tr;

        float cx, cy;

    public: 
        Rectangle(float brX, float brY, float trX, float trY, float tlX, float tlY, float blX, float blY, float r, float g, float b, float a);
        ~Rectangle();
        void draw(GLuint shaderProgram, bool wireframe);
        void selected(bool selected);
        bool isSelected();
        bool inBounds(float dx, float dy);
        void translate(float x, float y);
        void rotate(float angle);
        void scale(float factor);
        
};

#endif

