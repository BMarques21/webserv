# CPP Module 07

*This project has been created as part of the 42 curriculum by viceda-s.*

## Description

CPP Module 07 is an introduction to **C++ templates**, a powerful feature that enables generic programming in C++. This module focuses on understanding and implementing function templates and class templates, which allow writing code that works with any data type while maintaining type safety.

The project is divided into three exercises:

- **Exercise 00 (whatever)**: Implementation of basic function templates (`swap`, `min`, `max`)
- **Exercise 01 (iter)**: A function template that applies a function to every element of an array
- **Exercise 02 (Array)**: A templated array class with dynamic memory management and bounds checking

### Learning Objectives

- Understanding template syntax and instantiation
- Writing generic functions that work with multiple types
- Creating templated classes with proper memory management
- Implementing operator overloading in template classes
- Exception handling with templates
- Deep understanding of compile-time polymorphism

## Instructions

### Compilation Requirements

- **Compiler**: `c++` (or `g++`, `clang++`)
- **Standard**: C++98
- **Flags**: `-Wall -Wextra -Werror -std=c++98`

### Compilation

Each exercise has its own Makefile. Navigate to the desired exercise directory and run:

```bash
make
```

This will compile the exercise and create an executable.

#### Exercise 00 - whatever
```bash
cd ex00
make
./whatever
```

#### Exercise 01 - iter
```bash
cd ex01
make
./iter
```

#### Exercise 02 - Array
```bash
cd ex02
make
./Array
```

### Cleaning

To remove object files:
```bash
make clean
```

To remove object files and executable:
```bash
make fclean
```

To recompile everything:
```bash
make re
```

## Project Structure

```
cpp07/
├── ex00/                    # Function templates basics
│   ├── whatever.hpp        # Template functions: swap, min, max
│   ├── main.cpp            # Test program
│   └── Makefile
├── ex01/                    # Iter function template
│   ├── iter.hpp            # Template function that applies function to array
│   ├── main.cpp            # Test program with various types
│   └── Makefile
└── ex02/                    # Array template class
    ├── Array.hpp           # Templated array class implementation
    ├── main.cpp            # Test program
    └── Makefile
```

## Features

### Exercise 00: Basic Function Templates

Implements three fundamental function templates:
- **`swap<T>`**: Swaps the values of two variables of type T
- **`min<T>`**: Returns the minimum of two values of type T
- **`max<T>`**: Returns the maximum of two values of type T

These functions work with any type that supports the required operations (assignment, comparison).

### Exercise 01: Iter

Implements a generic `iter` function that:
- Takes an array, its length, and a function pointer
- Applies the function to every element of the array
- Works with any array type and any compatible function

The implementation includes overloads for both const and non-const function pointers, demonstrating template specialization.

### Exercise 02: Array Template Class

A fully-featured templated array class that provides:
- **Default constructor**: Creates an empty array
- **Parameterized constructor**: Creates an array of n elements (value-initialized)
- **Copy constructor**: Deep copy implementation
- **Assignment operator**: Deep copy with self-assignment protection
- **Destructor**: Proper memory cleanup
- **Subscript operator `[]`**: With bounds checking (throws `std::out_of_range`)
- **`size()` method**: Returns the number of elements

## Algorithm and Data Structure Explanation

### Templates: Compile-Time Polymorphism

Templates are a C++ feature that enables **generic programming** through compile-time code generation. Unlike runtime polymorphism (inheritance and virtual functions), templates generate specialized code for each type at compile time.

#### Why Templates?

1. **Type Safety**: Unlike void pointers or macros, templates maintain full type checking
2. **Code Reusability**: Write once, use with any compatible type
3. **Performance**: No runtime overhead; code is specialized at compile time
4. **Expressiveness**: Clear intent and readable code

#### Function Templates (ex00, ex01)

Function templates allow writing a single function definition that works with multiple types:

```cpp
template <typename T>
void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}
```

**How it works**:
- When `swap(intA, intB)` is called, the compiler generates `swap<int>`
- When `swap(strA, strB)` is called, the compiler generates `swap<std::string>`
- Each instantiation is a separate function in the compiled binary

**Template Argument Deduction**: The compiler can usually infer the type T from the function arguments, so explicit specification (`swap<int>`) is often unnecessary.

#### Class Templates (ex02)

Class templates extend the concept to entire classes:

```cpp
template <typename T>
class Array {
    T *_data;
    unsigned int _size;
    // ...
};
```

**Design Decisions**:

1. **Dynamic Memory Allocation**: Uses `new[]` to allocate memory dynamically based on the size parameter
   - Allows arrays of any size
   - Requires manual memory management (new/delete)

