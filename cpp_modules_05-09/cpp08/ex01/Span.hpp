//
// Created by viceda-s on 13/01/26.
//

#ifndef SPAN_HPP
#define SPAN_HPP

#include <vector>
#include <algorithm>
#include <stdexcept>

class Span {
private:
	unsigned int _maxSize;
	std::vector<int> _numbers;

	Span();

public:
	Span(unsigned int N);
	Span(const Span &other);
	Span &operator=(const Span &other);
	~Span();

	void addNumber(int number);
	int shortestSpan(void) const;
	int longestSpan(void) const;

	template <typename InputIterator>
	void addNumbers(InputIterator begin, InputIterator end) {
		for (InputIterator it = begin; it != end; ++it) {
			if (_numbers.size() >= _maxSize)
				throw std::out_of_range("Span is full");
			_numbers.push_back(*it);
		}
	}
};

#endif //SPAN_HPP