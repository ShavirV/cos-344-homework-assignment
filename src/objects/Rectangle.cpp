#include "../../include/Rectangle.h"

Rectangle::Rectangle(float brX, float brY, float trX, float trY, float tlX, float tlY, float blX, float blY, float r, float g, float b, float a)
{
    cx = (brX + trX + tlX + blX) / 4.0f;
    cy = (brY + trY + tlY + blY) / 4.0f;

    this->tr = new Triangle(brX, brY, trX, trY, tlX, tlY, r, g, b, a);
    this->tl = new Triangle(blX, blY, tlX, tlY, brX, brY, r, g, b, a);
}

Rectangle::~Rectangle()
{
    delete tl;
    delete tr;
}

void Rectangle::draw(GLuint shaderProgram, bool wireframe)
{
    tl->draw(shaderProgram, wireframe);
    tr->draw(shaderProgram, wireframe);
}

void Rectangle::selected(bool selected)
{
    tl->selected(selected);
    tr->selected(selected);
}

bool Rectangle::isSelected()
{
    return tl->isSelected() && tr->isSelected(); // they should always be the same so if one is false we have problems
}

bool Rectangle::inBounds(float dx, float dy)
{
    return tl->inBounds(dx, dy) && tr->inBounds(dx, dy);
}

void Rectangle::translate(float dx, float dy)
{
    if (!inBounds(dx, dy))
        return;

    cx += dx;
    cy += dy;
    tl->translate(dx, dy);
    tr->translate(dx, dy);
}

void Rectangle::rotate(float angle)
{
    tl->rotateAround(angle, cx, cy);
    tr->rotateAround(angle, cx, cy);
}

void Rectangle::scale(float factor)
{
    tl->scaleAround(factor, cx, cy);
    tr->scaleAround(factor, cx, cy);
}