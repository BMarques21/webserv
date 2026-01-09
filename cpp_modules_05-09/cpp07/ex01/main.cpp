//
// Created by viceda-s on 03/01/2026.
//

#include "iter.hpp"

template <typename T>
void printElement(T &element)
{
    std::cout << element << " ";
}

template <typename T>
void incrementElement(T &element)
{
    element++;
}

void toUppercase(char &c)
{
    if (c >= 'a' && c <= 'z')
        c = c - 32;
}

int main(void)
{
    std::cout << "Testing with int array..." << std::endl;
    int intArray[] = {1, 2, 3, 4, 5};
    int intLength = 5;

    std::cout << "Original: ";
    iter(intArray, intLength, printElement<int>);
    std::cout << std::endl;

    iter(intArray, intLength, incrementElement<int>);
    std::cout << "After increment: ";
    iter(intArray, intLength, printElement<int>);
    std::cout << std::endl << std::endl;

    std::cout <<"Testing with double array..." << std::endl;
    double doubleArray[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    int doubleLength = 5;

    std::cout << "Original: ";
    iter(doubleArray, doubleLength, printElement<double>);
    std::cout << std::endl;

    iter(doubleArray, doubleLength, incrementElement<double>);
    std::cout << "After increment: ";
    iter(doubleArray, doubleLength, printElement<double>);
    std::cout << std::endl << std::endl;

    std::cout << "Testing with char array..." << std::endl;
    char charArray[] = {'h', 'e', 'l', 'l', 'o'};
    int charLength = 5;

    std::cout << "Original: ";
    iter(charArray, charLength, printElement<char>);
    std::cout << std::endl;

    iter(charArray, charLength, toUppercase);
    std::cout << "After toUppercase: ";
    iter(charArray, charLength, printElement<char>);
    std::cout << std::endl << std::endl;

    // Test with string array
    std::cout << "Testing with string array..." << std::endl;
    std::string stringArray[] = {"Hello", "World", "42"};
    int stringLength = 3;

    std::cout << "Strings: ";
    iter(stringArray, stringLength, printElement<std::string>);
    std::cout << std::endl;

    return 0;
}
