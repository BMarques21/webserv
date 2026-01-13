//
// Created by viceda-s on 13/01/26.
//

#include "MutantStack.hpp"
#include <iostream>
#include <string>
#include <list>

int main() {
    std::cout << "### MutantStack Test ###" << std::endl;
    MutantStack<int> mstack;

    mstack.push(5);
    mstack.push(17);

    std::cout << "Size before pop: " << mstack.size() << std::endl;
    std::cout << "Top element before pop: " << mstack.top() << std::endl;
    mstack.pop();
    std::cout << "\nSize after pop: " << mstack.size() << std::endl;
    std::cout << "Top element after pop: " << mstack.top() << std::endl;
    std::cout << std::endl;

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
    std::cout << "Size after push: " << mstack.size() << std::endl;

    std::cout << "\nConvert to std::stack..." << std::endl;
    std::stack<int> s(mstack);
    std::cout << "Stack size: " << s.size() << std::endl;

    std::cout << "\nSame test with std::list..." << std::endl;
    std::list<int> myLst;

    myLst.push_back(5);
    myLst.push_back(17);

    std::cout << "List Size before pop: " << myLst.size() << std::endl;
    std::cout << "Top element before pop: " << myLst.back() << std::endl;
    myLst.pop_back();
    std::cout << "\nList Size after pop: " << myLst.size() << std::endl;
    std::cout << "Top element after pop: " << myLst.back() << std::endl;
    std::cout << std::endl;

    myLst.push_back(3);
    myLst.push_back(5);
    myLst.push_back(737);
    myLst.push_back(0);

    std::list<int>::iterator lit = myLst.begin();
    std::list<int>::iterator lite = myLst.end();

    ++lit;
    --lit;

    while (lit != lite) {
        std::cout << *lit << std::endl;
        ++lit;
    }
    std::cout << "List Size after push: " << myLst.size() << std::endl;

    std::cout << "\nReverse iterator test..." << std::endl;
    MutantStack<int> mstack2;
    mstack2.push(10);
    mstack2.push(20);
    mstack2.push(30);
    mstack2.push(40);
    mstack2.push(50);

    std::cout << "Forward: ";
    for (MutantStack<int>::iterator it2 = mstack2.begin(); it2 != mstack2.end(); ++it2) {
        std::cout << *it2 << " ";
    }
    std::cout << std::endl;

    std::cout << "Reverse: ";
    for (MutantStack<int>::reverse_iterator rit = mstack2.rbegin(); rit != mstack2.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    std::cout << "\nTest with std::string..." << std::endl;
    MutantStack<std::string> strStack;
    strStack.push("Hello");
    strStack.push("World");
    strStack.push("!");

    for (MutantStack<std::string>::iterator sit = strStack.begin(); sit != strStack.end(); ++sit) {
        std::cout << *sit << " ";
    }
    std::cout << std::endl;

    return 0;
}