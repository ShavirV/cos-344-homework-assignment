#ifndef REGPOLY_H
#define REGPOLY_H

/**
A regular n sided polygon, taking in a template parameter n for the vector space it lives in

Designed to work with GL_TRIANGLE_FAN 

sides is defined at runtime

usage for circles, smooth: 60 sided, centre (0,0,1), rad 0.15
low poly circle, octagon: 60 sided, centre

vertices: for a poly with s sides, centre C(x,y), and rad r:

vertex i is found at (x, y):
x = C.x + r * cos(startAngle + i * 2pi/s)
y = C.y + r * sin(startAngle + i * 2pi/s)

where startAnfle = -pi/2 to orient vertex[1] at the top, for consistency

 */

#include "Shape.h"
#include "Matrix.h"
#include "Vector.h"

#include <vector>
#include <cmath>
#include <iostream>
 
using namespace std;
 
#ifndef PI
#define PI 3.14159265358979323846f
#endif

template <int n>
class Polygon : public Shape<n>{
private:
    //vector containing our custom Vector class
    //vertices[0] = centre point so no need to rederive
    //vertices[1-n] = actual points on the peremiter
    //vertices[n+1] = copy of first point to close the shape
    std::vector<Vector<n>> vertices; 
    int sides;
    float radius;

public:

    Polygon(const Vector<n>& centre, float rad, int sides, float startAngle = -(float)PI/2.0f){
        this->sides = sides;
        this->radius = rad;

        //allocate memory 
        vertices.reserve(sides+2);

        vertices.push_back(centre);

        //add the peremiter vertices
        float step = 2.0f * (float)PI / (float)sides;
        for (int i = 0; i < sides; i++){
            float angle = startAngle + (i*step);

            Vector<n> v = centre;
            //calc x and y coords for ith vertex
            v[0] = centre[0] + radius * std::cos(angle);
            v[1] = centre[1] + radius * std::sin(angle);
            vertices.push_back(v);
        }

        //push a clone of the first one
        vertices.push_back(vertices[1]);
    }

    Polygon(const Polygon<n>& other)
        : sides(other.sides), radius(other.radius), vertices(other.vertices){}

    //apply a transformation matrix to the polygon
    virtual Polygon<n>& operator*=(const Matrix<n,n>& transform) override {
        for (auto& v : vertices) {
            v = transform * Matrix<n,1>(v);
        }
        return *this;
    }

    virtual Polygon<n>* operator*(const Matrix<n,n>& transform) const override {
        Polygon<n>* temp = new Polygon<n>(*this);
        *temp *= transform;
        return temp;
    }

    //utilities
    virtual float* getPoints() const override {
        int total = (int)vertices.size() * n;  //(sides+2) * n
        float* points = new float[total];
        int index = 0;
        for (const auto& v : vertices) {
            for (int i = 0; i < n; i++) {
                points[index++] = v[i];
            }
        }
        return points;
    }

    virtual int getNumPoints() const override {
        return (int)vertices.size() * n;
    }

    //use for glDrawArrays count
    int getVertexCount() const {
        return (int)vertices.size();  //sides + 2
    }

    int getSides() const { return sides; }

    ///ACTUAL PRAC 2 FUNCTIONS
    
    //already stored in the thingy
    virtual void midPoint(float* coordinates) override{
        for (int i = 0; i < n; i++)
            coordinates[i] = vertices[0][i];
    }

    //selection already in shape.h


    //move translated the polygon in 2D, building a homogenous translation matrix
    // [1 0 dx]
    // [0 1 dy]
    // [0 0  1]
    // then *= this matrix
    // w=up, s=down, a=left, d=right
    virtual void move(char dir) override{
        const float STEP = 0.05f; ///adjust as needed
        float dx = 0.0f, dy = 0.0f;

        //decide how to move
        switch (dir) {
            case 'w': dy =  STEP; break;
            case 's': dy = -STEP; break;
            case 'a': dx = -STEP; break;
            case 'd': dx =  STEP; break;
            default: return; //not our problem
        }

        //build the transformation matrix
            // [1 0 dx]
            // [0 1 dy]
            // [0 0  1]
        Matrix<n,n> T = Matrix<n,n>::identity();
        T[0][2] = dx;
        T[1][2] = dy;

        *this *= T;
    }

    //scale the polygon uniformly AROUND ITS CENTRE
    virtual void scale(char dir) override {
        const float FACTOR_UP = 1.15f;
        const float FACTOR_DOWN = 1.0f / FACTOR_UP;
        float s = (dir == '+') ? FACTOR_UP : FACTOR_DOWN;

        //get the centre and transform
        float cx = vertices[0][0];
        float cy = vertices[0][1];

        //T(-centre): translate centre to origin
        Matrix<n,n> Tneg = Matrix<n,n>::identity() = Matrix<n,n>::identity();
        Tneg[0][2] = -cx;
        Tneg[1][2] = -cy;

        //S(s): uniform scale
        Matrix<n,n> S = Matrix<n,n>::identity();
        S[0][0] = s;
        S[1][1] = s;

        //T(+centre): translate back
        Matrix<n,n> Tpos = Matrix<n,n>::identity();
        Tpos[0][2] = cx;
        Tpos[1][2] = cy;

        //Combined: Tpos * S * Tneg
        //Apply right-to-left: first Tneg, then S, then Tpos
        *this *= Tneg;
        *this *= S;
        *this *= Tpos;

        //this should happen instantaneoulsy when rendered, thus the translation should not be visible
    }

    virtual void rotate(char dir) override {
        const float ANGLE_STEP = 0.1f; //in radians, about 6 degrees
        float angle = (dir == 'l') ? ANGLE_STEP : -ANGLE_STEP;

        float cx = vertices[0][0];
        float cy = vertices[0][1];
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        //same as scaling pretty much, just apply Cosa and Sina instead

        //T(-centre)
        Matrix<n,n> Tneg = Matrix<n,n>::identity();
        Tneg[0][2] = -cx;
        Tneg[1][2] = -cy;

        //R(angle)
        Matrix<n,n> R = Matrix<n,n>::identity();
        R[0][0] =  cosA;  R[0][1] = -sinA;
        R[1][0] =  sinA;  R[1][1] =  cosA;

        //T(+centre)
        Matrix<n,n> Tpos = Matrix<n,n>::identity();
        Tpos[0][2] = cx;
        Tpos[1][2] = cy;

        *this *= Tneg;
        *this *= R;
        *this *= Tpos;
    }

    virtual void print() const override {
        cout << "Polygon(" << sides << " sides, "
             << vertices.size() << " stored vertices)" << endl;
        cout << "  Centre:  "; vertices[0].print();
        for (int i = 1; i <= sides; i++) {
            cout << "  Vertex " << i << ": "; vertices[i].print();
        }
    }

    virtual void setColour(char c) override {
        this->colour = std::string(1, c);
    } 




};


#endif