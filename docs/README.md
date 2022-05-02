# Quick start
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
# Full documentation
## Creating iterators
---
`rusty::iter(Collection)`  
Creates a rusty iterator from a collection (using `.begin()` and `.end()`)
```cpp
std::vector<int> numbers = { 1, 2, 3, 4 };
auto it = rusty::iter(numbers);
```
---
`rusty::iter(CppIterator begin, CppIterator end)`  
Creates a rusty iterator from from C++ iterators.
```cpp
std::vector<int> numbers = { 1, 2, 3, 4 };
auto it = rusty::iter(numbers.begin(), numbers.end());
```
---
`rusty::range<T>(T min, T max)`  
`rusty::range<T>(T min, T max, T step)`  
Creates an iterator, which starts with the provided `min` value, increasing the value by the provided `step` value (or 1, if not provided), until it reaches the `max` (exclusive) value.  
Negative step values will create an infinite iterator (this is not the intended use for this iterator). You should use the `infinite_range` function instead.
```cpp
auto it = rusty::range(0, 10); // yields 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
auto itWithStep = rusty::range(0, 10, 2); // yields 0, 2, 4, 6, 8
```
---
`rusty::range_inclusive<T>(T min, T max)`  
`rusty::range_inclusive<T>(T min, T max, T step)`  
Same as `rusty::range`, but the `max` value is also yielded.
```cpp
auto it = rusty::range_inclusive(0, 10); // yields 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
auto it2 = rusty::range_inclusive(0, 10, 2); // yields 0, 2, 4, 6, 8, 10
```
---
`rusty::infinite_range<T>(T min)`  
`rusty::infinite_range<T>(T min, T step)`  
Creates an infinite iterator, which starts with the provided `min` value, increasing the value by the provided `step` value (or 1, if not provided), every iteration.
```cpp
auto it = rusty::infinite_range(0); // yields 0, 1, 2, 3, 4, 5, etc...
auto itWithStep = rusty::infinite_range(0, 2); // yields 0, 2, 4, 6, 8, 10, etc...
```
---
`rusty::infinite_generator(GeneratorFunction)`  
`rusty::repeat_with(GeneratorFunction)`  
Creates an infinite iterator which yields elements by repeatedly calling the provided generator function.  
Both of these functions are identical, but `repeat_with` is the name used in Rust, so it was included here as well.
```cpp
int num = 0;
auto gen = [&]()
{
    int value = num;
    num = num * 2 + 1;
    return value;
};

auto it = rusty::infinite_generator(gen); // yields 0, 1, 3, 7, 15, 31, etc.
```
---
`rusty::finite_generator(GeneratorFunction)`  
`rusty::from_fn(GeneratorFunction)`  
Creates a finite iterator, which yields elements by repeatedly calling the provided generator function.  
The generator function must return an `std::optional` value.  
The iterator runs until the generator returns an empty value (`nullopt`).  
Both of these functions are identical, but `from_fn` is the name used in Rust, so it was included here as well.
```cpp
int num = 0;
auto gen = [&]() -> std::optional<int>
{
    int value = num;
    if (value > 20)
    {
        return { };
    }

    num = num * 2 + 1;
    return value;
};

auto it = rusty::finite_generator(gen); // yields 0, 1, 3, 7, 15
```
---
`rusty::empty<T>()`  
Creates an empty iterator, which yields no values.
```cpp
auto it = rusty::empty<int>(); // yields nothing
```
---
`rusty::once<T>(T value)`  
Creates an iterator that yields the provided value, only once.
```cpp
auto it = rusty::once<int>(123); // yields 123
```
---
`rusty::once_with(ValueCalculatorFunction)`  
Creates an iterator that yields the value returned by the provided function, only once.
```cpp
auto it = rusty::once_with([]() { return 123; }); // yields 123
```
---
`rusty::repeat<T>(T value)`  
Creates an iterator that yields the provided value, never stopping.
```cpp
auto it = rusty::repeat(123); // yields 123, 123, 123, 123, 123, ... forever
```
---
`rusty::successors<T>(T initialValue, SuccessorCalculatorFunction)`  
Creates an iterator where each element is calculated from the previous one, using the provided function.  
The iterator starts with the given first element (if not empty), and calls the provided function to calculate the next element.  
The function must return an `std::optional` value.  
The iterator stops when the provided function returns an empty value.  
Unfortunately, because the first parameter is an optional, the compiler cannot deduce T automatically, so you must either specify the template arguments manually, or explicitly pass an std::optional value as the first parameter.  
(for example, you can write `rusty::successors<int>(1, ...);` or `rusty::successors(std::optional(1), ...);`)  
If you know a solution for this, please tell me :)
```cpp
auto calculateSuccessor = [&](const int& previous) -> std::optional<int>
{
    int successor = previous * 2;
    if (successor > 100)
    {
        return { };
    }
    else
    {
        return successor;
    }
};

auto it = rusty::successors<int>(1, calculateSuccessor);
// yields 1, 2, 4, 8, 16, 32, 64
```
## Advancing iterators
Every iterator has a `next` method, which advances the iterator, and returns a pointer to the next value.  
Returns null if there are no more elements left in the iterator.  
The returned pointer becomes invalid if the iterator goes out of scope,
or if the iterator is advanced again (by calling `next`).
```cpp
auto it = rusty::range(0, 3);
int value = *it.next(); // == 0
value = *it.next(); // == 1
value = *it.next(); // == 2
const int* nothing = it.next(); // == nullptr
```
Usually, you won't call `next` manually, you'll probably  
iterate over the values, for example: `for (const auto& value : it) { ... }`,  
or call a method that consumes the iterator, for example: `it.sum()`.

