//
// Created by viceda-s on 04/01/2026.
//

#include "Span.hpp"
#include <algorithm>
#include <climits>
#include <cmath>

Span::Span(unsigned int N) : _maxSize(N) {
}

Span::Span(const Span &other) : _maxSize(other._maxSize), _numbers(other._numbers) {
}

Span &Span::operator=(const Span &other) {
    if (this != &other) {
        _maxSize = other._maxSize;
        _numbers = other._numbers;
    }
    return *this;
}

Span::~Span() {
}

void Span::addNumber(int number) {
    if (_numbers.size() >= _maxSize)
        throw std::out_of_range("Span is full");
    _numbers.push_back(number);
}

int Span::shortestSpan(void) const {
    if (_numbers.size() < 2)
        throw std::logic_error("Not enough numbers to find a span");

    std::vector<int> sorted = _numbers;
    std::sort(sorted.begin(), sorted.end());

    int shortest = INT_MAX;
    for (size_t i = 0; i < sorted.size() - 1; ++i) {
        int diff = sorted[i + 1] - sorted[i];
        if (diff < shortest)
            shortest = diff;
    }
    return shortest;
}

int Span::longestSpan(void) const {
    if (_numbers.size() < 2)
        throw std::logic_error("Not enough numbers to find a span");

    int min = *std::min_element(_numbers.begin(), _numbers.end());
    int max = *std::max_element(_numbers.begin(), _numbers.end());

    return max - min;
}