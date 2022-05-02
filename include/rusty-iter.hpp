// This library provides iterator functionality similar to what is available in the Rust programming language.
// See https://doc.rust-lang.org/std/iter/trait.Iterator.html

// Usage:
// All available functions are in the `rusty` namespace.
//
// Creating a rusty iterator from an stl collection:
//     auto it = rusty::iter(collection);
//
// Creating a rusty iterator from C++ iterators:
//     auto it = rusty::iter(begin, end);
//
// Iterating over the elements of a rusty iterator:
//     for (const auto& value : iter) { ... }
// or
//     iter.for_each([](const auto& value) { ... });
//
// Using iterators (example):
//     rusty::iter(collection)
//        .filter([](const int& value) { return value % 2 == 0; })
//        .step_by(2)
//        .map([](const int& value) { return value * value; })
//        .collect<std::vector<int>>();
//
// Iterators can be manually advanced by calling the `.next()` function on them, which will
// return a pointer to the next value, or a null pointer if the iterator has finished.

// Ranges
// Rust has ranges that can be used as iterators, so this functionality is also included here.
//
// Creating a range from min and max values (min inclusive, max exclusive)
//     auto rangeIt = rusty::range(0, 10); // yields 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
// Inclusive range:
//     auto rangeIt = rusty::range_inclusive(0, 10); // yields 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10

// Comparisons
// When using functions that require you to specify a comparison function:
// The provided comparison function must take two values and return a value
// that is <0 if the first value is less than the second, 0 if the two values are equal,
// and >0 if the first value is greater than the second.
// You can return any type that can be compared this way.
// When using C++20 or later, you for example can use std::strong_ordering (spaceship operator).
// For partial comparisions, you need to return an std::optional value, which is empty if the
// values cannot be compared, otherwise just return <0, 0, or >0 (as described above).

// Notes on memory safety
// Unlike Rust, C++ doesn't have compile time guarantees about memory safety, so you must be careful with the following:
// - The values passed to the user-provided functions are only guaranteed to be valid for the duration of that function call.
//   For example:
//
//       // DON'T DO THIS
//       int* ptr = nullptr;
//       std::vector<int> numbers = { 1, 2, 3 };
//       rusty::iter(numbers).for_each([](const int& i) { ptr = &i; });
//       std::cout << *ptr << std::endl; // BAD, ptr is invalid
//
//   Same for loops:
//
//       // DON'T DO THIS
//       int* ptr = nullptr;
//       std::vector<int> numbers = { 1, 2, 3 };
//       for (const int& i : rusty::iter(numbers))
//       {
//           ptr = &i; // ptr becomes invalid after the current iteration
//       }
//       std::cout << *ptr << std::endl; // BAD, ptr is invalid
//
// - Advancing the iterator after the underlying collection has been destroyed is undefined behavior.
//   For example:
//
//       DON'T DO THIS
//       for (const char& ch : rusty::iter(std::string("test")))
//       {
//           // BAD
//           // the temporary string value is already destroyed here
//           // this will cause bad things
//       }
//
//       // DO THIS INSTEAD
//       std::string str = "test";
//       for (const char& ch : rusty::iter(str))
//       {
//           // OK
//           // the original string value still exists,
//           // so the iterator is valid
//       }
//

// License
// This library is available under 2 licenses, you can choose whichever you prefer.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Alternative A - Do What The Fuck You Want To Public License
//
// Copyright (c) Kimbatt (https://github.com/Kimbatt)
//
// This work is free. You can redistribute it and/or modify it under the terms of
// the Do What The Fuck You Want To Public License, Version 2, as published by Sam Hocevar.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                    Version 2, December 2004
//
// Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
//
// Everyone is permitted to copy and distribute verbatim or modified
// copies of this license document, and changing it is allowed as long
// as the name is changed.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//
// 0. You just DO WHAT THE FUCK YOU WANT TO.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Alternative B - MIT License
//
// Copyright (c) Kimbatt (https://github.com/Kimbatt)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef RUSTY_ITER_HPP_INCLUDED
#define RUSTY_ITER_HPP_INCLUDED

#include <type_traits>
#include <optional>

namespace rusty
{
    namespace detail
    {
        //
        // Helpers
        //

        template <typename T>
        static char compare(const T& a, const T& b)
        {
            if (a < b)
            {
                return -1;
            }
            else if (a > b)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        template <typename T>
        static char compare_reverse(const T& a, const T& b)
        {
            return compare(b, a);
        }

        template <typename T>
        std::optional<char> compare_partial(const T& a, const T& b)
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
        }

        template <typename Collection, typename T>
        static void add_to_collection(Collection& collection, const T& value)
        {
            collection.push_back(value);
        }

        // Helper class which returns an increasing value when used as a functor.
        template <typename T>
        struct Counter
        {
            Counter(const T& count) : _currentCount(0), _maxCount(count)
            {
            }

            bool operator()()
            {
                return _currentCount++ < _maxCount;
            }

            template <typename Any>
            bool operator()(Any&&)
            {
                return this->operator()();
            }

        private:
            T _currentCount;
            T _maxCount;
        };

        // Helper class which returns a constant value when used as a functor.
        template <typename T>
        struct Getter
        {
            Getter(const T& value) : _value(value)
            {
            }

            const T& operator()() const
            {
                return _value;
            }

            template <typename Any>
            const T& operator()(Any&&) const
            {
                return this->operator()();
            }

        private:
            T _value;
        };

        // Helper class which returns an increasing value when used as a functor.
        template <typename T>
        struct InfiniteRangeGenerator
        {
            InfiniteRangeGenerator(const T& start, const T& step) : _value(start), _step(step)
            {
            }

            T operator()()
            {
                T currentValue = _value;
                _value += _step;
                return currentValue;
            }

        private:
            T _value;
            T _step;
        };

        // Helper class which returns an increasing value when used as a functor, stopping at a maximum value.
        template <typename T, bool Inclusive>
        struct FiniteRangeGenerator
        {
            FiniteRangeGenerator(const T& start, const T& end, const T& step) : _value(start), _end(end), _step(step)
            {
            }

            std::optional<T> operator()()
            {
                bool hasValue;
                if constexpr (Inclusive)
                {
                    hasValue = _value <= _end;
                }
                else
                {
                    hasValue = _value < _end;
                }

                if (hasValue)
                {
                    T currentValue = _value;
                    _value += _step;
                    return currentValue;
                }

                return { };
            }

        private:
            T _value;
            T _end;
            T _step;
        };

        // Helper class which returns the specified value when used as a functor, only once.
        template <typename T>
        struct YieldOnce
        {
            YieldOnce(const T& value) : _value(value), _done(false)
            {
            }

            std::optional<T> operator()()
            {
                if (_done)
                {
                    return { };
                }

                _done = true;
                return _value;
            }

            private:
                T _value;
                bool _done;
        };