2. **Value Initialization**: `new T[n]()` ensures elements are initialized
   - Primitive types (int, double) are zero-initialized
   - Class types are default-constructed
   - Prevents undefined behavior from uninitialized data

3. **Deep Copy Semantics**: Copy constructor and assignment operator perform deep copies
   - Each Array instance owns its own data
   - Prevents shallow copy issues (double deletion, unintended sharing)
   - Follows the **Rule of Three**: if you define one of (destructor, copy constructor, assignment operator), define all three

4. **Bounds Checking**: The subscript operator validates indices
   - Throws `std::out_of_range` for invalid indices
   - Trade-off: safety vs. performance
   - More robust than raw arrays or std::vector's `operator[]`

5. **Exception Safety**: Uses exceptions for error handling
   - Appropriate for C++ (vs. error codes)
   - Allows propagation up the call stack
   - Standard practice for operator overloading

#### Template Instantiation

Templates are instantiated (code is generated) when:
- The template is used with a specific type
- At compile time, the compiler substitutes T with the actual type
- If never used, no code is generated (no binary bloat)

#### Advantages Over Alternatives

| Feature | Templates | Inheritance/Virtual | Macros | void* |
|---------|-----------|-------------------|---------|-------|
| Type Safety | ✓ | ✓ | ✗ | ✗ |
| Performance | ✓ (no overhead) | ✗ (vtable) | ✓ | ✓ |
| Code Reusability | ✓ | Limited | ✓ | ✓ |
| Type Checking | Compile-time | Runtime | None | Runtime |
| Debuggability | Good | Good | Poor | Poor |

### Memory Management Strategy

The Array class uses **RAII** (Resource Acquisition Is Initialization):
- Resource (memory) is acquired in constructor
- Resource is released in destructor
- Guarantees no memory leaks (unless exceptions during construction)

**Complexity Analysis**:
- Construction: O(n) time, O(n) space
- Copy: O(n) time, O(n) space
- Element access: O(1) time
- Destruction: O(n) time

## Resources

### Documentation
- [cppreference.com - Templates](https://en.cppreference.com/w/cpp/language/templates)
- [cppreference.com - Template Parameters](https://en.cppreference.com/w/cpp/language/template_parameters)
- [cppreference.com - Function Templates](https://en.cppreference.com/w/cpp/language/function_template)
- [cppreference.com - Class Templates](https://en.cppreference.com/w/cpp/language/class_template)
- [C++ Standard Library - Array](https://en.cppreference.com/w/cpp/container/array)

### Articles and Tutorials
- [LearnCPP.com - Function Templates](https://www.learncpp.com/cpp-tutorial/function-templates/)
- [LearnCPP.com - Class Templates](https://www.learncpp.com/cpp-tutorial/class-templates/)
- [GeeksforGeeks - Templates in C++](https://www.geeksforgeeks.org/templates-cpp/)
- [Exception Safety](https://en.cppreference.com/w/cpp/language/exceptions)

### Books
- "C++ Templates: The Complete Guide" by David Vandevoorde and Nicolai M. Josuttis
- "Effective C++" by Scott Meyers (Items on templates and resource management)
- "The C++ Programming Language" by Bjarne Stroustrup (Chapter on Templates)

### AI Usage

AI was **not** used in the development of this project. All code was written manually to ensure full understanding of:
- Template syntax and semantics
- Memory management with templates
- Operator overloading in template contexts
- Exception handling
- The Rule of Three in resource management

The README documentation was also written without AI assistance, based on understanding gained through:
- Reading official C++ documentation
- Studying template mechanics
- Analyzing the compilation and instantiation process
- Testing edge cases and understanding the implementation choices

This hands-on approach ensured deep comprehension of template programming concepts and best practices in C++ generic programming.

## Testing

### Exercise 00 Tests
The main function tests the template functions with:
- Integer types (swap, min, max)
- String types (swap, min, max)

Expected output demonstrates that the same template code works correctly with different types.

### Exercise 01 Tests
The main function demonstrates:
- Printing array elements (int, double, char, string)
- Modifying array elements (increment)
- Custom transformations (uppercase conversion)

### Exercise 02 Tests
The provided main tests:
- Construction with size parameter
- Copy constructor and assignment operator (deep copy verification)
- Bounds checking with negative and out-of-range indices
- Memory management (no leaks)

To run with memory leak detection:
```bash
valgrind --leak-check=full ./Array
```

## Notes

- All code follows the **C++98 standard** (no C++11 or later features)
- Templates must be defined in header files (implementation in .hpp)
- The compiler performs **template instantiation** at compile time
- Template errors can produce verbose compiler messages
- Understanding templates is essential for using the STL effectively

## Author

**viceda-s** - 42 Network Student

---

*For questions, issues, or suggestions, please refer to the 42 project evaluation guidelines.*

