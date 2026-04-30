#ifndef SQUARE_CPP
#define SQUARE_CPP

#include "Square.h"

template <int n>
Square<n>::Square(const Vector<n>& center, float height, float width){
    //copy center and apply calculations to move in 2d
    tl = center;
    tl[0] -= width/2;
    tl[1] += height/2;

    tr = center; 
    tr[0] += width/2;
    tr[1] += height/2;

    bl = center; 
    bl[0] -= width/2;
    bl[1] -= height/2;
    
    br = center; 
    br[0] += width/2;
    br[1] -= height/2;
}

template <int n>
Square<n>::Square(const Vector<n>& tl, const Vector<n>& tr, const Vector<n>& br, const Vector<n>& bl){
    this->tl = tl;
    this->tr = tr;
    this->br = br;
    this->bl = bl;
}

template <int n>
Square<n>::Square(const Square<n>&other){
    this->tl = other.tl;
    this->tr = other.tr;
    this->br = other.br;
    this->bl = other.bl;
}

template <int n>
Square<n>& Square<n>::operator*=(const Matrix<n,n>&transform){
    //perform matrix multipliation on each of the vectors

    //convert to <n,1> matrix, multiply, then convert back to vector
    tl = transform * Matrix<n,1>(tl);
    tr = transform * Matrix<n,1>(tr);
    bl = transform * Matrix<n,1>(bl);
    br = transform * Matrix<n,1>(br);

    return *this;
}

template <int n>
Square<n>* Square<n>::operator*(const Matrix<n,n>&transform) const{
    Square<n>* temp = new Square(*this);
    *temp *= transform;
    return temp;
}

template <int n>
float* Square<n>::getPoints() const{
    float* points = new float[4 * n];

    int index = 0;

    for (int i = 0; i < n; i++){
        points[index++] = tl[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = tr[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = br[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = bl[i];
    }

    return points;
}

template <int n>
int Square<n>::getNumPoints() const{
    return n*4;
}

template <int n>
void Square<n>::midPoint(float* coordinates) {
    for (int i = 0; i < n; i++)
        coordinates[i] = (tl[i] + tr[i] + br[i] + bl[i]) / 4.0f;
}
 
template <int n>
void Square<n>::setColour(char c) {
    this->colour = std::string(1, c);
}
 
template <int n>
void Square<n>::move(char dir) {
    const float STEP = 0.05f;
    float dx = 0.0f, dy = 0.0f;
    switch (dir) {
        case 'w': dy =  STEP; break;
        case 's': dy = -STEP; break;
        case 'a': dx = -STEP; break;
        case 'd': dx =  STEP; break;
        default: return;
    }
 
    Matrix<n,n> T = Matrix<n,n>::identity();
    T[0][2] = dx;
    T[1][2] = dy;
    *this *= T;
}
 
template <int n>
void Square<n>::scale(char dir) {
    const float UP   = 1.15f;
    const float DOWN = 1.0f / UP;
    float s = (dir == '+') ? UP : DOWN;
 
    // Centroid of the 4 corners
    float cx = (tl[0] + tr[0] + br[0] + bl[0]) / 4.0f;
    float cy = (tl[1] + tr[1] + br[1] + bl[1]) / 4.0f;
 
    Matrix<n,n> Tneg = Matrix<n,n>::identity(); 
    Tneg[0][2] = -cx; 
    Tneg[1][2] = -cy;

    Matrix<n,n> S = Matrix<n,n>::identity();    
    S[0][0] = s;     
    S[1][1] = s;

    Matrix<n,n> Tpos = Matrix<n,n>::identity(); 
    Tpos[0][2] = cx;  
    Tpos[1][2] = cy;
 
    *this *= Tneg;
    *this *= S;
    *this *= Tpos;
}
 
template <int n>
void Square<n>::rotate(char dir) {
    const float STEP = 0.1f;  // radians
    float angle = (dir == 'l') ? STEP : -STEP;
 
    float cx = (tl[0] + tr[0] + br[0] + bl[0]) / 4.0f;
    float cy = (tl[1] + tr[1] + br[1] + bl[1]) / 4.0f;
 
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
 
    Matrix<n,n> Tneg = Matrix<n,n>::identity();
    Tneg[0][2] = -cx; 
    Tneg[1][2] = -cy;
 
    Matrix<n,n> R = Matrix<n,n>::identity();
    R[0][0] =  cosA;  R[0][1] = -sinA;
    R[1][0] =  sinA;  R[1][1] =  cosA;
 
    Matrix<n,n> Tpos = Matrix<n,n>::identity(); 
    Tpos[0][2] = cx; 
    Tpos[1][2] = cy;
 
    *this *= Tneg;
    *this *= R;
    *this *= Tpos;
}
 

#endif