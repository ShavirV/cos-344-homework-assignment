#ifndef VECTOR_CPP
#define VECTOR_CPP  

#include "Vector.h"
#include <cmath>

template<int n>
Vector<n>::Vector(){
    this->arr  = new float[n];
    for (int i = 0; i < n; i++){
        arr[i] = 0.0f;
    }
}

template<int n>
Vector<n>::Vector(std::initializer_list<float> list){
    this->arr = new float[n];

    int i = 0;
    for (float val : list){
        if (i >= n) break; //ignore extra vals
        arr[i] = val;
        i++;
    }

    //fill remaining indices with 0
    while (i < n){
        arr[i] = 0.0f;
        i++;
    }
}

template<int n>
Vector<n>::Vector(float * toCopy){
    this->arr = toCopy; //shallow copy
}

template<int n>
Vector<n>::~Vector(){
    if (this->arr) delete[] arr;
}

template<int n>
Vector<n>::Vector(const Vector<n> &other){
    //deep copy
    this->arr = new float[n];

    for (int i = 0; i < n; i++){
        arr[i] = other.arr[i];
    }
}

template<int n>
Vector<n>::Vector(const Matrix<n,1> &mat){
    this->arr = new float[n];

    for (int i = 0; i < n; i++){
        this->arr[i] = mat[i][0];
    }
}

template<int n>
Vector<n>& Vector<n>::operator=(const Vector<n> &other){
    if (this == &other) return *this;

    delete[] arr;
    this->arr = new float[n];

    for (int i =  0; i < n; i++){
        this->arr[i] = other.arr[i];
    }

    return *this;
}

template<int n>
Vector<n> Vector<n>::operator+(const Vector<n> other) const{
    Vector<n> sum;
    for (int i = 0; i < n; i++){
        sum.arr[i] = this->arr[i] + other.arr[i];
    }

    return sum;
}

template<int n>
Vector<n> Vector<n>::operator-(const Vector<n> other) const{
    Vector<n> diff;
    for (int i = 0; i < n; i++){
        diff.arr[i] = this->arr[i] - other.arr[i];
    }

    return diff;
}

//scalar product
template<int n>
Vector<n> Vector<n>::operator*(const float scalar) const{
    Vector<n> prod;
    for (int i = 0; i < n; i++){
        prod.arr[i] = this->arr[i] * scalar;
    }

    return prod;
}

//dot product
template<int n>
float Vector<n>::operator*(const Vector<n> other) const{
    float prod = 0;
    for (int i = 0; i < n; i++){
        prod += this->arr[i] * other.arr[i];
    }
    return prod;
}

template<int n>
float Vector<n>::magnitude() const{
    float sum = 0;
    for (int i = 0; i < n; i++){
        sum += this->arr[i] * this->arr[i];
    } 

    return std::sqrt(sum);
}

template<int n>
Vector<n>::operator Matrix<n,1>() const{
    //create a matrix from the passed in vector
    Matrix<n,1> mat;
    for (int i = 0; i < n; i++){
        mat[i][0] = this->arr[i];
    }
    return mat;
}

template<>
inline Vector<3> Vector<3>::crossProduct(const Vector<3> other) const{
    //a1 a2 a3
    //b1 b2 b3
    Vector<3> prod;
    float a1 = this->arr[0];
    float a2 = this->arr[1];
    float a3 = this->arr[2];
    float b1 = other.arr[0];
    float b2 = other.arr[1];
    float b3 = other.arr[2];

    prod.arr[0] = a2*b3 - a3*b2;
    prod.arr[1] = a3*b1 - a1*b3;
    prod.arr[2] = a1*b2 - a2*b1;

    return prod;
}

template<int n>
Vector<n> Vector<n>::unitVector() const{
    float magnitude = this->magnitude();

    if (magnitude == 0){
        throw std::runtime_error("Invalid unit vector");
    }

    //divide all indices by magnitude
    Vector<n> unit;

    for (int i = 0; i < n; i++){
        unit.arr[i] = this->arr[i] / magnitude;
    }

    return unit;
}

template<int n>
int Vector<n>::getN() const{
 return n;
}

#endif 