//
// Created by viceda-s on 03/01/2026.
//

#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <stdexcept>

template <typename T>
class Array
{
private:
    T *_data;
    unsigned int _size;

public:
    Array() : _data(NULL), _size(0) { }

    // Constructor with size parameter: creates array with n elements
    // Elements are initialized by default (value-initialized)
    Array(unsigned int n) : _size(n) {
        _data = new T[n]();
    }

    // Copy constructor: deep copy
    Array(const Array &other) : _size(other._size) {
        _data = new T[_size];
        for (unsigned int i = 0; i < _size; i++)
            _data[i] = other._data[i];
    }

    // Assignment operator: deep copy
    Array &operator=(const Array &other) {
        if (this != &other)
        {
            delete[] _data;
            _size = other._size;
            _data = new T[_size];
            for (unsigned int i = 0; i < _size; i++)
                _data[i] = other._data[i];
        }
        return *this;
    }

    // Destructor
    ~Array() {
        delete[] _data;
    }

    // Subscript operator - non-const version
    // Throws std::out_of_range exception if index is out of bounds
    T &operator[](unsigned int index) {
        if (index >= _size)
            throw std::out_of_range("Index out of bounds");
        return _data[index];
    }

    // Subscript operator - const version
    const T &operator[](unsigned int index) const {
        if (index >= _size)
            throw std::out_of_range("Index out of bounds");
        return _data[index];
    }

    // Size member function: returns the number of elements
    unsigned int size(void) const {
        return _size;
    }
};

#endif