## Iterator functions
---
`.step_by<T>(T step)`  
Creates an iterator that steps multiple times with each iteration, by the given step amount.  
Using 1 as the step value will produce identical behavior to the original iterator.  
Using step values that are less than 1 are invalid, and will create an empty iterator.
```cpp
std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
auto it = rusty::iter(numbers).step_by(3); // yields 1, 4, 7, 10
```
---
`.chain(OtherIterator)`  
Appends an iterator to the end of the current iterator.  
This new iterator will yield elements from the first iterator until it finishes, then from the second iterator.
```cpp
auto it = rusty::range(0, 3).chain(rusty::range(10, 13));
// yields 0, 1, 2, 10, 11, 12
```
---
`.zip(OtherIterator)`  
Creates an iterator that iterates over two iterators in parallel.  
This new iterator will yield a pair of elements, the first element is from the first iterator, the second element is from the second.  
This iterator will stop yielding elements when any of the iterators are finished.
```cpp
std::vector<std::string> texts = { "hello world", "foo", "bar", "test 1234" };
auto it = rusty::range(0, 10).zip(rusty::iter(texts));
// yields std::pair values:
// { 0, "hello world" },
// { 1, "foo" },
// { 2, "bar" },
// { 3, "test 1234 }
```
---
`.intersperse<T>(T separator)`  
Creates an iterator that inserts a separator value between each element.  
The separator will not be inserted before the first element, nor after the last element.
```cpp
auto it = rusty::range(0, 4).intersperse(-1); // yields 0, -1, 1, -1, 2, -1, 3
```
---
`.intersperse_with(SeparatorGetterFunction)`  
Creates an iterator that inserts a separator value between each element.  
The current separator value is obtained by calling the provided `SeparatorGetterFunction` function.  
The separator will not be inserted before the first element, nor after the last element.
```cpp
int num = 0;
auto separatorGetter = [&]()
{
    return --num;
};

auto it = rusty::range(0, 4).intersperse_with(separatorGetter);
// yields 0, -1, 1, -2, 2, -3, 3
```
---
`.map(MapFunction)`  
Transforms all values in the iterator, replacing each element with a new one.
The provided map function takes the old value, and returns the new value.
The old and the new values can have different types.
```cpp
std::vector<std::string> texts = { "hello world", "foo", "bar", "test 1234" };
auto it = rusty::iter(texts)
    .map([](const std::string& str) { return str.length(); });
// yields 11, 3, 3, 9
```
---
`.filter(FilterFunction)`  
Creates an iterator that only yields elements that satisfy the filter function.  
Elements that the filter function returns false for are skipped, the rest are kept.
```cpp
auto it = rusty::range(0, 10)
    .filter([](const int& num) { return num % 3 == 0; });
// yields 0, 3, 6, 9
```
---
`.filter_map(FilterMapFunction)`  
Creates an iterator that filters and maps at the same time.  
The provided filterMap function takes the old value, and must return a non-empty `std::optional` value if the current element should be kept (and transformed), and must return an empty value if the element should be skipped.
```cpp
auto filterMapFunction = [](const std::string& str) -> std::optional<size_t>
{
    if (str.length() > 5)
    {
        return str.length();
    }
    else
    {
        return { };
    }
};

std::vector<std::string> texts = { "hello world", "foo", "bar", "test 1234" };
auto it = rusty::iter(texts).filter_map(filterMapFunction);
// yields 11, 9
```
---
`.peekable()`   
Creates an iterator which has an extra `peek` function, which allows you to retrieve the next value in the iterator without advancing the current iterator.  
Note that the underlying iterator is still advanced to get the next value when `peek` is called.
```cpp
auto it = rusty::range(0, 10).peekable();
int peeked = *it.peek(); // returns 0, but the iterator is not advanced
int peeked2 = *it.peek(); // still returns 0
int next = *it.next(); // returns 0
peeked = *it.peek(); // returns 1
peeked2 = *it.peek(); // still returns 1
next = *it.next(); // returns 1
// etc.
```
---
`.skip_while(Predicate)`  
Creates an iterator that skips elements while the given predicate returns true.  
When the predicate returns false for the first time, then no more items will be skipped.
```cpp
auto it = rusty::range(0, 10)
    .skip_while([](const int& num) { return num < 5; });
// yields 5, 6, 7, 8, 9
```
---
`.take_while(Predicate)`  
Creates an iterator that yields elements only while the given predicate returns true.  
When the predicate returns false for the first time, then the rest of the elements are skipped.
```cpp
auto it = rusty::range(0, 10)
    .take_while([](const int& num) { return num < 5; });
// yields 0, 1, 2, 3, 4
```
---
`.skip<T>(T count)`  
Creates an iterator which skips the given number of elements, by advancing the underlying iterator that many times.  
If the underlying iterator is too short, then this function creates an empty iterator.
```cpp
auto it = rusty::range(0, 10).skip(6); // yields 6, 7, 8, 9
```
---
`.take<T>(T count)`  
Creates an iterator which yields the given number of elements at most.  
If the underlying iterator is too short, then less elements will be yielded.
```cpp
auto it = rusty::range(0, 10).take(6); // yields 0, 1, 2, 3, 4, 5
```
---
`.enumerate()`  
Creates an iterator which yields the current element as well as its index in the current iterator.  
The new iterator returns pairs, where the first element is the index, and the second element is the value.
```cpp
std::vector<std::string> texts = { "hello world", "foo", "bar", "test 1234" };
auto it = rusty::iter(texts).enumerate();
// yields std::pair values:
// { 0, "hello world" },
// { 1, "foo" },
// { 2, "bar" },
// { 3, "test 1234" }
```
---
`.flatten()`  
Creates an iterator from an iterator of iterators, which removes one level of nesting.  
Note that this function can only be called on nested iterators.
```cpp
std::vector<std::string> texts = { "abc", "def", "ghi" };
auto it = rusty::iter(texts)
    .map([](const std::string& str) { return rusty::iter(str); })
    .flatten();
// yields 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'
```
---
`.inspect(InspectFunction)`  
Creates an iterator that will call the provided inspect function on each value before yielding them.  
The inspect function cannot change those values, only read (inspect) them.  
This function can be useful for debugging.
```cpp
auto it = rusty::range(0, 5)

    // will print 0, 1, 2, 3, 4
    .inspect([](const int& num) { std::cout << num << std::endl; })
    .map([](const int& num) { return num * 2 + 1; })

    // will print 1, 3, 5, 7, 9
    .inspect([](const int& num) { std::cout << num << std::endl; })

    // etc...
```
---
`.cycle()`  
Creates an iterator that repeats the current iterator endlessly.   
When that iterator is finshed, it will start again at the beginning, instead of finishing.   
If the current iterator is empty, then the cycled iterator will also be empty.
```cpp
auto it = rusty::range(0, 3).cycle(); // yields, 0, 1, 2, 0, 1, 2, 0, etc...
```
## Consumer functions
---
`.for_each(Callback)`  
Calls the provided callback on all remaining elements of the iterator.
```cpp
rusty::range(0, 10).for_each([](const int& num)
{
    std::cout << num << std::endl;
});
// prints 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
```
---
`.collect<Collection>()`  
Transforms the iterator into a collection, e.g. an `std::vector`, `std::string`, `std::set`, etc.
```cpp
std::vector<int> values = rusty::range(0, 5).collect<std::vector<int>>();
// will contain 0, 1, 2, 3, 4
```
---
`.collect_with_size_hint<Collection, T>(T sizeHint)`  
Same as collect, but calls `reserve` on the collection before adding any elements to it.
```cpp
std::vector<int> values = rusty::range(0, 5)
    // will reserve memory for 5 elements in advance
    .collect_with_size_hint<std::vector<int>>(5);
// the vector will still contain 0, 1, 2, 3, 4
```
---
`.partition<Collection>(Predicate)`  
Partitions the elements of the iterator based on the provided predicate callback.  
This function creates two collections - the first collection will contain all values which the predicate returned false for, the second will contain the rest (which the predicate returned true for).
```cpp
std::pair<std::vector<int>, std::vector<int>> partitioned =
    rusty::range(0, 10)
    .partition<std::vector<int>>([](const int& num) { return num % 2 == 0 });
// will contain:
// .first == { 1, 3, 5, 7, 9 }
// .second == { 0, 2, 4, 6, 8 }
```
---
`.reduce(ReduceFunction)`  
Reduces the iterator into a single value, by repeatedly calling the provided reducer function.  
The reducer function takes two parameters:  
The first parameter is the accumulator, which will contain the last value returned by the reducer function. (when it's first called, it will be the first element of the iterator)  
The second parameter is the current element of the iterator.  
The reducer function must return a value, which will be passed as the first parameter to the reducer function, or, for the last element, this value will be the result of the entire reduce operation.  
For empty iterators, an empty value is returned. In all other cases, a non-empty value will be returned.
```cpp
auto reduceFunc = [](const int& a, const int& b)
{
    return a > b ? a : b;
};

int reduced = rusty::range(0, 10).reduce(reduceFunc);
// reduced == 9
```
---
`.fold<T>(T initialValue, FoldFunction)`  
Similarly to `reduce`, this function also reduces the iterator to a single value, the difference is that `fold` takes an initial value, instead of using the first value in the iterator.  
Because of this, `fold` will always return a value. (If the iterator is empty, then the initial value is returned)
```cpp
auto foldFunc = [](const int& acc, const int& current)
{
    return acc + current;
};

int folded = rusty::range(0, 10).fold(0, foldFunc);
// folded == 45
```
---
`.count()`  
Consumes the iterator, and returns the number of elements in it.
```cpp
size_t count = rusty::range(5, 8).count();
// count == 3
```
---
`.last()`  
Consumes the iterator, and returns its last element.  
If the iterator is empty, then an empty value is returned.
```cpp
int last = *rusty::range(0, 10).last();
// last == 9
std::optional<int> lastEmpty = rusty::range(0, 0).last();
// lastEmpty == nullopt
```
---
`.nth<T>(T index)`  
Returns the element at the given index, by advancing the iterator idx + 1 times.  
If the iterator doesn't have enough elements, then an empty value is returned.
```cpp
int fifth = *rusty::range(0, 10).nth(5);
// fifth == 5
std::optional<int> fifthEmpty = rusty::range(0, 3).nth(5);
// fifthEmpty == nullopt
```
---
`.all(Predicate)`  
Calls the provided predicate on all elements of the iterator, and checks if all returned values are true.  
The process will stop at the first false value, so this function might not fully consume the iterator.  
For empty iterators, this function returns true.
```cpp
auto lessThanFive = [](const int& num)
{
    return num < 5;
};

rusty::range(0, 4).all(lessThanFive); // true
rusty::range(0, 10).all(lessThanFive); // false
```
---
`.any(Predicate)`  
Calls the provided predicate on all elements of the iterator, and checks if any of the returned values is true.  
The process will stop at the first true value, so this function might not fully consume the iterator.  
For empty iterators, this function returns false.
```cpp
auto lessThanFive = [](const int& num)
{
    return num < 5;
};

rusty::range(0, 10).any(lessThanFive); // true
rusty::range(6, 10).any(lessThanFive); // false
```
---
`.find(Predicate)`  
Advances the iterator until a value that satisfies the predicate is found.  
The process will stop at the first such value, so this function might not fully consume the iterator.  
If no element was found, then an empty value is returned.
```cpp
auto isFive = [](const int& num)
{
    return num == 5;
};

int five = *rusty::range(0, 10).find(isFive);
// five == 5
std::optional<int> emptyFive = rusty::range(10, 20).find(isFive);
// emptyFive == nullopt
```
---
`.position(Predicate)`  
Advances the iterator until a value that satisfies the predicate is found, and returns its index.  
The process will stop at the first such value, so this function might not fully consume the iterator.  
If no element was found, then an empty value is returned.
```cpp
auto isFive = [](const int& num)
{
    return num == 5;
};

int fiveIndex = *rusty::range(0, 10).position(isFive);
// fiveIndex == 5
std::optional<int> emptyIndex = rusty::range(10, 20).position(isFive);
// emptyIndex == nullopt
```
---
`.min()`  
Returns the minimum value in the iterator, comparing elements with the < operator.  
If there are multiple minimum values, then the first one is returned.  
If the iterator is empty, then an empty value is returned.
```cpp
int min = *rusty::range(0, 10).min();
// min == 0
std::optional<int> emptyMin = rusty::range(0, 0).min();
// emptyMin == nullopt
```
---
`.min_by(Comparer)`  
Returns the minimum value in the iterator, comparing elements with the provided comparer function.  
If there are multiple minimum values, then the first one is returned.  
If the iterator is empty, then an empty value is returned.
```cpp
auto comparer = [](const int& a, const int& b)
{
    return a - b;
};

int min = *rusty::range(0, 10).min_by(comparer);
// min == 0
std::optional<int> emptyMin = rusty::range(0, 0).min_by(comparer);
// emptyMin == nullopt
```
---
`.max()`  
Returns the maximum value in the iterator, comparing elements with the > operator.  
If there are multiple maximum values, then the first one is returned.  
If the iterator is empty, then an empty value is returned.
```cpp
int max = *rusty::range(0, 10).max();
// max == 9
std::optional<int> emptyMax = rusty::range(0, 0).max();
// emptyMax == nullopt
```
---
`.max_by(Comparer)`  
Returns the maximum value in the iterator, comparing elements with the provided comparer function.  
If there are multiple maximum values, then the first one is returned.  
If the iterator is empty, then an empty value is returned.
```cpp
auto comparer = [](const int& a, const int& b)
{
    return a - b;
};

int max = *rusty::range(0, 10).max_by(comparer);
// min == 9
std::optional<int> emptyMax = rusty::range(0, 0).max_by(comparer);
// emptyMax == nullopt
```
---
`.sum()`  
Returns the sum of all elements in the iterator, by adding all elements together.  
If the iterator is empty, then 0 is returned.
```cpp
rusty::range(0, 10).sum(); // == 45
```
---
`.product()`  
Returns the product of all elements in the iterator, by multiplying all elements together.  
If the iterator is empty, then 1 is returned.
```cpp
rusty::range(1, 10).product(); // == 362880
```
---
`.is_sorted_ascending()`  
Returns true if the iterator is sorted ascending, so that no element is less than the previous one.  
The process will stop when a non-sorted pair is encountered, so this function might not fully consume the iterator.  
For iterators with less than 2 elements, this function always returns true.
```cpp
std::vector<int> numbersAscending = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
std::vector<int> numbersDescending = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
rusty::iter(numbersAscending).is_sorted_ascending(); // true
rusty::iter(numbersDescending).is_sorted_ascending(); // false
```
---
`.is_sorted_descending()`  
Returns true if the iterator is sorted descending, so that no element is greater than the previous one.  
The process will stop when a non-sorted pair is encountered, so this function might not fully consume the iterator.  
For iterators with less than 2 elements, this function always returns true.
```cpp
std::vector<int> numbersAscending = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
std::vector<int> numbersDescending = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
rusty::iter(numbersAscending).is_sorted_descending(); // false
rusty::iter(numbersDescending).is_sorted_descending(); // true
```
---
`.is_sorted_by(Comparer)`  
Returns true if the iterator is sorted by the given comparer.  
The comparer function must return:  
- a negative value if the first compared value is less than the second,
- a positive value if the first compared value is greater than the second,
- zero if they are equal

The process will stop when a non-sorted pair is encountered, so this function might not fully consume the iterator.  
For iterators with less than 2 elements, this function always returns true.
```cpp
auto comparer = [](const int& a, const int& b)
{
    return a - b;
};

std::vector<int> numbersAscending = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
std::vector<int> numbersDescending = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
rusty::iter(numbersAscending).is_sorted_by(comparer); // true
rusty::iter(numbersDescending).is_sorted_by(comparer); // false
```
## Comparison functions
---
`.cmp(OtherIterator)`  
Lexicographically compares the elements of this iterator with another iterator's elements.  
Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<int> shortRange = { 0, 1 };
std::vector<int> longRange = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
std::vector<int> smallRange = { 0, 1, 2, 3 };
std::vector<int> bigRange = { 5, 6, 7, 8 };

rusty::iter(shortRange).cmp(rusty::iter(shortRange)); // == 0
rusty::iter(shortRange).cmp(rusty::iter(longRange)); // <0
rusty::iter(longRange).cmp(rusty::iter(shortRange)); // >0
rusty::iter(bigRange).cmp(rusty::iter(smallRange)); // >0
rusty::iter(smallRange).cmp(rusty::iter(bigRange)); // <0
```
---
`.cmp_by(OtherIterator, Comparer)`  
Lexicographically compares the elements of this iterator with another iterator's elements, using the provided comparison function.  
Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
auto cmp = [](const int& a, const int& b) { return a - b; };

std::vector<int> shortRange = { 0, 1 };
std::vector<int> longRange = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
std::vector<int> smallRange = { 0, 1, 2, 3 };
std::vector<int> bigRange = { 5, 6, 7, 8 };

rusty::iter(shortRange).cmp_by(rusty::iter(shortRange), cmp); // == 0
rusty::iter(shortRange).cmp_by(rusty::iter(longRange), cmp); // <0
rusty::iter(longRange).cmp_by(rusty::iter(shortRange), cmp); // >0
rusty::iter(bigRange).cmp_by(rusty::iter(smallRange), cmp); // >0
rusty::iter(smallRange).cmp_by(rusty::iter(bigRange), cmp); // <0
```
---
`.partial_cmp(OtherIterator)`  
Lexicographically compares the elements of this iterator with another iterator's elements, using the provided comparison function.  
Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.  
Returns an empty value if there was at least one pair of elements that could not be compared.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<float> shortRangeFloat = { 0.0f, 1.0f };
std::vector<float> longRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
std::vector<float> smallRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f };
std::vector<float> bigRangeFloat = { 5.0f, 6.0f, 7.0f, 8.0f };
std::vector<float> nonComparable = { 0.0f, std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f };