        // Helper class which returns a value calculated from the given function when used as a functor, only once.
        template <typename ValueCalculatorFunction>
        struct YieldOnceWith
        {
            using ValueType = typename std::invoke_result<ValueCalculatorFunction>::type;

            YieldOnceWith(const ValueCalculatorFunction& valueCalculatorFunction) : _valueCalculatorFunction(valueCalculatorFunction), _done(false)
            {
            }

            std::optional<ValueType> operator()()
            {
                if (_done)
                {
                    return { };
                }

                _done = true;
                return _valueCalculatorFunction();
            }

        private:
            ValueCalculatorFunction _valueCalculatorFunction;
            bool _done;
        };

        // Helper class which returns the specified value when used as a functor.
        template <typename T>
        struct YieldValue
        {
            YieldValue(const T& value) : _value(value)
            {
            }

            T operator()()
            {
                return _value;
            }

        private:
            T _value;
        };

        // Helper class which calculates the successor of a value using the provided function
        template <typename T, typename SuccessorCalculatorFunction>
        struct SuccessorCalculator
        {
            SuccessorCalculator(const std::optional<T>& initialValue, const SuccessorCalculatorFunction& successorCalculatorFunction) :
                _value(initialValue), _successorCalculatorFunction(successorCalculatorFunction)
            {
            }

            std::optional<T> operator()()
            {
                if (!_value)
                {
                    return _value;
                }

                T prev = *_value;
                _value = _successorCalculatorFunction(prev);
                return prev;
            }

        private:
            std::optional<T> _value;
            SuccessorCalculatorFunction _successorCalculatorFunction;
        };

        // Helper class for displaying better error messages
        template <typename Func, typename ...Args>
        struct ReturnTypeHelperConstRefOrValue
        {
            constexpr static bool check()
            {
                constexpr bool isInvocable = std::is_invocable_v<Func, Args...>;

                // If you get a compile error here, then you probably didn't define the function's arguments as a const reference or as a value.
                // You also probably got a lot of other compile errors, like:
                //     'type': is not a member of 'std::invoke_result<Func, Args>'
                //     syntax error: identifier 'type'
                //     syntax error: missing '>' before identifier 'type'
                //     etc.
                // To resolve this problem, for example, if the first parameter's type is an int, then it must be either:
                //     int, const int, const int&, auto, const auto&, auto&&
                // You'll get a compile error if it's either one of these:
                //     int&, auto&, int*, const int*, int&&
                static_assert(isInvocable, "The function's parameters must be declared as either const reference or a value (see comment above).");

                return isInvocable;
            }

        private:
            static constexpr bool _isInvocable = check();

        public:
            using type = typename std::invoke_result<Func, Args...>::type;
        };

        //
        // Forward declarations
        //

        template <typename CppIterator>
        struct CppIteratorWrapper;

        template <typename IterType, typename T>
        struct StepByIter;

        template <typename IterType, typename ChainedIterType>
        struct ChainIter;

        template <typename IterType, typename ZippedIterType>
        struct ZipIter;

        template <typename IterType, typename SeparatorGetter>
        struct IntersperseWithIter;

        template <typename IterType, typename MapFunction>
        struct MapIter;

        template <typename IterType, typename FilterFunction>
        struct FilterIter;

        template <typename IterType, typename FilterMapFunction>
        struct FilterMapIter;

        template <typename IterType>
        struct PeekableIter;

        template <typename IterType, typename Predicate>
        struct SkipWhileIter;

        template <typename IterType, typename Predicate>
        struct TakeWhileIter;

        template <typename IterType>
        struct FlattenIter;

        template <typename IterType, typename InspectCallback>
        struct InspectIter;

        template <typename IterType>
        struct CycleIter;

        template <typename GeneratorFunction>
        struct GeneratorIter;

        template <typename GeneratorFunction>
        struct FiniteGeneratorIter;

        template <typename T>
        struct EmptyIter;

        //
        // Iterator base class
        //

        // Base class for all iterators.
        template <typename ConcreteIterType, typename OutType>
        struct IterBase
        {
            IterBase() = default;
            IterBase(const IterBase&) = default;
            IterBase& operator=(const IterBase&) = default;

            // Advances the iterator, and returns a pointer to the next value.
            // Returns null if there are no more elements left in the iterator.
            // The returned pointer becomes invalid if the iterator goes out of scope,
            // or if the iterator is advanced again (by calling next()).
            inline const OutType* next()
            {
                return concrete_iter()->next_internal();
            }

            //
            // Consumer functions
            //

            // Calls the provided callback on all remaining elements of the iterator.
            template <typename Callback>
            void for_each(const Callback& callback)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Callback, const OutType&>::check();

                while (const OutType* value = next())
                {
                    callback(*value);
                }
            }

            // Transforms the iterator into a collection, e.g. an std::vector, std::string, std::set, etc.
            template <typename Collection>
            Collection collect()
            {
                Collection coll{ };
                while (const OutType* value = next())
                {
                    add_to_collection(coll, *value);
                }

                return coll;
            }

            // Same as collect, but calls `reserve` on the collection before adding any elements to it.
            template <typename Collection, typename SizeType = size_t>
            Collection collect_with_size_hint(const SizeType& sizeHint)
            {
                Collection coll{ };
                coll.reserve(sizeHint);

                while (const OutType* value = next())
                {
                    add_to_collection(coll, *value);
                }

                return coll;
            }

            // Partitions the elements of the iterator based on the provided predicate callback.
            // This function creates two collections - the first collection will contain all values which
            // the predicate returned false for, the second will contain the rest (which the predicate returned true for).
            template <typename Collection, typename Predicate>
            std::pair<Collection, Collection> partition(const Predicate& predicate)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                std::pair<Collection, Collection> result = { };
                Collection* collections[2] = { &result.first, &result.second };

                for_each([&](const OutType& value)
                {
                    add_to_collection(*collections[predicate(value)], value);
                });

                return result;
            }

            // Reduces the iterator into a single value, by repeatedly calling the provided reducer function.
            // The reducer function takes two parameters:
            // The first parameter is the accumulator, which will contain the last value returned by the reducer function.
            // (when it's first called, it will be the first element of the iterator)
            // The second parameter is the current element of the iterator.
            // The reducer function must return a value, which will be passed as the first parameter to the reducer function,
            // or, for the last element, this value will be the result of the entire reduce operation.
            // For empty iterators, an empty value is returned. In all other cases, a non-empty value will be returned.
            template <typename ReduceFunction>
            std::optional<OutType> reduce(const ReduceFunction& reduceFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<ReduceFunction, const OutType&, const OutType&>::check();

                const OutType* first = next();
                if (!first)
                {
                    // no elements, return an empty value
                    return { };
                }

                OutType reduced = *first;
                while (const OutType* value = next())
                {
                    const OutType& reducedConst = reduced;
                    reduced = reduceFunction(reducedConst, *value);
                }

                return reduced;
            }

