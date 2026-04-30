#ifndef SHAPE_H
#define SHAPE_H

#include "Matrix.h"

#include <iostream>

template <int n>
class Shape{
    public:
        virtual ~Shape() {}
        virtual Shape& operator*=(const Matrix<n,n>&) =0;
        virtual Shape* operator*(const Matrix<n,n>&) const =0;
        virtual float* getPoints() const =0;
        virtual int getNumPoints() const =0;
        virtual void print() const =0;

        virtual void select() {selected = true; setColour('s');} //1-4, selected
        virtual void deselect() {selected = false; setColour('o');} //original
        virtual void move(char dir) =0; //wasd
        virtual void scale(char dir) =0; //+-
        virtual void rotate(char dir) =0; //lr

        virtual void midPoint(float* coordinates) =0;

        virtual void setColour(char c) =0;

        bool selected = false;
        std::string colour = "";
};

#endif /*SHAPE_H*/