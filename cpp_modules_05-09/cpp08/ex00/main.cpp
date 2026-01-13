//
// Created by viceda-s on 13/01/2026.
//

#include "easyfind.hpp"
#include <iostream>
#include <vector>
#include <list>

int main(void) {
	// Test with vector
	std::vector<int> vec;
	vec.push_back(1);
	vec.push_back(5);
	vec.push_back(42);
	vec.push_back(10);
	vec.push_back(3);

	std::cout << "Testing with vector..." << std::endl;
	try {
		std::vector<int>::iterator it = easyfind(vec, 42);
		std::cout << "Found value: " << *it << std::endl;
	} catch (const std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

	try {
		std::vector<int>::iterator it = easyfind(vec, 99);
		std::cout << "Found value: " << *it << std::endl;
	} catch (const std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

	// Test with list
	std::list<int> lst;
	lst.push_back(2);
	lst.push_back(8);
	lst.push_back(15);
	lst.push_back(7);

	std::cout << "\nTesting with list..." << std::endl;
	try {
		std::list<int>::iterator it = easyfind(lst, 15);
		std::cout << "Found value: " << *it << std::endl;
	} catch (const std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

	try {
		std::list<int>::iterator it = easyfind(lst, 100);
		std::cout << "Found value: " << *it << std::endl;
	} catch (const std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

	return 0;
}