            // Similarly to `reduce`, this function also reduces the iterator to a single value,
            // the difference is that `fold` takes an initial value, instead of using the first value in the iterator.
            // Because of this, `fold` will always return a value. (If the iterator is empty, then the initial value is returned)
            template <typename T, typename FoldFunction>
            T fold(T initialValue, const FoldFunction& foldFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<FoldFunction, const OutType&, const OutType&>::check();

                while (const OutType* value = next())
                {
                    const T& currentValue = initialValue;
                    initialValue = foldFunction(currentValue, *value);
                }

                return initialValue;
            }

            // Consumes the iterator, and returns the number of elements in it.
            template <typename T = size_t>
            T count()
            {
                T count = T(0);

                while (next())
                {
                    ++count;
                }

                return count;
            }

            // Consumes the iterator, and returns its last element.
            // If the iterator is empty, then an empty value is returned.
            std::optional<OutType> last()
            {
                const OutType* first = next();
                if (!first)
                {
                    // no elements, return an empty value
                    return { };
                }

                OutType last = *first;
                while (const OutType* value = next())
                {
                    last = *value;
                }

                return last;
            }

            // Returns the element at the given index, by advancing the iterator idx + 1 times.
            // If the iterator doesn't have enough elements, then an empty value is returned.
            template <typename T>
            std::optional<OutType> nth(T idx)
            {
                T currentIdx = T(0);
                while (const OutType* value = next())
                {
                    if (currentIdx++ == idx)
                    {
                        return *value;
                    }
                }

                return { };
            }

            // Calls the provided predicate on all elements of the iterator, and checks if all returned values are true.
            // The process will stop at the first false value, so this function might not fully consume the iterator.
            // For empty iterators, this function returns true.
            template <typename Predicate>
            bool all(const Predicate& predicate)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                while (const OutType* value = next())
                {
                    if (!predicate(*value))
                    {
                        return false;
                    }
                }

