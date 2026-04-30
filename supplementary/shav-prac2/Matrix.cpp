#ifndef MATRIX_CPP
#define MATRIX_CPP

#include "Matrix.h"

template<int n, int m>
Matrix<n,m>::Matrix(){
    this->arr = new float*[n];

    for (int i = 0; i < n; i++){
        arr[i] = new float[m];
        for (int j = 0; j  < m; j++){
            arr[i][j] = 0.0f;
        }
    }
}

template<int n, int m>
Matrix<n,m>::Matrix(float** other){
    this->arr = new float*[n];

    for (int i = 0; i < n; i++){
        this->arr[i] = new float[m];
        for (int j = 0; j < m; j++){
            this->arr[i][j] = other[i][j];
        }
    }
}

template<int n, int m>
Matrix<n,m>::Matrix(const Matrix<n,m> &other){
    //deep copy
    this->arr = new float*[n];

    for (int i = 0; i < n; i++){
        this->arr[i] = new float[m];
        for (int j = 0; j < m; j++){
            this->arr[i][j] = other.arr[i][j];
        }
    }
}

template<int n, int m>
Matrix<n,m>::~Matrix(){
    if (this->arr){
        for (int i = 0; i < n; i++){
            if (this->arr[i]){
                delete[] arr[i];
            }
        }
        delete[] arr;
    }
}

template<int n, int m>
Matrix<n,m>& Matrix<n,m>::operator=(const Matrix<n,m> &other){
    if (this == &other) return *this;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            this->arr[i][j] = other.arr[i][j];
        }
    }

    return *this;
}

template<int n, int m> //i have no idea how this one works
template<int a>
Matrix<n,a> Matrix<n,m>::operator*(const Matrix<m,a> other) const{
    Matrix<n,a> result;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < a; j++){
            result[i][j] = 0.0f;
            for (int k = 0; k < m; k++){
                result[i][j] += arr[i][k] * other [k][j];
            }
        }
    }
    
    return result;
}

template<int n, int m>
Matrix<n,m> Matrix<n,m>::operator*(const float scalar) const{
    Matrix<n,m> prod;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            prod.arr[i][j] = this->arr[i][j] * scalar;
        }
    }
    
    return prod;
}

template<int n, int m>
Matrix<n,m> Matrix<n,m>::operator+(const Matrix<n,m> other) const{
    Matrix<n,m> sum;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            sum.arr[i][j] = this->arr[i][j] + other.arr[i][j];
        }
    }

    return sum;
}

template<int n, int m>
Matrix<m,n> Matrix<n,m>::operator~() const{
    //transpose
    Matrix<m,n> transpose;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            transpose[j][i] = this->arr[i][j];
        }
    }

    return transpose;
}   

template<int n, int m>
int Matrix<n,m>::getM() const{
    return m;
}

template<int n, int m>
int Matrix<n,m>::getN() const{
    return n;
}

template<int n, int m>
float Matrix<n,m>::determinant() const{
    if (m != n){
        throw std::runtime_error("Matrix is not square");
    }

    //gaussisan elimination 
    Matrix<n,n> temp = *this;
    float det = 1.0f;
    int swapCount = 0;

    for (int i = 0; i < n; i++){
        //find pivot
        int pivot = i;
        for (int row = i+1; row < n; row++){
            if (std::fabs(temp[row][i]) > std::fabs(temp[pivot][i])){
                pivot = row;
            }
        }

        //if the pivot is zero, det is 0
        if (std::fabs(temp[pivot][i]) < 1e-6f){ //tolerence point for float, 0.00...1f, basically 0
            return 0.0f;
        }

        //swap rows if needed
        if (pivot != i){
            std::swap(temp.arr[i], temp.arr[pivot]);
            swapCount++;
        }

        //eliminate below
        for (int row = i+1; row < n; row++){
            float factor = temp[row][i] / temp[i][i];
            for (int col = i; col < n; col++){
                temp[row][col] -= factor * temp[i][col];
            }
        }
    }

    //multiply the diagonal
    for (int i = 0; i < n; i++){
        det *= temp[i][i];
    }

    if (swapCount % 2 != 0){
        det = -det;
    }
    
    return det;
}

#endif

//idea for laplace expansion
//recursive function
//decompose into smaller submatrices
//use the cofactor or whatever it is 
//base case, 2x2 matrix? then calculate det of that manually

//need a function that takes in indices to block out, and if i==blockx or j==blocky, skip
//when copying over indices
//this is some fuckass O(n!) type solution