*rusty::iter(shortRangeFloat).partial_cmp(rusty::iter(shortRangeFloat)); // == 0
*rusty::iter(shortRangeFloat).partial_cmp(rusty::iter(longRangeFloat)); // <0
*rusty::iter(longRangeFloat).partial_cmp(rusty::iter(shortRangeFloat)); // >0
*rusty::iter(bigRangeFloat).partial_cmp(rusty::iter(smallRangeFloat)); // >0
*rusty::iter(smallRangeFloat).partial_cmp(rusty::iter(bigRangeFloat)); // <0
rusty::iter(smallRangeFloat).partial_cmp(rusty::iter(nonComparable)); // == nullopt
```
---
`.partial_cmp_by(OtherIterator, PartialComparer)`  
Lexicographically compares the elements of this iterator with another iterator's elements, using the provided comparison function.  
The comparison function must be a partial comparer, which means that it can return an empty value if the elements cannot be compared.  
If the elements can be compared, then the usual <0, 0, and >0 must be returned.  
Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.  
Returns an empty value if there was at least one pair of elements that could not be compared.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
auto partialCmp = [](const float& a, const float& b) -> std::optional<char>
{
    if (a == b)
    {
        return 0;
    }
    else if (a < b)
    {
        return -1;
    }
    else if (a > b)
    {
        return 1;
    }
    else
    {
        return { };
    }
};

*rusty::iter(shortRangeFloat).partial_cmp_by(rusty::iter(shortRangeFloat), partialCmp); // == 0
*rusty::iter(shortRangeFloat).partial_cmp_by(rusty::iter(longRangeFloat), partialCmp); // <0
*rusty::iter(longRangeFloat).partial_cmp_by(rusty::iter(shortRangeFloat), partialCmp); // >0
*rusty::iter(bigRangeFloat).partial_cmp_by(rusty::iter(smallRangeFloat), partialCmp); // >0
*rusty::iter(smallRangeFloat).partial_cmp_by(rusty::iter(bigRangeFloat), partialCmp); // <0
rusty::iter(smallRangeFloat).partial_cmp_by(rusty::iter(nonComparable), partialCmp); // == nullopt
```
---
`.eq(OtherIterator)`  
Determines if the elements of this iterator are equal to the elements of another iterator.  
Returns true only if both iterators have the same number of elements, and all elements are equal.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<int> shortRange = { 0, 1 };
std::vector<int> longRange = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
rusty::iter(shortRange).eq(rusty::iter(shortRange)); // true
rusty::iter(shortRange).eq(rusty::iter(longRange)); // false
rusty::iter(longRange).eq(rusty::iter(shortRange)); // false
```
---
`.eq_by(OtherIterator, EqualityComparer)`  
Determines if the elements of this iterator are equal to the elements of another iterator, using the provided equality comparer function.  
Returns true only if both iterators have the same number of elements, and all elements are equal.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
auto eq = [](const int& a, const int& b) { return a == b; };

std::vector<int> shortRange = { 0, 1 };
std::vector<int> longRange = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

rusty::iter(shortRange).eq_by(rusty::iter(shortRange), eq); // true
rusty::iter(shortRange).eq_by(rusty::iter(longRange), eq); // false
rusty::iter(longRange).eq_by(rusty::iter(shortRange), eq); // false
```
---
`.ne(OtherIterator)`  
Determines if the elements of this iterator are not equal to the elements of another iterator.  
Returns true if the iterators have a different number of elements, or if there is at least one pair of elements that are different.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<int> shortRange = { 0, 1 };
std::vector<int> longRange = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
std::vector<int> smallRange = { 0, 1, 2, 3 };
std::vector<int> bigRange = { 5, 6, 7, 8 };

