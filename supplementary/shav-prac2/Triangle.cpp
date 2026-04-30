#ifndef TRIANGLE_CPP
#define TRIANGLE_CPP

#include "Triangle.h"

template<int n>
Triangle<n>::Triangle(const Vector<n>& p1, const Vector<n>& p2, const Vector<n>& p3){
    this->p1 = p1;
    this->p2 = p2;
    this->p3 = p3;
}

template<int n>
Triangle<n>::Triangle(const Triangle<n>&other){
    this->p1 = other.p1;
    this->p2 = other.p2;
    this->p3 = other.p3;
}

template<int n>
Triangle<n>& Triangle<n>::operator*=(const Matrix<n,n>&transform){
    p1 = transform * Matrix<n,1>(p1);
    p2 = transform * Matrix<n,1>(p2);
    p3 = transform * Matrix<n,1>(p3);
    
    return *this;
}

template<int n>
Triangle<n>* Triangle<n>::operator*(const Matrix<n,n>&transform) const{
    Triangle<n>* temp = new Triangle(*this);
    *temp *= transform;
    return temp;
}

template<int n>
float* Triangle<n>::getPoints() const{

    float* points = new float[3 * n];

    int index = 0;

    for (int i = 0; i < n; i++){
        points[index++] = p1[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = p2[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = p3[i];
    }

    return points;
}

template<int n>
int Triangle<n>::getNumPoints() const{
    return 3*n;
}

template <int n>
void Triangle<n>::midPoint(float* coordinates) {
    for (int i = 0; i < n; i++)
        coordinates[i] = (p1[i] + p2[i] + p3[i]) / 3.0f;
}
 
// ------------------------------------------------------------------
// setColour
// ------------------------------------------------------------------
template <int n>
void Triangle<n>::setColour(char c) {
    this->colour = std::string(1, c);
}
 
template <int n>
void Triangle<n>::move(char dir) {
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
void Triangle<n>::scale(char dir) {
    const float UP   = 1.15f;
    const float DOWN = 1.0f / UP;
    float s = (dir == '+') ? UP : DOWN;
 
    //compute centroid as pivot
    float cx = (p1[0] + p2[0] + p3[0]) / 3.0f;
    float cy = (p1[1] + p2[1] + p3[1]) / 3.0f;
 
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
void Triangle<n>::rotate(char dir) {
    const float STEP = 0.1f;
    float angle = (dir == 'l') ? STEP : -STEP;
 
    // Centroid as pivot
    float cx = (p1[0] + p2[0] + p3[0]) / 3.0f;
    float cy = (p1[1] + p2[1] + p3[1]) / 3.0f;
 
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