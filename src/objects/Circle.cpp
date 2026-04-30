#include "../../include/Circle.h"

Circle::Circle (float cX, float cY, float radius, int N, float r, float g, float b, float a)
{
    this->N = N;
    this->cX = cX;
    this->cY = cY;
    this->radius = radius;

    this->triangles = new Triangle*[N];

    for (int i =0; i< N; i++)
    {
        float angle1 = 2.0f * M_PI * i / N;
        float angle2 = 2.0f * M_PI * (i+1) / N;

        float x2 = cX + radius * cos(angle1);
        float y2 = cY + radius * sin(angle1);

        float x3 = cX + radius * cos(angle2);
        float y3 = cY + radius * sin(angle2);

        triangles[i] = new Triangle(cX, cY, x2, y2, x3, y3, r, g, b ,a);
    }
}

Circle::~Circle()
{
    for (int i =0; i< N; i++)
    {
        delete triangles[i];
    }
    delete [] triangles;
}

void Circle::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i =0; i<N; i++)
    {
        triangles[i]->draw(shaderProgram, wireframe);
    }
}

void Circle::selected(bool selected)
{
    for (int i =0; i< N; i++)
    {
        triangles[i]->selected(selected);
    }
}

bool Circle::isSelected()
{
    for (int i =0; i< N; i++)
    {
        if (!triangles[i]->isSelected())
        {
            return false;
        }
    }

    return true;
}

bool Circle::inBounds(float dx, float dy)
{
    for (int i =0; i< N; i++)
    {
        if (!triangles[i]->inBounds(dx,dy))
        {
            return false;
        }
    }

    return true; 
}

void Circle::translate(float dx, float dy)
{
    if (!inBounds(dx, dy))
        return;

    cX += dx;
    cY += dy;
    for (int i = 0; i < N; i++)
        triangles[i]->translate(dx, dy);
}

void Circle::rotate(float angle)
{
    for (int i = 0; i < N; i++)
        triangles[i]->rotateAround(angle, cX, cY);
}

void Circle::scale(float factor)
{
    for (int i = 0; i < N; i++)
        triangles[i]->scaleAround(factor, cX, cY);
}