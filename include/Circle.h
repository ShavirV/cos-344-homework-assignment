#ifndef CIRCLE_H
#define CIRCLE_H

#include <cmath>
#include "Triangle.h"

class Circle
{
    private:
        Triangle** triangles;
        int N;
        float radius;
        float cX, cY;
        
    public:
        Circle (float cX, float cY, float radius, int N, float r, float g, float b, float a);
        ~Circle();
        void draw(GLuint shader, bool wireframe);
        void selected(bool selected);
        bool isSelected();
        bool inBounds(float dx, float dy);
        void translate(float dx, float dy);
        void rotate(float angle);
        void scale(float factor);
};

#endif