rusty::iter(shortRange).ne(rusty::iter(shortRange)); // false
rusty::iter(shortRange).ne(rusty::iter(longRange)); // true
rusty::iter(longRange).ne(rusty::iter(shortRange)); // true
rusty::iter(bigRange).ne(rusty::iter(smallRange)); // true
rusty::iter(smallRange).ne(rusty::iter(bigRange)); // true
```
---
`.lt(OtherIterator)`  
Lexicographically compares the elements of this iterator with another iterator's elements.  
Returns true if this iterator is less than the other iterator, false otherwise.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<float> shortRangeFloat = { 0.0f, 1.0f };
std::vector<float> longRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
std::vector<float> smallRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f };
std::vector<float> bigRangeFloat = { 5.0f, 6.0f, 7.0f, 8.0f };
std::vector<float> nonComparable = { 0.0f, std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f };

rusty::iter(shortRangeFloat).lt(rusty::iter(shortRangeFloat)); // false
rusty::iter(shortRangeFloat).lt(rusty::iter(longRangeFloat)); // true
rusty::iter(longRangeFloat).lt(rusty::iter(shortRangeFloat)); // false
rusty::iter(bigRangeFloat).lt(rusty::iter(smallRangeFloat)); // false
rusty::iter(smallRangeFloat).lt(rusty::iter(bigRangeFloat)); // true
rusty::iter(nonComparable).lt(rusty::iter(nonComparable)); // false
```
---
`.le(OtherIterator)`  
Lexicographically compares the elements of this iterator with another iterator's elements.  
Returns true if this iterator is less than or equal to the other iterator, false otherwise.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<float> shortRangeFloat = { 0.0f, 1.0f };
std::vector<float> longRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
std::vector<float> smallRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f };
std::vector<float> bigRangeFloat = { 5.0f, 6.0f, 7.0f, 8.0f };
std::vector<float> nonComparable = { 0.0f, std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f };

