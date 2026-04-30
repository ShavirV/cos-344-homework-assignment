#ifndef SQUARE_H
#define SQUARE_H

#include "Shape.h"
#include "Matrix.h"
#include "Vector.h"

template <int n>
class Square: public Shape<n> {
    private:
        Vector<n> tl;
        Vector<n> tr;
        Vector<n> br;
        Vector<n> bl;
    public:
        Square(const Vector<n>& center, float height, float width);
        Square(const Vector<n>& tl, const Vector<n>& tr, const Vector<n>& br, const Vector<n>& bl);
        Square(const Square<n>&);
        virtual Square<n>& operator*=(const Matrix<n,n>&);
        virtual Square<n>* operator*(const Matrix<n,n>&) const;
        virtual float* getPoints() const;
        virtual int getNumPoints() const;

        virtual void print() const{
            cout << "_ P1 _ " << endl;
            tl.print();
            cout << "_ P2 _ " << endl;
            tr.print();
            cout << "_ P3 _ " << endl;
            br.print();
            cout << "_ P4 _ " << endl;
            bl.print();
        }

        virtual void midPoint(float* coordinates) override;
        virtual void setColour(char c) override;
        virtual void move(char dir) override;
        virtual void scale(char dir) override;
        virtual void rotate(char dir) override;
};

#include "Square.cpp"

#endif /*SQUARE_H*/
