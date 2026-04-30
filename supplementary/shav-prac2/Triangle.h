#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Shape.h"
#include "Vector.h"

template <int n>
class Triangle: public Shape<n> {
    private:
        Vector<n> p1;
        Vector<n> p2;
        Vector<n> p3;
    public:
        Triangle(const Vector<n>& p1, const Vector<n>& p2, const Vector<n>& p3);
        Triangle(const Triangle<n>&);
        virtual Triangle<n>& operator*=(const Matrix<n,n>&);
        virtual Triangle<n>* operator*(const Matrix<n,n>&) const;
        virtual float* getPoints() const;
        virtual int getNumPoints() const;

        virtual void print() const{
            cout << "_ P1 _ " << endl;
            p1.print();
            cout << "_ P2 _ " << endl;
            p2.print();
            cout << "_ P3 _ " << endl;
            p3.print();
        }

        virtual void move(char dir); //wasd
        virtual void scale(char dir); //+-
        virtual void rotate(char dir); //lr

        virtual void midPoint(float* coordinates);

        virtual void setColour(char c);
};

#include "Triangle.cpp"

#endif /*TRIANGLE_H*/