rusty::iter(shortRangeFloat).le(rusty::iter(shortRangeFloat)); // true
rusty::iter(shortRangeFloat).le(rusty::iter(longRangeFloat)); // true
rusty::iter(longRangeFloat).le(rusty::iter(shortRangeFloat)); // false
rusty::iter(bigRangeFloat).le(rusty::iter(smallRangeFloat)); // false
rusty::iter(smallRangeFloat).le(rusty::iter(bigRangeFloat)); // true
rusty::iter(nonComparable).le(rusty::iter(nonComparable)); // false
```
---
`.gt(OtherIterator)`  
Lexicographically compares the elements of this iterator with another iterator's elements.  
Returns true if this iterator is greater than the other iterator, false otherwise.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.  
```cpp
std::vector<float> shortRangeFloat = { 0.0f, 1.0f };
std::vector<float> longRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
std::vector<float> smallRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f };
std::vector<float> bigRangeFloat = { 5.0f, 6.0f, 7.0f, 8.0f };
std::vector<float> nonComparable = { 0.0f, std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f };

rusty::iter(shortRangeFloat).gt(rusty::iter(shortRangeFloat)); // false
rusty::iter(shortRangeFloat).gt(rusty::iter(longRangeFloat)); // false
rusty::iter(longRangeFloat).gt(rusty::iter(shortRangeFloat)); // true
rusty::iter(bigRangeFloat).gt(rusty::iter(smallRangeFloat)); // true
rusty::iter(smallRangeFloat).gt(rusty::iter(bigRangeFloat)); // false
rusty::iter(nonComparable).gt(rusty::iter(nonComparable)); // false
```
---
`.ge(OtherIterator)`  
Lexicographically compares the elements of this iterator with another iterator's elements.  
Returns true if this iterator is greater than or equal to the other iterator, false otherwise.  
Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
```cpp
std::vector<float> shortRangeFloat = { 0.0f, 1.0f };
std::vector<float> longRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
std::vector<float> smallRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f };
std::vector<float> bigRangeFloat = { 5.0f, 6.0f, 7.0f, 8.0f };
std::vector<float> nonComparable = { 0.0f, std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f };

rusty::iter(shortRangeFloat).ge(rusty::iter(shortRangeFloat)); // true
rusty::iter(shortRangeFloat).ge(rusty::iter(longRangeFloat)); // false
rusty::iter(longRangeFloat).ge(rusty::iter(shortRangeFloat)); // true
rusty::iter(bigRangeFloat).ge(rusty::iter(smallRangeFloat)); // true
rusty::iter(smallRangeFloat).ge(rusty::iter(bigRangeFloat)); // false
rusty::iter(nonComparable).ge(rusty::iter(nonComparable)); // false
```
