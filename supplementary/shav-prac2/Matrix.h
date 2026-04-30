#ifndef MATRIX_H
#define MATRIX_H

#include <iostream>
#include <cmath>
#include <iomanip>

#include "Vector.h"

template <int n>
class Vector;

template<int n, int m>
class Matrix
{
protected:
    float **arr;

public:
    Matrix();
    Matrix(float **);
    Matrix(const Matrix<n,m> &);
    virtual ~Matrix();
    float *&operator[](int index) const
    {
        if (index >= n || index < 0)
        {
            throw "Invalid index";
        }

        return arr[index];
    }

    
    Matrix<n,m>& operator=(const Matrix<n,m>&);
    template<int a>
    Matrix<n,a> operator*(const Matrix<m,a>) const;
    //PRAC2
    Matrix<n,m>& operator*=(const Matrix<m,m>& other) {
    *this = *this * other;
    return *this;
    }
    Matrix<n,m> operator*(const float) const;
    Matrix<n,m> operator+(const Matrix<n,m>) const;
    Matrix<m,n> operator~() const;
    void print() const
    {
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < m; j++)
            {
                std::cout << (*this)[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
    int getM() const;
    int getN() const;

    float determinant() const;

    //PRAC2 
    static Matrix<n,n> identity(){
        Matrix<n,n> identity;
        for (int i = 0; i < n; i++){
            identity[i][i] = 1;
        }
        return identity;
    }

    
};

#include "Matrix.cpp"

#endif /*MATRIX_H*/