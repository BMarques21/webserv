//
// Created by viceda-s on 04/01/2026.
//

#include "MutantStack.hpp"
#include <iostream>
#include <string>
#include <list>

int main(void) {
	// Subject example
	std::cout << "=== MutantStack Test ===" << std::endl;
	MutantStack<int> mstack;

	mstack.push(5);
	mstack.push(17);

	std::cout << mstack.top() << std::endl;

	mstack.pop();

	std::cout << mstack.size() << std::endl;

	mstack.push(3);
	mstack.push(5);
	mstack.push(737);
	mstack.push(0);

	MutantStack<int>::iterator it = mstack.begin();
	MutantStack<int>::iterator ite = mstack.end();

	++it;
	--it;
	while (it != ite) {
		std::cout << *it << std::endl;
		++it;
	}

	// Test conversion to std::stack
	std::cout << "\n=== Conversion to std::stack ===" << std::endl;
	std::stack<int> s(mstack);
	std::cout << "Stack size: " << s.size() << std::endl;

	// Compare with std::list (same output)
	std::cout << "\n=== Same Test with std::list ===" << std::endl;
	std::list<int> myList;

	myList.push_back(5);
	myList.push_back(17);
	myList.pop_front();
	myList.push_back(3);
	myList.push_back(5);
	myList.push_back(737);
	myList.push_back(0);

	std::list<int>::iterator lit = myList.begin();
	std::list<int>::iterator lite = myList.end();

	++lit;
	--lit;
	while (lit != lite) {
		std::cout << *lit << std::endl;
		++lit;
	}

	// Test reverse iterators
	std::cout << "\n=== Reverse Iterator Test ===" << std::endl;
	MutantStack<int> mstack2;
	mstack2.push(1);
	mstack2.push(2);
	mstack2.push(3);
	mstack2.push(4);
	mstack2.push(5);

	std::cout << "Forward: ";
	for (MutantStack<int>::iterator it2 = mstack2.begin(); it2 != mstack2.end(); ++it2)
		std::cout << *it2 << " ";
	std::cout << std::endl;

	std::cout << "Reverse: ";
	for (MutantStack<int>::reverse_iterator rit = mstack2.rbegin(); rit != mstack2.rend(); ++rit)
		std::cout << *rit << " ";
	std::cout << std::endl;

	// Test with other types
	std::cout << "\n=== Test with std::string ===" << std::endl;
	MutantStack<std::string> stringStack;
	stringStack.push("Hello");
	stringStack.push("World");
	stringStack.push("!");

	for (MutantStack<std::string>::iterator sit = stringStack.begin(); sit != stringStack.end(); ++sit)
		std::cout << *sit << " ";
	std::cout << std::endl;

	return 0;
}