                return true;
            }

            // Calls the provided predicate on all elements of the iterator, and checks if any of the returned values is true.
            // The process will stop at the first true value, so this function might not fully consume the iterator.
            // For empty iterators, this function returns false.
            template <typename Predicate>
            bool any(const Predicate& predicate)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                while (const OutType* value = next())
                {
                    if (predicate(*value))
                    {
                        return true;
                    }
                }

                return false;
            }

            // Advances the iterator until a value that satisfies the predicate is found.
            // The process will stop at the first such value, so this function might not fully consume the iterator.
            // If no element was found, then an empty value is returned.
            template <typename Predicate>
            std::optional<OutType> find(const Predicate& predicate)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                while (const OutType* value = next())
                {
                    if (predicate(*value))
                    {
                        return *value;
                    }
                }

                return { };
            }

            // Advances the iterator until a value that satisfies the predicate is found, and returns its index.
            // The process will stop at the first such value, so this function might not fully consume the iterator.
            // If no element was found, then an empty value is returned.
            template <typename Predicate, typename Position = size_t>
            std::optional<Position> position(const Predicate& predicate)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                Position pos = Position(0);
                while (const OutType* value = next())
                {
                    if (predicate(*value))
                    {
                        return pos;
                    }

                    ++pos;
                }

                return { };
            }

            // Returns the minimum value in the iterator, comparing elements with the < operator.
            // If there are multiple minimum values, then the first one is returned.
            // If the iterator is empty, then an empty value is returned.
            std::optional<OutType> min()
            {
                return min_by(compare<OutType>);
            }

            // Returns the minimum value in the iterator, comparing elements with the provided comparer function.
            // If there are multiple minimum values, then the first one is returned.
            // If the iterator is empty, then an empty value is returned.
            template <typename Comparer>
            std::optional<OutType> min_by(const Comparer& comparer)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Comparer, const OutType&, const OutType&>::check();

                const OutType* first = next();
                if (!first)
                {
                    return { };
                }

                OutType minValue = *first;
                while (const OutType* value = next())
                {
                    const OutType& minValueConst = minValue;
                    if (comparer(*value, minValueConst) < 0)
                    {
                        minValue = *value;
                    }
                }

                return minValue;
            }

            // Returns the maximum value in the iterator, comparing elements with the > operator.
            // If there are multiple maximum values, then the first one is returned.
            // If the iterator is empty, then an empty value is returned.
            std::optional<OutType> max()
            {
                return max_by(compare<OutType>);
            }

            // Returns the maximum value in the iterator, comparing elements with the provided comparer function.
            // If there are multiple maximum values, then the first one is returned.
            // If the iterator is empty, then an empty value is returned.
            template <typename Comparer>
            std::optional<OutType> max_by(const Comparer& comparer)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Comparer, const OutType&, const OutType&>::check();

                const OutType* first = next();
                if (!first)
                {
                    return { };
                }

                OutType maxValue = *first;
                while (const OutType* value = next())
                {
                    const OutType& maxValueConst = maxValue;
                    if (comparer(*value, maxValueConst) > 0)
                    {
                        maxValue = *value;
                    }
                }

                return maxValue;
            }

            // Returns the sum of all elements in the iterator, by adding all elements together.
            // If the iterator is empty, then 0 is returned.
            template <typename T = OutType>
            T sum()
            {
                T sum = T(0);

                while (const OutType* value = next())
                {
                    sum += *value;
                }

                return sum;
            }

            // Returns the product of all elements in the iterator, by multiplying all elements together.
            // If the iterator is empty, then 1 is returned.
            template <typename T = OutType>
            T product()
            {
                T product = T(1);

                while (const OutType* value = next())
                {
                    product *= *value;
                }

                return product;
            }

            // Returns true if the iterator is sorted ascending, so that no element is less than the previous one.
            // The process will stop when a non-sorted pair is encountered, so this function might not fully consume the iterator.
            // For iterators with less than 2 elements, this function always returns true.
            bool is_sorted_ascending()
            {
                return is_sorted_by(compare<OutType>);
            }

            // Returns true if the iterator is sorted descending, so that no element is greater than the previous one.
            // The process will stop when a non-sorted pair is encountered, so this function might not fully consume the iterator.
            // For iterators with less than 2 elements, this function always returns true.
            bool is_sorted_descending()
            {
                return is_sorted_by(compare_reverse<OutType>);
            }

            // Returns true if the iterator is sorted by the given comparer.
            // The comparer function must return:
            // - a negative value if the first compared value is less than the second,
            // - a positive value if the first compared value is greater than the second,
            // - zero if they are equal
            // The process will stop when a non-sorted pair is encountered, so this function might not fully consume the iterator.
            // For iterators with less than 2 elements, this function always returns true.
            template <typename Comparer>
            bool is_sorted_by(const Comparer& comparer)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Comparer, const OutType&, const OutType&>::check();

                const OutType* first = next();
                if (!first)
                {
                    // no elements, return true
                    return true;
                }

                OutType prev = *first;
                while (const OutType* value = next())
                {
                    const OutType& prevConst = prev;
                    if (comparer(prevConst, *value) > 0)
                    {
                        // not sorted, return false immediately
                        return false;
                    }

                    prev = *value;
                }

                return true;
            }

            //
            // Comparison functions
            //

            // Lexicographically compares the elements of this iterator with another iterator's elements, using the provided comparison function.
            // The comparison function must be a partial comparer, which means that it can return an empty value if the elements cannot be compared.
            // If the elements can be compared, then the usual <0, 0, and >0 must be returned.
            // Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.
            // Returns an empty value if there was at least one pair of elements that could not be compared.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType, typename PartialComparisonFunction>
            std::optional<char> partial_cmp_by(OtherIterType&& other, const PartialComparisonFunction& partialComparisonFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<PartialComparisonFunction, const OutType&, const OutType&>::check();

                while (true)
                {
                    const OutType* thisValue = next();
                    const OutType* otherValue = other.next();

                    if (!thisValue)
                    {
                        if (otherValue)
                        {
                            // current iterator has finished before the other, so this < other
                            return -1;
                        }
                        else
                        {
                            // both iterators have finished at the same time, so this == other
                            return 0;
                        }
                    }

                    if (!otherValue)
                    {
                        // other iterator has finished before the current, so this > other
                        return 1;
                    }

                    // none of the iterators have finished yet, compare the values
                    std::optional<char> compared = partialComparisonFunction(*thisValue, *otherValue);
                    if (!compared || *compared != 0)
                    {
                        // values are different or cannot be compared, return the result
                        return compared;
                    }
                    else
                    {
                        // values are equal, check the next ones
                    }
                }
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements, using the provided comparison function.
            // Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.
            // Returns an empty value if there was at least one pair of elements that could not be compared.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            std::optional<char> partial_cmp(OtherIterType&& other)
            {
                return partial_cmp_by(other, compare_partial<OutType>);
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements, using the provided comparison function.
            // Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType, typename ComparisonFunction>
            char cmp_by(OtherIterType&& other, const ComparisonFunction& comparisonFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<ComparisonFunction, const OutType&, const OutType&>::check();

                auto partialComparisonFunction = [&](const OutType& a, const OutType& b) -> std::optional<OutType>
                {
                    return comparisonFunction(a, b);
                };

                // we always return a non-empty value in our comparer, so we can just get the value
                return *partial_cmp_by(other, partialComparisonFunction);
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements.
            // Returns -1 if this iterator is less than the other iterator, 0 if they are equal, and 1 if this iterator is greater than the other iterator.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            char cmp(OtherIterType&& other)
            {
                return cmp_by(other, compare<OutType>);
            }

            // Determines if the elements of this iterator are equal to the elements of another iterator, using the provided equality comparer function.
            // Returns true only if both iterators have the same number of elements, and all elements are equal.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType, typename EqualityComparerFunction>
            bool eq_by(OtherIterType&& other, const EqualityComparerFunction& equalityComparerFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<EqualityComparerFunction, const OutType&, const OutType&>::check();

                while (true)
                {
                    const OutType* thisValue = next();
                    const OutType* otherValue = other.next();

                    if (!thisValue)
                    {
                        // this iterator has finished, the two iterators can only be equal if the other iterator has finished too
                        return !otherValue;
                    }

                    if (!otherValue)
                    {
                        // other iterator has finished before the current, so they cannot be equal
                        return false;
                    }

                    // none of the iterators have finished yet, check equality
                    if (!equalityComparerFunction(*thisValue, *otherValue))
                    {
                        // values are different
                        return false;
                    }
                    else
                    {
                        // values are equal, check the next ones
                    }
                }
            }

            // Determines if the elements of this iterator are equal to the elements of another iterator.
            // Returns true only if both iterators have the same number of elements, and all elements are equal.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            bool eq(OtherIterType&& other)
            {
                return eq_by(other, [](const OutType& a, const OutType& b) { return a == b; });
            }

            // Determines if the elements of this iterator are not equal to the elements of another iterator.
            // Returns true if the iterators have a different number of elements, or if there is at least one pair of elements that are different.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            bool ne(OtherIterType&& other)
            {
                return !eq(other);
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements.
            // Returns true if this iterator is less than the other iterator, false otherwise.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            bool lt(OtherIterType&& other)
            {
                std::optional<char> cmp = partial_cmp(other);
                return cmp && *cmp < 0;
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements.
            // Returns true if this iterator is less than or equal to the other iterator, false otherwise.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            bool le(OtherIterType&& other)
            {
                std::optional<char> cmp = partial_cmp(other);
                return cmp && *cmp <= 0;
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements.
            // Returns true if this iterator is greater than the other iterator, false otherwise.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            bool gt(OtherIterType&& other)
            {
                std::optional<char> cmp = partial_cmp(other);
                return cmp && *cmp > 0;
            }

            // Lexicographically compares the elements of this iterator with another iterator's elements.
            // Returns true if this iterator is greater than or equal to the other iterator, false otherwise.
            // Both iterators are advanced until their value becomes different, or until either of them has no more elements left.
            template <typename OtherIterType>
            bool ge(OtherIterType&& other)
            {
                std::optional<char> cmp = partial_cmp(other);
                return cmp && *cmp >= 0;
            }

            //
            // Iterator functions
            //

            // Creates an iterator that steps multiple times with each iteration, by the given step amount.
            // Using 1 as the step value will produce identical behavior to the original iterator.
            // Using step values that are less than 1 are invalid, and will create an empty iterator.
            template <typename T>
            StepByIter<ConcreteIterType, T> step_by(const T& stepSize)
            {
                return StepByIter<ConcreteIterType, T>(*concrete_iter(), stepSize);
            }

            // Appends an iterator to the end of the current iterator.
            // This new iterator will yield elements from the first iterator until it finishes, then from the second iterator.
            template <typename ChainedIterType>
            ChainIter<ConcreteIterType, ChainedIterType> chain(const ChainedIterType& chainedIter)
            {
                return ChainIter<ConcreteIterType, ChainedIterType>(*concrete_iter(), chainedIter);
            }

            // Creates an iterator that iterates over two iterators in parallel.
            // This new iterator will yield a pair of elements,
            // the first element is from the first iterator, the second element is from the second.
            // This iterator will stop yielding elements when any of the iterators are finished.
            template <typename ZippedIterType>
            ZipIter<ConcreteIterType, ZippedIterType> zip(const ZippedIterType& zippedIter)
            {
                return ZipIter<ConcreteIterType, ZippedIterType>(*concrete_iter(), zippedIter);
            }

            // Creates an iterator that inserts a separator value between each element.
            // The separator will not be inserted before the first element, nor after the last element.
            IntersperseWithIter<ConcreteIterType, Getter<OutType>> intersperse(const OutType& separator)
            {
                return IntersperseWithIter<ConcreteIterType, Getter<OutType>>(*concrete_iter(), Getter<OutType>(separator));
            }

            // Creates an iterator that inserts a separator value between each element.
            // The current separator value is obtained by calling the provided separatorGetter function.
            // The separator will not be inserted before the first element, nor after the last element.
            template <typename SeparatorGetter>
            IntersperseWithIter<ConcreteIterType, SeparatorGetter> intersperse_with(const SeparatorGetter& separatorGetter)
            {
                return IntersperseWithIter<ConcreteIterType, SeparatorGetter>(*concrete_iter(), separatorGetter);
            }

            // Transforms all values in the iterator, replacing each element with a new one.
            // The provided map function takes the old value, and returns the new value.
            // The old and the new values can have different types.
            template <typename MapFunction>
            MapIter<ConcreteIterType, MapFunction> map(const MapFunction& mapFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<MapFunction, const OutType&>::check();

                return MapIter<ConcreteIterType, MapFunction>(*concrete_iter(), mapFunction);
            }

            // Creates an iterator that only yields elements that satisfy the filter function.
            // Elements that the filter function returns false for are skipped, the rest are kept.
            template <typename FilterFunction>
            FilterIter<ConcreteIterType, FilterFunction> filter(const FilterFunction& filterFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<FilterFunction, const OutType&>::check();

                return FilterIter<ConcreteIterType, FilterFunction>(*concrete_iter(), filterFunction);
            }

            // Creates an iterator that filters and maps at the same time.
            // The provided filterMap function takes the old value, and must return a non-empty std::optional value
            // if the current element should be kept (and transformed),
            // and must return an empty value if the element should be skipped.
            template <typename FilterMapFunction>
            FilterMapIter<ConcreteIterType, FilterMapFunction> filter_map(const FilterMapFunction& filterMapFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<FilterMapFunction, const OutType&>::check();

                return FilterMapIter<ConcreteIterType, FilterMapFunction>(*concrete_iter(), filterMapFunction);
            }

            // Creates an iterator which has an extra `peek` function, which allows you
            // to retrieve the next value in the iterator without advancing the current iterator.
            // Note that the underlying iterator is still advanced to get the next value when `peek` is called.
            PeekableIter<ConcreteIterType> peekable()
            {
                return PeekableIter<ConcreteIterType>(*concrete_iter());
            }

            // Creates an iterator that skips elements while the given predicate returns true.
            // When the predicate returns false for the first time, then no more items will be skipped.
            template <typename Predicate>
            SkipWhileIter<ConcreteIterType, Predicate> skip_while(const Predicate& pred)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                return SkipWhileIter<ConcreteIterType, Predicate>(*concrete_iter(), pred);
            }

            // Creates an iterator that yields elements only while the given predicate returns true.
            // When the predicate returns false for the first time, then the rest of the elements are skipped.
            template <typename Predicate>
            TakeWhileIter<ConcreteIterType, Predicate> take_while(const Predicate& pred)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<Predicate, const OutType&>::check();

                return TakeWhileIter<ConcreteIterType, Predicate>(*concrete_iter(), pred);
            }

            // Creates an iterator which skips the given number of elements,
            // by advancing the underlying iterator that many times.
            // If the underlying iterator is too short, then this function creates an empty iterator.
            template <typename T = size_t>
            SkipWhileIter<ConcreteIterType, Counter<T>> skip(const T& count)
            {
                return SkipWhileIter<ConcreteIterType, Counter<T>>(*concrete_iter(), Counter<T>(count));
            }

            // Creates an iterator which yields the given number of elements at most.
            // If the underlying iterator is too short, then less elements will be yielded.
            template <typename T = size_t>
            TakeWhileIter<ConcreteIterType, Counter<T>> take(const T& count)
            {
                return TakeWhileIter<ConcreteIterType, Counter<T>>(*concrete_iter(), Counter<T>(count));
            }

            // Creates an iterator which yields the current element as well as its index in the current iterator.
            // The new iterator returns pairs, where the first element is the index, and the second element is the value.
            template <typename T = size_t>
            ZipIter<GeneratorIter<InfiniteRangeGenerator<T>>, ConcreteIterType> enumerate()
            {
                using Generator = InfiniteRangeGenerator<T>;
                using GeneratorIterType = GeneratorIter<Generator>;
                return ZipIter<GeneratorIterType, ConcreteIterType>(GeneratorIterType(Generator(0, 1)), *concrete_iter());
            }

            // Creates an iterator from an iterator of iterators, which removes one level of nesting.
            // Note that this function can only be called on nested iterators.
            FlattenIter<ConcreteIterType> flatten()
            {
                return FlattenIter<ConcreteIterType>(*concrete_iter());
            }

            // Creates an iterator that will call the provided inspect function on each value before yielding them.
            // The inspect function cannot change those values, only read (inspect) them.
            // This function can be useful for debugging.
            template <typename InspectFunction>
            InspectIter<ConcreteIterType, InspectFunction> inspect(const InspectFunction& inspectFunction)
            {
                constexpr bool typeCheck = ReturnTypeHelperConstRefOrValue<InspectFunction, const OutType&>::check();

                return InspectIter<ConcreteIterType, InspectFunction>(*concrete_iter(), inspectFunction);
            }

            // Creates an iterator that repeats the current iterator endlessly.
            // When that iterator is finshed, it will start again at the beginning, instead of finishing.
            // If the current iterator is empty, then the cycled iterator will also be empty.
            CycleIter<ConcreteIterType> cycle()
            {
                return CycleIter<ConcreteIterType>(*concrete_iter());
            }

            //
            // C++ iterator functionality
            //

            struct CppIterator
            {
                using value_type = OutType;
                using pointer = value_type*;
                using reference = value_type&;

                CppIterator(IterBase* iter) : _iter(iter), _value()
                {
                    step();
                }

                CppIterator() : _iter(nullptr), _value()
                {
                }

                ~CppIterator()
                {
                    _iter = nullptr;
                    _value.reset();
                }

                const OutType& operator*()
                {
                    return *_value;
                }

                CppIterator& operator++()
                {
                    step();
                    return *this;
                }

                CppIterator operator++(int)
                {
                    CppIterator prev = *this;
                    step();
                    return prev;
                }

                const OutType* operator->() const
                {
                    if (_value)
                    {
                        return &*_value;
                    }
                    else
                    {
                        return nullptr;
                    }
                }

                friend bool operator==(const CppIterator& a, const CppIterator& b)
                {
                    return a._iter == b._iter && a.is_done() && b.is_done();
                }

                friend bool operator!=(const CppIterator& a, const CppIterator& b)
                {
                    return !(a == b);
                }

            private:
                void step()
                {
                    if (!_iter)
                    {
                        // the iterator has already finished
                        return;
                    }

                    const OutType* first = _iter->next();
                    if (first)
                    {
                        _value = *first;
                    }
                    else
                    {
                        // no more elements left
                        _iter = nullptr;
                        _value.reset();
                    }
                }

                inline bool is_done() const
                {
                    return !_value;
                }

                IterBase* _iter;
                std::optional<OutType> _value;
            };

            CppIterator begin()
            {
                return CppIterator(this);
            }

            CppIterator end()
            {
                return CppIterator();
            }

        private:
            inline ConcreteIterType* concrete_iter()
            {
                return static_cast<ConcreteIterType*>(this);
            }
        };

        //
        // Iterator implementations
        //

        template <typename CppIterType>
        struct CppIteratorWrapper : public IterBase<CppIteratorWrapper<CppIterType>, typename CppIterType::value_type>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using OutType = typename CppIterType::value_type;

            CppIteratorWrapper(const CppIterType& begin, const CppIterType& end) : _begin(begin), _end(end)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (_begin == _end)
                {
                    return nullptr;
                }
                else
                {
                    const OutType* current = &*_begin;
                    ++_begin;
                    return current;
                }
            }

            CppIterType _begin;
            CppIterType _end;
        };

        template <typename IterType, typename T>
        struct StepByIter : public IterBase<StepByIter<IterType, T>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            StepByIter(const IterType& iter, const T& stepSize) : _iter(iter), _stepSize(stepSize), _first(true), _invalid(stepSize <= 0)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (_invalid)
                {
                    return nullptr;
                }

                if (_first)
                {
                    _first = false;
                    return _iter.next();
                }
                else
                {
                    const OutType* last = nullptr;
                    for (T i = 0; i < _stepSize; ++i)
                    {
                        last = _iter.next();
                    }

                    return last;
                }
            }

            IterType _iter;
            T _stepSize;
            bool _first;
            bool _invalid;
        };

        template <typename IterType, typename ChainedIterType>
        struct ChainIter : public IterBase<ChainIter<IterType, ChainedIterType>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            ChainIter(const IterType& iter, const ChainedIterType& chainedIter) : _iter(iter), _chainedIter(chainedIter), _firstDone(false)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (!_firstDone)
                {
                    const OutType* value = _iter.next();
                    if (value != nullptr)
                    {
                        return value;
                    }
                    else
                    {
                        _firstDone = true;
                    }
                }

                return _chainedIter.next();
            }

            IterType _iter;
            ChainedIterType _chainedIter;
            bool _firstDone;
        };

        template <typename IterType, typename ZippedIterType>
        struct ZipIter : public IterBase<ZipIter<IterType, ZippedIterType>, std::pair<typename IterType::OutType, typename ZippedIterType::OutType>>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = std::pair<InType, typename ZippedIterType::OutType>;

            ZipIter(const IterType& iter, const ZippedIterType& zippedIter) : _iter(iter), _zippedIter(zippedIter), _tmpResult(), _anyDone(false)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (_anyDone)
                {
                    return nullptr;
                }

                const InType* value = _iter.next();
                const typename ZippedIterType::OutType* zippedValue = _zippedIter.next();

                if (value == nullptr || zippedValue == nullptr)
                {
                    // return null if any of the iterators are finished
                    _anyDone = true;
                    return nullptr;
                }

                _tmpResult = { *value, *zippedValue };
                return &_tmpResult;
            }

            IterType _iter;
            ZippedIterType _zippedIter;
            OutType _tmpResult;
            bool _anyDone;
        };

        template <typename IterType, typename SeparatorGetter>
        struct IntersperseWithIter : public IterBase<IntersperseWithIter<IterType, SeparatorGetter>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            IntersperseWithIter(const IterType& iter, const SeparatorGetter& separatorGetter) :
                _iter(iter), _separatorGetter(separatorGetter), _tmpResult(), _nextResult(), _separatorIsNext(false)
            {
                if (const OutType* next = _iter.next())
                {
                    // get the next result immediately, so that we can know if the iterator is empty or not
                    _nextResult = *next;
                }
            }

        private:
            const OutType* next_internal()
            {
                if (_separatorIsNext)
                {
                    _separatorIsNext = false;
                    _tmpSeparator = _separatorGetter();
                    return &*_tmpSeparator;
                }
                else
                {
                    if (_nextResult)
                    {
                        _tmpResult = *_nextResult;

                        // check if we have a next element
                        // if yes, then we need to insert a separator in the next call

                        if (const OutType* next = _iter.next())
                        {
                            _nextResult = *next;
                            _separatorIsNext = true;
                        }
                        else
                        {
                            _nextResult.reset();
                        }

                        return &*_tmpResult;
                    }

                    return nullptr;
                }
            }

            IterType _iter;
            SeparatorGetter _separatorGetter;
            std::optional<OutType> _tmpSeparator;
            std::optional<OutType> _tmpResult;
            std::optional<OutType> _nextResult;
            bool _separatorIsNext;
        };

        template <typename IterType, typename MapFunction>
        struct MapIter : public IterBase<MapIter<IterType, MapFunction>, typename ReturnTypeHelperConstRefOrValue<MapFunction, typename IterType::OutType>::type>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = typename ReturnTypeHelperConstRefOrValue<MapFunction, InType>::type;

            MapIter(const IterType& iter, const MapFunction& mapFunction) : _iter(iter), _mapFunction(mapFunction), _tmpResult()
            {
            }

        private:
            const OutType* next_internal()
            {
                if (const InType* current = _iter.next())
                {
                    _tmpResult = _mapFunction(*current);
                    return &*_tmpResult;
                }
                else
                {
                    return nullptr;
                }
            }

            IterType _iter;
            MapFunction _mapFunction;
            std::optional<OutType> _tmpResult;
        };

        template <typename IterType, typename FilterFunction>
        struct FilterIter : public IterBase<FilterIter<IterType, FilterFunction>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            FilterIter(const IterType& iter, const FilterFunction& filterFunction) : _iter(iter), _filterFunction(filterFunction)
            {
            }

        private:
            const OutType* next_internal()
            {
                while (const InType* current = _iter.next())
                {
                    if (_filterFunction(*current))
                    {
                        return current;
                    }
                }

                return nullptr;
            }

            IterType _iter;
            FilterFunction _filterFunction;
        };

        // if you get a compile error here, then it's likely that your filterMapFunction doesn't return an std::optional value
        template <typename IterType, typename FilterMapFunction>
        struct FilterMapIter : public IterBase<FilterMapIter<IterType, FilterMapFunction>, typename ReturnTypeHelperConstRefOrValue<FilterMapFunction, typename IterType::OutType>::type::value_type>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = typename ReturnTypeHelperConstRefOrValue<FilterMapFunction, InType>::type::value_type;

            FilterMapIter(const IterType& iter, const FilterMapFunction& filterMapFunction) : _iter(iter), _filterMapFunction(filterMapFunction), _tmpResult()
            {
            }

        private:
            const OutType* next_internal()
            {
                while (const InType* current = _iter.next())
                {
                    std::optional<OutType> result = _filterMapFunction(*current);
                    if (result)
                    {
                        _tmpResult = *result;
                        return &_tmpResult;
                    }
                }

                return nullptr;
            }

            IterType _iter;
            FilterMapFunction _filterMapFunction;
            OutType _tmpResult;
        };

        template <typename IterType>
        struct PeekableIter : public IterBase<PeekableIter<IterType>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            PeekableIter(const IterType& iter) : _iter(iter), _nextItem(), _tmpResult()
            {
            }

            // Returns the next item, without advancing the iterator.
            // Returns null if there are no more elements.
            const OutType* peek()
            {
                if (!_nextItem)
                {
                    // next item is unknown, get it from the iter
                    if (const OutType* next = _iter.next())
                    {
                        // iter still has values
                        _nextItem.emplace(*next);
                    }
                    else
                    {
                        // iter has finished, no more values
                        _nextItem.reset();
                        return nullptr;
                    }
                }

                std::optional<OutType>& nextValue = *_nextItem;
                if (nextValue)
                {
                    // next item is known, and has value
                    return &*nextValue;
                }
                else
                {
                    // next item is known, but has no value
                    return nullptr;
                }
            }

        private:
            const OutType* next_internal()
            {
                if (_nextItem)
                {
                    // next item was already peeked

                    _tmpResult = *_nextItem;
                    _nextItem.reset();

                    if (_tmpResult)
                    {
                        return &*_tmpResult;
                    }
                    else
                    {
                        return nullptr;
                    }
                }

                // next item was not peeked, just return the next item from the iter
                return _iter.next();
            }

            IterType _iter;
            std::optional<std::optional<OutType>> _nextItem;
            std::optional<OutType> _tmpResult;
        };

        template <typename IterType, typename Predicate>
        struct SkipWhileIter : public IterBase<SkipWhileIter<IterType, Predicate>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            SkipWhileIter(const IterType& iter, const Predicate& predicate) : _iter(iter), _predicate(predicate), _done(false)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (!_done)
                {
                    _done = true;

                    while (const OutType* value = _iter.next())
                    {
                        if (!_predicate(*value))
                        {
                            return value;
                        }
                    }

                    return nullptr;
                }

                return _iter.next();
            }

            IterType _iter;
            Predicate _predicate;
            bool _done;
        };

        template <typename IterType, typename Predicate>
        struct TakeWhileIter : public IterBase<TakeWhileIter<IterType, Predicate>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            TakeWhileIter(const IterType& iter, const Predicate& predicate) : _iter(iter), _predicate(predicate), _done(false)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (_done)
                {
                    return nullptr;
                }

                if (const OutType* value = _iter.next())
                {
                    if (_predicate(*value))
                    {
                        return value;
                    }
                }

                _done = true;
                return nullptr;
            }

            IterType _iter;
            Predicate _predicate;
            bool _done;
        };

        template <typename IterType>
        struct FlattenIter : IterBase<FlattenIter<IterType>, typename IterType::OutType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InnerIterType = typename IterType::OutType;
            using InType = IterType;
            using OutType = typename InnerIterType::OutType;

            FlattenIter(const IterType& iter) : _iter(iter), _innerIter()
            {
                advance();
            }

        private:
            const OutType* next_internal()
            {
                if (!_innerIter)
                {
                    return nullptr;
                }

                while (true)
                {
                    if (const OutType* value = _innerIter->next())
                    {
                        return value;
                    }
                    else
                    {
                        if (!advance())
                        {
                            return nullptr;
                        }
                    }
                }
            }

            bool advance()
            {
                if (const InnerIterType* inner = _iter.next())
                {
                    _innerIter = *inner;
                    return true;
                }
                else
                {
                    _innerIter.reset();
                    return false;
                }
            }

            IterType _iter;
            std::optional<InnerIterType> _innerIter;
        };

        template <typename IterType, typename InspectCallback>
        struct InspectIter : public IterBase<InspectIter<IterType, InspectCallback>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            InspectIter(const IterType& iter, const InspectCallback& inspectCallback) : _iter(iter), _inspectCallback(inspectCallback)
            {
            }

        private:
            const OutType* next_internal()
            {
                const OutType* value = _iter.next();
                if (value)
                {
                    _inspectCallback(*value);
                }

                return value;
            }

            IterType _iter;
            InspectCallback _inspectCallback;
        };

        template <typename IterType>
        struct CycleIter : public IterBase<CycleIter<IterType>, typename IterType::OutType>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = typename IterType::OutType;
            using OutType = InType;

            CycleIter(const IterType& iter) : _originalIter(iter), _iter(iter), _iterIsEmpty(true)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (const OutType* value = _iter.next())
                {
                    _iterIsEmpty = false;
                    return value;
                }

                if (_iterIsEmpty)
                {
                    return nullptr;
                }

                // lambdas don't have a copy assignment operator, so if there is a lambda in the iterator chain,
                // then this will cause a compiler error:
                // _iter = _originalIter;
                // instead, we have to use the copy constructor, which works just fine (also call the destructor before)
                _iter.~IterType();
                new (&_iter) IterType(_originalIter);

                return _iter.next();
            }

            IterType _originalIter;
            IterType _iter;
            bool _iterIsEmpty;
        };

        template <typename GeneratorFunction>
        struct GeneratorIter : public IterBase<GeneratorIter<GeneratorFunction>, typename std::invoke_result<GeneratorFunction>::type>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using OutType = typename std::invoke_result<GeneratorFunction>::type;

            GeneratorIter(const GeneratorFunction& generatorFunction) : _generatorFunction(generatorFunction), _tmpResult()
            {
            }

        private:
            const OutType* next_internal()
            {
                _tmpResult = _generatorFunction();
                return &_tmpResult;
            }

            GeneratorFunction _generatorFunction;
            OutType _tmpResult;
        };

        template <typename GeneratorFunction>
        struct FiniteGeneratorIter : public IterBase<FiniteGeneratorIter<GeneratorFunction>, typename std::invoke_result<GeneratorFunction>::type::value_type>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using OutType = typename std::invoke_result<GeneratorFunction>::type::value_type;

            FiniteGeneratorIter(const GeneratorFunction& generatorFunction) : _generatorFunction(generatorFunction), _tmpResult(), _done(false)
            {
            }

        private:
            const OutType* next_internal()
            {
                if (_done)
                {
                    return nullptr;
                }

                if (std::optional<OutType> value = _generatorFunction())
                {
                    _tmpResult = *value;
                    return &_tmpResult;
                }
                else
                {
                    _done = true;
                    return nullptr;
                }
            }

            GeneratorFunction _generatorFunction;
            OutType _tmpResult;
            bool _done;
        };

        template <typename T>
        struct EmptyIter : public IterBase<EmptyIter<T>, T>
        {
            template <typename ConcreteIterType, typename OutType>
            friend struct IterBase;

            using InType = T;
            using OutType = T;

            EmptyIter()
            {
            }

        private:
            const OutType* next_internal()
            {
                return nullptr;
            }
        };
    }

    //
    // Iterator creator functions
    //

    // Creates an iterator from C++ iterators.
    template <typename Iter>
    detail::CppIteratorWrapper<Iter> iter(const Iter& begin, const Iter& end)
    {
        return detail::CppIteratorWrapper<Iter>(begin, end);
    }

    // Creates an iterator from a collection, e.g. std::vector, std::string, std::set, etc.
    // Equivalent to <c>iter(collection.begin(), collection.end())</c>.
    template <typename Collection>
    detail::CppIteratorWrapper<typename Collection::const_iterator> iter(const Collection& collection)
    {
        return iter(collection.begin(), collection.end());
    }

    // Creates an infinite iterator which yields elements by repeatedly calling the provided generator function.
    template <typename GeneratorFunction>
    detail::GeneratorIter<GeneratorFunction> infinite_generator(const GeneratorFunction& generatorFunction)
    {
        return detail::GeneratorIter<GeneratorFunction>(generatorFunction);
    }

    // Same as `generator`, but with the name used in Rust.
    template <typename GeneratorFunction>
    detail::GeneratorIter<GeneratorFunction> repeat_with(const GeneratorFunction& generatorFunction)
    {
        return generator(generatorFunction);
    }

    // Creates a finite iterator, which yields elements by repeatedly calling the provided generator function.
    // The generator function must return an std::optional value.
    // The iterator runs until the generator returns an empty value (nullopt).
    template <typename GeneratorFunction>
    detail::FiniteGeneratorIter<GeneratorFunction> finite_generator(const GeneratorFunction& generatorFunction)
    {
        return detail::FiniteGeneratorIter<GeneratorFunction>(generatorFunction);
    }

    // Same as `finite_generator`, but with the name used in Rust.
    template <typename GeneratorFunction>
    detail::FiniteGeneratorIter<GeneratorFunction> from_fn(const GeneratorFunction& generatorFunction)
    {
        return finite_generator(generatorFunction);
    }

    // Creates an infinite iterator, which starts with the provided `min` value,
    // increasing the value by 1 every iteration.
    template <typename T>
    detail::GeneratorIter<detail::InfiniteRangeGenerator<T>> infinite_range(const T& min)
    {
        return infinite_generator(detail::InfiniteRangeGenerator<T>(min, T(1)));
    }

    // Creates an infinite iterator, which starts with the provided `min` value,
    // increasing the value by the provided `step` value every iteration.
    template <typename T>
    detail::GeneratorIter<detail::InfiniteRangeGenerator<T>> infinite_range(const T& min, const T& step)
    {
        return infinite_generator(detail::InfiniteRangeGenerator<T>(min, step));
    }

    // Creates an iterator, which starts with the provided `min` value,
    // increasing the value by 1 until it reaches the `max` (exclusive) value.
    template <typename T>
    detail::FiniteGeneratorIter<detail::FiniteRangeGenerator<T, false>> range(const T& min, const T& max)
    {
        return finite_generator(detail::FiniteRangeGenerator<T, false>(min, max, T(1)));
    }

    // Creates an iterator, which starts with the provided `min` value,
    // increasing the value by the provided `step` value,
    // until it reaches the `max` (exclusive) value.
    // Negative step values will create an infinite iterator (this is not the intended use for this iterator).
    // You should use the `infinite_range` function instead.
    template <typename T>
    detail::FiniteGeneratorIter<detail::FiniteRangeGenerator<T, false>> range(const T& min, const T& max, const T& step)
    {
        return finite_generator(detail::FiniteRangeGenerator<T, false>(min, max, step));
    }

    // Creates an iterator, which starts with the provided `min` value,
    // increasing the value by 1 until it reaches the `max` (inclusive) value.
    template <typename T>
    detail::FiniteGeneratorIter<detail::FiniteRangeGenerator<T, true>> range_inclusive(const T& min, const T& max)
    {
        return finite_generator(detail::FiniteRangeGenerator<T, true>(min, max, T(1)));
    }

    // Creates an iterator, which starts with the provided `min` value,
    // increasing the value by the provided `step` value,
    // until it reaches the `max` (inclusive) value.
    // Negative step values will create an infinite iterator (this is not the intended use for this iterator).
    // You should use the `infiniteRange` function instead.
    template <typename T>
    detail::FiniteGeneratorIter<detail::FiniteRangeGenerator<T, true>> range_inclusive(const T& min, const T& max, const T& step)
    {
        return finite_generator(detail::FiniteRangeGenerator<T, true>(min, max, step));
    }

    // Creates an iterator that yields no values.
    template <typename T>
    detail::EmptyIter<T> empty()
    {
        return detail::EmptyIter<T>();
    }

    // Creates an iterator that yields the provided value, only once.
    template <typename T>
    detail::FiniteGeneratorIter<detail::YieldOnce<T>> once(const T& value)
    {
        return finite_generator(detail::YieldOnce<T>(value));
    }

    // Creates an iterator that yields the value returned by the provided function, only once.
    template <typename ValueCalculatorFunction>
    detail::FiniteGeneratorIter<detail::YieldOnceWith<ValueCalculatorFunction>> once_with(const ValueCalculatorFunction& valueCalculatorFunction)
    {
        return finite_generator(detail::YieldOnceWith<ValueCalculatorFunction>(valueCalculatorFunction));
    }

    // Creates an iterator that yields the provided value, never stopping.
    template <typename T>
    detail::GeneratorIter<detail::YieldValue<T>> repeat(const T& value)
    {
        return infinite_generator(detail::YieldValue<T>(value));
    }

    // Creates an iterator where each element is calculated from the previous one, using the provided function.
    // The iterator starts with the given first element (if not empty), and calls the provided function to calculate the next element.
    // The function must return an std::optional value.
    // The iterator stops when the provided function returns an empty value.
    // Unfortunately, because the first parameter is an optional, the compiler cannot deduce T automatically, so you must
    // either specify the template arguments manually, or explicitly pass an std::optional value as the first parameter.
    // (for example, you can write `rusty::successors<int>(1, ...);` or `rusty::successors(std::optional(1), ...);`)
    // If you know a solution for this, please tell me :)
    template <typename T, typename SuccessorCalculatorFunction>
    detail::FiniteGeneratorIter<detail::SuccessorCalculator<T, SuccessorCalculatorFunction>> successors(const std::optional<T>& initialValue, const SuccessorCalculatorFunction& successorCalculatorFunction)
    {
        return finite_generator(detail::SuccessorCalculator<T, SuccessorCalculatorFunction>(initialValue, successorCalculatorFunction));
    }
}

#endif // RUSTY_ITER_HPP_INCLUDED
