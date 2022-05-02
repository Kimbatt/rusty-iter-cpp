# rusty-iter-cpp
This library provides iterator functionality similar to what is available in the Rust programming language.
See https://doc.rust-lang.org/std/iter/trait.Iterator.html
## Usage
This is a single-file, header-only library. To use it, just include [rusty-iter.hpp](https://github.com/Kimbatt/rusty-iter-cpp/blob/master/include/rusty-iter.hpp) in your project.  

All available functions are in the `rusty` namespace.  

Creating a rusty iterator from an stl collection:
```cpp
auto it = rusty::iter(collection);
```
Creating a rusty iterator from C++ iterators:
```cpp
auto it = rusty::iter(begin, end);
```
Iterating over the elements of a rusty iterator:
```cpp
for (const auto& value : iter) { ... }
```
or
```cpp
iter.for_each([](const auto& value) { ... });
```
Using iterators (example):
```cpp
rusty::iter(collection)
   .filter([](const int& value) { return value % 2 == 0; })
   .step_by(2)
   .map([](const int& value) { return value * value; })
   .collect<std::vector<int>>();
```
Iterators can be manually advanced by calling the `.next()` function on them, which will return a pointer to the next value, or a null pointer if the iterator has finished.
## Documentation
See https://github.com/Kimbatt/rusty-iter-cpp/blob/master/docs/README.md
## Requirements
This library requires C++17 or later.  
There are no external dependencies, only the C++ standard library (`type_traits` and `optional` headers are used).
## Comparison functions
When using functions that require you to specify a comparison function:  
The provided comparison function must take two values and return a value that is <0 if the first value is less than the second, 0 if the two values are equal, and >0 if the first value is greater than the second.  
You can return any type that can be compared this way.  
When using C++20 or later, you for example can use `std::strong_ordering` (spaceship operator).  
For partial comparisions, you need to return an `std::optional value`, which is empty if the values cannot be compared, otherwise just return <0, 0, or >0 (as described above).
## Notes on memory safety
Unlike Rust, C++ doesn't have compile time guarantees about memory safety, so you must be careful with the following:
- The values passed to the user-provided functions are only guaranteed to be valid for the duration of that function call.
For example:
```cpp
// DON'T DO THIS
int* ptr = nullptr;
std::vector<int> numbers = { 1, 2, 3 };
rusty::iter(numbers).for_each([](const int& i) { ptr = &i; });
std::cout << *ptr << std::endl; // BAD, ptr is invalid
```
- Same for loops:
```cpp
// DON'T DO THIS
int* ptr = nullptr;
std::vector<int> numbers = { 1, 2, 3 };
for (const int& i : rusty::iter(numbers))
{
    ptr = &i; // ptr becomes invalid after the current iteration
}
std::cout << *ptr << std::endl; // BAD, ptr is invalid
```
- Advancing the iterator after the underlying collection has been destroyed is undefined behavior.
For example:
```cpp
// DON'T DO THIS
for (const char& ch : rusty::iter(std::string("test")))
{
    // BAD
    // the temporary string value is already destroyed here
    // this will cause bad things
}

// DO THIS INSTEAD
std::string str = "test";
for (const char& ch : rusty::iter(str))
{
    // OK
    // the original string value still exists,
    // so the iterator is valid
}
```
## License
Licensed under either the WTFPL or the MIT License, at your choice.
See https://github.com/Kimbatt/rusty-iter-cpp/blob/master/LICENSE.txt
