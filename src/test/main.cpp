#include "../../include/rusty-iter.hpp"

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <limits>

struct TestCase
{
    void operator()(bool condition, const char* msg)
    {
        ++_numAllTestCases;
        if (condition)
        {
            std::cout << "Test case succeeded: " << msg << std::endl;
            return;
        }

        ++_numFailedTestCases;
        std::cerr << "Test case failed: " << msg << std::endl;
        _failedTestCases.emplace_back(msg);
    }

    int summary()
    {
        // print summary

        std::cout
            << std::endl
            << "----------------- SUMMARY -----------------" << std::endl
            << "Number of test cases: " << _numAllTestCases << std::endl
            << "Number of successful test cases: " << (_numAllTestCases - _numFailedTestCases) << std::endl
            << "Number of failed test cases: " << _numFailedTestCases << std::endl;

        if (_numFailedTestCases == 0)
        {
            std::cout << std::endl << "All tests passed" << std::endl << std::endl;
        }
        else
        {
            std::cerr << std::endl << "Some tests failed:" << std::endl;

            for (const std::string& failedTestCase : _failedTestCases)
            {
                std::cerr << "  " << failedTestCase << std::endl;
            }
        }

        return _numFailedTestCases != 0;
    }

private:
    size_t _numAllTestCases = 0;
    size_t _numFailedTestCases = 0;
    std::vector<std::string> _failedTestCases = { };
};


bool useForEachForIterTest = false;

template <typename Collection>
bool collections_equal(const Collection& a, const Collection& b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    size_t size = a.size();
    for (size_t i = 0; i < size; ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }

    return true;
}

template <typename T, typename Iter>
bool test_iter(Iter&& iter, const std::vector<T>& expected)
{
    size_t index = 0;

    bool inequality = false;
    bool sizeMismatch = false;

    auto callback = [&](const T& item)
    {
        size_t currentIndex = index++;

        if (currentIndex >= expected.size())
        {
            sizeMismatch = false;
            return;
        }

        if (expected[currentIndex] != item)
        {
            inequality = true;
            return;
        }
    };

    // for testing both iter.for_each and for (const auto& item : iter) { }
    if (useForEachForIterTest)
    {
        iter.for_each(callback);
    }
    else
    {
        for (const T& item : iter)
        {
            callback(item);
        }
    }

    return index == expected.size() && !inequality && !sizeMismatch;
}

template <typename T, typename Iter>
bool test_peek(Iter&& iter, const std::vector<T>& expected, int peekCount)
{
    auto peekableIter = iter.peekable();

    bool inequality = false;
    bool sizeMismatch = false;

    for (size_t i = 0; i < expected.size(); ++i)
    {
        for (int c = 0; c < peekCount; ++c)
        {
            // test multiple peeks

            const T* peeked = peekableIter.peek();
            if (peeked == nullptr)
            {
                return false;
            }

            if (expected[i] != *peeked)
            {
                return false;
            }
        }

        if (const T* next = peekableIter.next())
        {
            if (expected[i] != *next)
            {
                return false;
            }
        }
    }

    // iterator has finished, no more elements
    if (peekableIter.peek() != nullptr)
    {
        return false;
    }

    return true;
}

template <typename Collection, typename Iter>
bool test_collect_ordered(Iter&& iter, const Collection& expected)
{
    Collection collected = iter.template collect<Collection>();

    if (expected.size() != collected.size())
    {
        return false;
    }

    auto collectedIter = collected.begin();

    for (const auto& value : expected)
    {
        if (value != *collectedIter)
        {
            return false;
        }

        ++collectedIter;
    }

    return true;
}

template <typename Collection, typename Iter>
bool test_collect_ordered_with_size_hint(Iter&& iter, const Collection& expected, size_t sizeHint)
{
    Collection collected = iter.template collect_with_size_hint<Collection>(sizeHint);

    if (expected.size() != collected.size())
    {
        return false;
    }

    auto collectedIter = collected.begin();

    for (const auto& value : expected)
    {
        if (value != *collectedIter)
        {
            return false;
        }

        ++collectedIter;
    }

    return true;
}

template <typename Iter, typename Collection>
bool test_infinite_iter(Iter&& rangeIter, size_t count, const Collection& expected)
{
    Collection results{ };
    results.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        if (const int* next = rangeIter.next())
        {
            results.push_back(*next);
        }
        else
        {
            return false;
        }
    }

    return collections_equal(results, expected);
}

void test_iter_functionality(TestCase& testCase)
{
    std::vector<int> numbers = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<int> empty = { };

    testCase(test_iter(rusty::iter(numbers), numbers), "basic iterator functionality, simple");
    testCase(test_iter(rusty::iter(empty), empty), "basic iterator functionality, empty");

    testCase(test_iter(rusty::iter(numbers.begin(), numbers.end()), numbers), "basic iterator functionality with begin - end, simple");
    testCase(test_iter(rusty::iter(empty.begin(), empty.end()), empty), "basic iterator functionality with begin - end, empty");
}

void test_generators(TestCase& testCase)
{
    struct CountingGenerator
    {
        CountingGenerator(int startValue, int step) : _value(startValue), _step(step)
        {
        }

        int operator()()
        {
            int prev = _value;
            _value += _step;
            return prev;
        }

        int _value;
        int _step;
    };

    struct FiniteCountingGenerator
    {
        FiniteCountingGenerator(int startValue, int maxValue) : _value(startValue), _maxValue(maxValue)
        {
        }

        std::optional<int> operator()()
        {
            return next();
        }

        std::optional<int> next()
        {
            if (_value >= _maxValue)
            {
                return { };
            }

            int prev = _value;
            ++_value;
            return prev;
        }

        std::optional<int> next_back()
        {
            if (_value >= _maxValue)
            {
                return { };
            }

            return --_maxValue;
        }

        int _value;
        int _maxValue;
    };

    testCase(test_infinite_iter(rusty::infinite_generator(CountingGenerator(0, 1)), 10, std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "infinite generator, 0 to 10");
    testCase(test_infinite_iter(rusty::infinite_generator(CountingGenerator(10, 2)), 10, std::vector<int>{ 10, 12, 14, 16, 18, 20, 22, 24, 26, 28 }), "infinite generator, 10 to 30, step 2");
    testCase(test_infinite_iter(rusty::infinite_generator(CountingGenerator(0, 0)), 10, std::vector<int>{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }), "infinite generator, 0 step, always 0");

    testCase(test_iter(rusty::finite_generator(FiniteCountingGenerator(0, 10)), std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "finite generator, 0 to 10");
    testCase(test_iter(rusty::finite_generator(FiniteCountingGenerator(10, 20)), std::vector<int>{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }), "finite generator, 10 to 20");

    auto doubleEndedGenerator = rusty::double_ended_finite_generator(FiniteCountingGenerator(0, 10));
    testCase(
        *doubleEndedGenerator.next() == 0 &&
        *doubleEndedGenerator.next() == 1 &&
        *doubleEndedGenerator.next() == 2 &&
        *doubleEndedGenerator.next_back() == 9 &&
        *doubleEndedGenerator.next_back() == 8 &&
        *doubleEndedGenerator.next_back() == 7 &&
        *doubleEndedGenerator.next() == 3 &&
        *doubleEndedGenerator.next() == 4 &&
        *doubleEndedGenerator.next_back() == 6 &&
        *doubleEndedGenerator.next_back() == 5 &&
        doubleEndedGenerator.next_back() == nullptr &&
        doubleEndedGenerator.next_back() == nullptr &&
        doubleEndedGenerator.next() == nullptr &&
        doubleEndedGenerator.next() == nullptr,
        "double ended finite generator"
    );
}

void test_ranges(TestCase& testCase)
{
    testCase(test_iter(rusty::range(0, 10), std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "range, 0 to 10");
    testCase(test_iter(rusty::range(10, 20), std::vector<int>{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }), "range, 10 to 20");
    testCase(test_iter(rusty::range(0, 1), std::vector<int>{ 0 }), "range, 0 to 1");
    testCase(test_iter(rusty::range(0, 0), std::vector<int>{ }), "empty range");
    testCase(test_iter(rusty::range(0, -1), std::vector<int>{ }), "reverse range");

    testCase(test_iter(rusty::range(0, 10, 2), std::vector<int>{ 0, 2, 4, 6, 8 }), "range, 0 to 10, step 2");
    testCase(test_iter(rusty::range(0, 5, 2), std::vector<int>{ 0, 2, 4 }), "range, 0 to 5, step 2");

    testCase(test_iter(rusty::range_inclusive(0, 10), std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), "inclusive range, 0 to 10");
    testCase(test_iter(rusty::range_inclusive(0, 10, 2), std::vector<int>{ 0, 2, 4, 6, 8, 10 }), "inclusive range, 0 to 10, step 2");
    testCase(test_iter(rusty::range_inclusive(0, 5, 2), std::vector<int>{ 0, 2, 4 }), "inclusive range, 0 to 5, step 2");

    testCase(test_infinite_iter(rusty::infinite_range(0), 10, std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "infinite range, checking from 0, 10 times");
    testCase(test_infinite_iter(rusty::infinite_range(10), 10, std::vector<int>{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }), "infinite range, checking from 10, 10 times");
    testCase(test_infinite_iter(rusty::infinite_range(10, 2), 10, std::vector<int>{ 10, 12, 14, 16, 18, 20, 22, 24, 26, 28 }), "infinite range, checking from 10, 10 times, step 2");
}

void test_empty(TestCase& testCase)
{
    testCase(test_iter(rusty::empty<int>(), std::vector<int>{ }), "empty iterator");

    auto emptyIt = rusty::empty<int>();
    bool isEmpty = true;
    for (int i = 0; i < 10; ++i)
    {
        isEmpty &= emptyIt.next() == nullptr;
    }

    testCase(isEmpty, "empty iterator, calling next 10 times");
}

void test_once(TestCase& testCase)
{
    testCase(test_iter(rusty::once(123), std::vector<int>{ 123 }), "once, check if only has one element");

    auto onceIt = rusty::once(123);
    bool ok = true;
    ok &= *onceIt.next() == 123;
    ok &= onceIt.next() == nullptr;
    ok &= onceIt.next() == nullptr;
    ok &= onceIt.next() == nullptr;
    ok &= onceIt.next() == nullptr;
    testCase(ok, "once, one element, calling next manually");
}

void test_once_with(TestCase& testCase)
{
    testCase(test_iter(rusty::once_with([]() { return 123; }), std::vector<int>{ 123 }), "once_with, check if only has one element");

    auto onceIt = rusty::once_with([]() { return 123; });
    bool ok = true;
    ok &= *onceIt.next() == 123;
    ok &= onceIt.next() == nullptr;
    ok &= onceIt.next() == nullptr;
    ok &= onceIt.next() == nullptr;
    ok &= onceIt.next() == nullptr;
    testCase(ok, "once_with, one element, calling next manually");
}

void test_repeat(TestCase& testCase)
{
    testCase(test_iter(rusty::repeat(123).take(10), std::vector<int>{ 123, 123, 123, 123, 123, 123, 123, 123, 123, 123 }), "repeat, check 10 elements");

    auto repeatIt = rusty::repeat(123);
    bool ok = true;
    for (int i = 0; i < 10; ++i)
    {
        ok &= *repeatIt.next() == 123;
    }

    testCase(ok, "repeat, 10 elements, calling next manually");
}

void test_successors(TestCase& testCase)
{
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

    testCase(test_iter(rusty::successors<int>(1, calculateSuccessor), std::vector<int>{ 1, 2, 4, 8, 16, 32, 64 }), "successors");
    testCase(test_iter(rusty::successors<int>(1, [](const int&) { return std::optional<int>(); }), std::vector<int>{ 1 }), "successors, initial value only");
    testCase(test_iter(rusty::successors<int>({ }, calculateSuccessor), std::vector<int>{ }), "successors, empty starting value");
}


void test_step_by(TestCase& testCase)
{
    testCase(test_iter(rusty::range(0, 10).step_by(1), std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "step by, 0 to 10, step 1");
    testCase(test_iter(rusty::range(0, 10).step_by(2), std::vector<int>{ 0, 2, 4, 6, 8 }), "step_by, 0 to 10, step 2");
    testCase(test_iter(rusty::range(0, 10).step_by(3), std::vector<int>{ 0, 3, 6, 9 }), "step_by, 0 to 10, step 3");
    testCase(test_iter(rusty::range(0, 10).step_by(5), std::vector<int>{ 0, 5 }), "step_by, 0 to 10, step 5");
    testCase(test_iter(rusty::range(0, 10).step_by(10), std::vector<int>{ 0 }), "step_by, 0 to 10, step 10");
    testCase(test_iter(rusty::range(0, 10).step_by(0), std::vector<int>{ }), "step_by, 0 to 10, step 0");
    testCase(test_iter(rusty::range(0, 10).step_by(-1), std::vector<int>{ }), "step_by, 0 to 10, step -1");
}

void test_chain(TestCase& testCase)
{
    std::vector<int> randomNumbers = { 4, 6, 7, 3, 3, 2, 8, 9, 9, 5 };

    testCase(test_iter(
        rusty::range(0, 10).chain(rusty::range(10, 20)),
        std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }
    ), "chain, 0 to 10 then 10 to 20");

    testCase(test_iter(
        rusty::range(0, 5).chain(rusty::range(5, 10)).chain(rusty::range(10, 15)),
        std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 }
    ), "chain 3x, 0 to 5, 5 to 10, and 10 to 15");

    testCase(test_iter(
        rusty::range(0, 5).chain(rusty::iter(randomNumbers)),
        std::vector<int>{ 0, 1, 2, 3, 4, 4, 6, 7, 3, 3, 2, 8, 9, 9, 5 }
    ), "chain, 0 to 5 then numbers from a vector");
}

void test_zip(TestCase& testCase)
{
    std::vector<int> randomNumbers = { 5, 4, 2, 6, 8, 6, 4, 5, 5, 6 };
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    testCase(test_iter(
        rusty::range(0, 10).zip(rusty::range(10, 20)),
        std::vector<std::pair<int, int>>{
            { 0, 10 }, { 1, 11 }, { 2, 12 }, { 3, 13 }, { 4, 14 }, { 5, 15 }, { 6, 16 }, { 7, 17 }, { 8, 18 }, { 9, 19 }
        }
    ), "zip, 0 to 10 and 10 to 20");

    testCase(test_iter(
        rusty::range(0, 20).zip(rusty::iter(randomNumbers)),
        std::vector<std::pair<int, int>>{
            { 0, 5 }, { 1, 4 }, { 2, 2 }, { 3, 6 }, { 4, 8 }, { 5, 6 }, { 6, 4 }, { 7, 5 }, { 8, 5 }, { 9, 6 }
        }
    ), "zip, 0 to 20 and numbers from a vector (early termination)");

    testCase(test_iter(
        rusty::iter(randomNumbers).zip(rusty::iter(texts)),
        std::vector<std::pair<int, std::string>>{
            { 5, "hello world" }, { 4, "foo" }, { 2, "bar" }, { 6, "test 1234" }
        }
    ), "zip, different types");
}

void test_intersperse(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    testCase(test_iter(
        rusty::iter(texts).intersperse("-------"),
        std::vector<std::string>{ "hello world", "-------", "foo", "-------", "bar", "-------", "test 1234" }
    ), "intersperse");

    testCase(test_iter(
        rusty::iter(std::vector<std::string> { "foo" }).intersperse("-------"),
        std::vector<std::string>{ "foo" }
    ), "intersperse, single string");

    testCase(test_iter(
        rusty::iter(std::vector<std::string> { }).intersperse("-------"),
        std::vector<std::string>{ }
    ), "intersperse, empty input");

    testCase(test_iter(
        rusty::range(0, 10).intersperse(-1),
        std::vector<int>{ 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 8, -1, 9 }
    ), "intersperse, numbers");

    int number = 0;
    testCase(test_iter(
        rusty::iter(texts).intersperse_with([&]() { return std::to_string(number++) + " int"; }),
        std::vector<std::string>{ "hello world", "0 int", "foo", "1 int", "bar", "2 int", "test 1234" }
    ), "intersperse_with, string");

    number = 0;
    testCase(test_iter(
        rusty::range(0, 10).intersperse_with([&]() { return --number; }),
        std::vector<int>{ 0, -1, 1, -2, 2, -3, 3, -4, 4, -5, 5, -6, 6, -7, 7, -8, 8, -9, 9 }
    ), "intersperse_with, numbers");
}

void test_map(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    testCase(test_iter(
        rusty::iter(texts).map([](const std::string& text) { return (int)text.length(); }),
        std::vector<int>{ 11, 3, 3, 9 }
    ), "map, different type (map strings to their length)");

    testCase(test_iter(
        rusty::range(0, 10).map([](int number) { return number * 2; }),
        std::vector<int>{ 0, 2, 4, 6, 8, 10, 12, 14, 16, 18 }
    ), "map, numbers");

    testCase(test_iter(
        rusty::range(0, 10).map([](int number) { return std::to_string(number); }),
        std::vector<std::string>{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }
    ), "map, numbers to strings");
}

void test_filter(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    testCase(test_iter(
        rusty::iter(texts).filter([](const std::string& text) { return text.length() > 5; }),
        std::vector<std::string>{ "hello world", "test 1234" }
    ), "filter, strings longer than 5");

    testCase(test_iter(
        rusty::range(0, 10).filter([](int number) { return number % 2 == 0; }),
        std::vector<int>{ 0, 2, 4, 6, 8 }
    ), "filter, even numbers");

    testCase(test_iter(
        rusty::range(0, 10).filter([](int number) { return number % 2 != 0; }),
        std::vector<int>{ 1, 3, 5, 7, 9 }
    ), "filter, odd numbers");
}

void test_filter_map(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    testCase(test_iter(
        rusty::iter(texts).filter_map(
            [](const std::string& text) -> std::optional<int>
            {
                if (text.length() > 5)
                {
                    return (int)text.length();
                }
                else
                {
                    return { };
                }
            }
        ),
        std::vector<int>{ 11, 9 }
    ), "filter_map, strings longer than 5, mapped to their length");

    testCase(test_iter(
        rusty::range(0, 10).filter_map(
            [](int number) -> std::optional<int>
            {
                if (number % 2 == 0)
                {
                    return number * 2;
                }
                else
                {
                    return { };
                }
            }
        ),
        std::vector<int>{ 0, 4, 8, 12, 16 }
    ), "filter_map, even numbers, mapped to their double");

    testCase(test_iter(
        rusty::range(0, 10).filter_map(
            [](int number) -> std::optional<std::string>
            {
                if (number % 2 != 0)
                {
                    return std::to_string(number);
                }
                else
                {
                    return { };
                }
            }
        ),
        std::vector<std::string>{ "1", "3", "5", "7", "9" }
    ), "filter_map, odd numbers, mapped to their string value");
}

void test_peekable(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    testCase(test_peek<int>(rusty::range(0, 10), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, 3), "peekable, numbers");
    testCase(test_peek<std::string>(rusty::iter(texts), { "hello world", "foo", "bar", "test 1234" }, 3), "peekable, strings");
}

void test_skip_while(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    std::vector<int> randomNumbers = { 2, 6, 4, 3, 6, 5, 6, 5, 6, 7 };

    testCase(test_iter(
        rusty::iter(texts).skip_while([](const std::string& text) { return text.length() > 5; }),
        std::vector<std::string>{ "foo", "bar", "test 1234" }
    ), "skip_while, strings longer than 5");

    testCase(test_iter(
        rusty::range(0, 10).skip_while([](int number) { return number < 5; }),
        std::vector<int>{ 5, 6, 7, 8, 9 }
    ), "skip_while, numbers smaller than 5");

    testCase(test_iter(
        rusty::iter(randomNumbers).skip_while([](int number) { return number % 2 == 0; }),
        std::vector<int>{ 3, 6, 5, 6, 5, 6, 7 }
    ), "skip_while, skip even numbers at the start");

    testCase(test_iter(
        rusty::iter(randomNumbers).skip_while([](int number) { return true; }),
        std::vector<int>{ }
    ), "skip_while, skip everything");

    testCase(test_iter(
        rusty::iter(randomNumbers).skip_while([](int number) { return false; }),
        std::vector<int>{ 2, 6, 4, 3, 6, 5, 6, 5, 6, 7 }
    ), "skip_while, skip nothing");
}

void test_take_while(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    std::vector<int> randomNumbers = { 2, 6, 4, 3, 6, 5, 6, 5, 6, 7 };

    testCase(test_iter(
        rusty::iter(texts).take_while([](const std::string& text) { return text.length() > 5; }),
        std::vector<std::string>{ "hello world" }
    ), "take_while, strings longer than 5");

    testCase(test_iter(
        rusty::range(0, 10).take_while([](int number) { return number < 5; }),
        std::vector<int>{ 0, 1, 2, 3, 4 }
    ), "take_while, numbers smaller than 5");

    testCase(test_iter(
        rusty::iter(randomNumbers).take_while([](int number) { return number % 2 == 0; }),
        std::vector<int>{ 2, 6, 4 }
    ), "take_while, take even numbers at the start");

    testCase(test_iter(
        rusty::iter(randomNumbers).take_while([](int number) { return true; }),
        std::vector<int>{ 2, 6, 4, 3, 6, 5, 6, 5, 6, 7 }
    ), "take_while, take everything");

    testCase(test_iter(
        rusty::iter(randomNumbers).take_while([](int number) { return false; }),
        std::vector<int>{ }
    ), "take_while, take nothing");
}

void test_skip(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    std::vector<int> randomNumbers = { 8, 5, 2, 8, 6, 2, 1, 7, 3, 7 };

    testCase(test_iter(
        rusty::iter(texts).skip(2),
        std::vector<std::string>{ "bar", "test 1234" }
    ), "skip, strings, skip 2");

    testCase(test_iter(
        rusty::range(0, 10).skip(4),
        std::vector<int>{ 4, 5, 6, 7, 8, 9 }
    ), "skip, numbers, skip 4");

    testCase(test_iter(
        rusty::iter(randomNumbers).skip(5),
        std::vector<int>{ 2, 1, 7, 3, 7 }
    ), "skip, numbers from vector, skip 5");

    testCase(test_iter(
        rusty::iter(texts).skip(0),
        std::vector<std::string>{ "hello world", "foo", "bar", "test 1234" }
    ), "skip, strings, skip 0");

    testCase(test_iter(
        rusty::iter(texts).skip(10),
        std::vector<std::string>{ }
    ), "skip, strings, skip 10");
}

void test_take(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    std::vector<int> randomNumbers = { 7, 9, 1, 6, 1, 7, 9, 1, 7, 0 };

    testCase(test_iter(
        rusty::iter(texts).take(2),
        std::vector<std::string>{ "hello world", "foo" }
    ), "take, strings, take 2");

    testCase(test_iter(
        rusty::range(0, 10).take(4),
        std::vector<int>{ 0, 1, 2, 3 }
    ), "take, numbers, take 4");

    testCase(test_iter(
        rusty::iter(randomNumbers).take(5),
        std::vector<int>{ 7, 9, 1, 6, 1 }
    ), "take, numbers from vector, take 5");

    testCase(test_iter(
        rusty::iter(texts).take(0),
        std::vector<std::string>{ }
    ), "take, strings, take 0");

    testCase(test_iter(
        rusty::iter(texts).take(10),
        std::vector<std::string>{ "hello world", "foo", "bar", "test 1234" }
    ), "take, strings, take 10");
}

void test_enumerate(TestCase& testCase)
{
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    std::vector<int> randomNumbers = { 1, 7, 4, 8, 5, 4, 5, 7, 0, 3 };

    testCase(test_iter(
        rusty::iter(texts).enumerate<int>(),
        std::vector<std::pair<int, std::string>>{ { 0, "hello world" }, { 1, "foo" }, { 2, "bar" }, { 3, "test 1234" } }
    ), "enumerate, strings");

    testCase(test_iter(
        rusty::range(0, 10).enumerate<int>(),
        std::vector<std::pair<int, int>>{ { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 }, { 8, 8 }, { 9, 9 } }
    ), "enumerate, numbers");

    testCase(test_iter(
        rusty::iter(randomNumbers).enumerate<int>(),
        std::vector<std::pair<int, int>>{ { 0, 1 }, { 1, 7 }, { 2, 4 }, { 3, 8 }, { 4, 5 }, { 5, 4 }, { 6, 5 }, { 7, 7 }, { 8, 0 }, { 9, 3 } }
    ), "enumerate, numbers from vector");
}

void test_flatten(TestCase& testCase)
{
    std::vector<std::vector<int>> numbers =
    {
        { 1, 2, 3 },
        { 4, 5, 6 },
        { 7, 8, 9 }
    };

    std::vector<std::vector<std::string>> texts =
    {
        { "hello", "world" },
        { "foo", "bar" },
        { "test", "1234" }
    };

    testCase(test_iter(
        rusty::iter(numbers).map([](const std::vector<int>& numbers) { return rusty::iter(numbers); }).flatten(),
        std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9 }
    ), "flatten, vectors of numbers");

    testCase(test_iter(
        rusty::iter(texts).map([](const std::vector<std::string>& texts) { return rusty::iter(texts); }).flatten(),
        std::vector<std::string>{ "hello", "world", "foo", "bar", "test", "1234" }
    ), "flatten, vectors of strings");

    std::vector<std::string> texts2 =
    {
        "abc",
        "def",
        "ghi"
    };

    testCase(test_iter(
        rusty::iter(texts2)
        .map([](const std::string& str) { return rusty::iter(str); })
        .flatten(),
        std::vector<char>{ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i' }
    ), "flatten, strings to chars");
}

void test_inspect(TestCase& testCase)
{
    std::vector<int> randomNumbers = { 1, 6, 8, 8, 5, 9, 1, 4, 9, 2 };

    std::vector<int> inspected;
    rusty::iter(randomNumbers)
        .inspect([&](const int& currentValue) { inspected.push_back(currentValue); })
        .for_each([](const auto&) { }); // do nothing, just to finish the iterator

    bool inspectCountOk = inspected.size() == randomNumbers.size();
    bool inspectValuesOk = true;

    if (inspectCountOk)
    {
        for (size_t i = 0; i < randomNumbers.size(); ++i)
        {
            if (randomNumbers[i] != inspected[i])
            {
                inspectValuesOk = false;
            }
        }
    }

    testCase(inspectCountOk && inspectValuesOk, "inspect");
}

void test_cycle(TestCase& testCase)
{
    auto cycleIter = rusty::range(0, 3).cycle();
    std::vector<int> cycleGeneratedValues;
    std::vector<int> cycleExpectedValues = { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 };
    for (int i = 0; i < 10; ++i)
    {
        cycleGeneratedValues.push_back(*cycleIter.next());
    }

    testCase(collections_equal(cycleGeneratedValues, cycleExpectedValues), "cycle");

    auto emptyCycleIter = rusty::range(0, 0).cycle();
    bool cycleIsEmpty = true;
    for (int i = 0; i < 10; ++i)
    {
        if (emptyCycleIter.next() != nullptr)
        {
            cycleIsEmpty = false;
        }
    }

    testCase(cycleIsEmpty, "empty cycle");

    testCase(collections_equal(
        rusty::range(0, 3)
        .map([](const int& num) { return num; }) // test case for using a lambda
        .cycle()
        .take(10)
        .collect<std::vector<int>>(),
        std::vector<int>{ 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 }
    ), "cycle, with lambda");
}


void test_collect(TestCase& testCase)
{
    testCase(test_collect_ordered<std::vector<int>>(rusty::range(0, 10), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "collect to vector");
    testCase(test_collect_ordered<std::list<int>>(rusty::range(0, 10), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }), "collect to list");

    testCase(test_collect_ordered_with_size_hint<std::vector<int>>(rusty::range(0, 10), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, 10), "collect with size hint to vector");
}

void test_partition(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    auto partitioned = rusty::iter(numbers).partition<std::vector<int>>([](const int& number) { return number % 2 == 0; });
    testCase(
        collections_equal(partitioned.first, std::vector<int>{ 1, 3, 5, 7, 9 }) &&
        collections_equal(partitioned.second, std::vector<int>{ 2, 4, 6, 8, 10 }),
        "partition even/odd numbers"
    );

    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    auto textPartitioned = rusty::iter(texts).partition<std::vector<std::string>>([](const std::string& text) { return text.size() > 5; });
    testCase(
        collections_equal(textPartitioned.first, std::vector<std::string>{ "foo", "bar" }) &&
        collections_equal(textPartitioned.second, std::vector<std::string>{ "hello world", "test 1234" }),
        "partition strings"
    );
}

void test_reduce(TestCase& testCase)
{
    auto reduceFunc = [](const int& acc, const int& current)
    {
        // return the smallest value that is greater than 5, and is divisible by 2

        bool accIsValid = acc > 5 && acc % 2 == 0;
        bool currentIsValid = current > 5 && current % 2 == 0;

        if (accIsValid && currentIsValid)
        {
            return std::min(acc, current);
        }
        else if (currentIsValid)
        {
            return current;
        }
        else
        {
            return acc;
        }
    };

    testCase(*rusty::range(0, 10).reduce(reduceFunc) == 6, "reduce");
    testCase(!rusty::range(0, 0).reduce(reduceFunc).has_value(), "reduce, empty iterator");
}

void test_fold(TestCase& testCase)
{
    auto foldFunc = [](const int& acc, const int& current) { return acc + current; };

    testCase(rusty::range(0, 0).fold(0, foldFunc) == 0, "fold, empty iterator");
    testCase(rusty::range(0, 10).fold(0, foldFunc) == 45, "fold");
    testCase(rusty::range(1, 10).fold(1, [](const int& acc, const int& current) { return acc * current; }) == 362880, "fold, multiplication");

    // test rfold
    testCase(rusty::range(0, 0).rfold(0, foldFunc) == 0, "rfold, empty iterator");
    testCase(rusty::range(0, 10).rfold(0, foldFunc) == 45, "rfold");
    testCase(rusty::range(1, 10).rfold(1, [](const int& acc, const int& current) { return acc * current; }) == 362880, "rfold, multiplication");
}

void test_count(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(rusty::iter(numbers).count() == 10, "count, from vector");
    testCase(rusty::range(0, 10).count() == 10, "count, range");
    testCase(rusty::range(0, 0).count() == 0, "count, empty iterator");
}

void test_last(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).last() == 10, "last, from vector");
    testCase(*rusty::range(0, 10).last() == 9, "last, range");
    testCase(!rusty::range(0, 0).last().has_value(), "last, empty iterator");
}

void test_nth(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).nth(0) == 1, "nth, from vector, 0th");
    testCase(*rusty::iter(numbers).nth(4) == 5, "nth, from vector, 4th");
    testCase(!rusty::iter(numbers).nth(15).has_value(), "nth, from vector, index out of bounds");
    testCase(*rusty::range(0, 10).nth(0) == 0, "nth, range, 0th");
    testCase(*rusty::range(0, 10).nth(4) == 4, "nth, range, 4th");
    testCase(!rusty::range(0, 10).nth(15).has_value(), "nth, range, index out of bounds");
    testCase(!rusty::range(0, 0).nth(0).has_value(), "nth, empty iterator");

    auto iter = rusty::iter(numbers);
    testCase(*iter.nth(0) == 1, "nth, manually stepping iterator, 1");
    testCase(*iter.nth(0) == 2, "nth, manually stepping iterator, 2");
    testCase(*iter.nth(0) == 3, "nth, manually stepping iterator, 3");
    testCase(*iter.nth(5) == 9, "nth, manually stepping iterator, 4");
    testCase(*iter.nth(0) == 10, "nth, manually stepping iterator, 5");
    testCase(!iter.nth(0).has_value(), "nth, manually stepping iterator, 6");

    // test nth_back
    testCase(*rusty::iter(numbers).nth_back(0) == 10, "nth_back, from vector, 0th");
    testCase(*rusty::iter(numbers).nth_back(4) == 6, "nth_back, from vector, 4th");
    testCase(!rusty::iter(numbers).nth_back(15).has_value(), "nth_back, from vector, index out of bounds");
    testCase(*rusty::range(0, 10).nth_back(0) == 9, "nth_back, range, 0th");
    testCase(*rusty::range(0, 10).nth_back(4) == 5, "nth_back, range, 4th");
    testCase(!rusty::range(0, 10).nth_back(15).has_value(), "nth_back, range, index out of bounds");
    testCase(!rusty::range(0, 0).nth_back(0).has_value(), "nth_back, empty iterator");

    auto iter2 = rusty::iter(numbers);
    testCase(*iter2.nth_back(0) == 10, "nth_back, manually stepping iterator, 1");
    testCase(*iter2.nth_back(0) == 9, "nth_back, manually stepping iterator, 2");
    testCase(*iter2.nth_back(0) == 8, "nth_back, manually stepping iterator, 3");
    testCase(*iter2.nth_back(5) == 2, "nth_back, manually stepping iterator, 4");
    testCase(*iter2.nth_back(0) == 1, "nth_back, manually stepping iterator, 5");
    testCase(!iter2.nth_back(0).has_value(), "nth_back, manually stepping iterator, 6");
}

void test_all(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(rusty::iter(numbers).all([](const int& number) { return number > 0; }), "all, test positive");
    testCase(!rusty::iter(numbers).all([](const int& number) { return number > 5; }), "all, test if >5");
    testCase(rusty::range(0, 0).all([](const int& number) { return number > 0; }), "all, empty iterator");
}

void test_any(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(rusty::iter(numbers).any([](const int& number) { return number > 5; }), "any, test if >5");
    testCase(!rusty::iter(numbers).any([](const int& number) { return number > 10; }), "any, test if >10");
    testCase(!rusty::range(0, 0).any([](const int& number) { return number > 0; }), "any, empty iterator");
}

void test_find(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).find([](const int& number) { return number == 5; }) == 5, "find, find 5");
    testCase(!rusty::iter(numbers).find([](const int& number) { return number == 15; }).has_value(), "find, find 15");
    testCase(!rusty::range(0, 0).find([](const int& number) { return true; }).has_value(), "find, empty iterator");
    testCase(*rusty::iter(numbers).find([](const int& number) { return number % 2 == 0; }) == 2, "find, find an even number");
    testCase(*rusty::iter(numbers).find([](const int& number) { return number > 5; }) == 6, "find, find a number >5");

    // test rfind
    testCase(*rusty::iter(numbers).rfind([](const int& number) { return number == 5; }) == 5, "find, find 5");
    testCase(!rusty::iter(numbers).rfind([](const int& number) { return number == 15; }).has_value(), "find, find 15");
    testCase(!rusty::range(0, 0).rfind([](const int& number) { return true; }).has_value(), "find, empty iterator");
    testCase(*rusty::iter(numbers).rfind([](const int& number) { return number % 2 == 0; }) == 10, "find, find an even number");
    testCase(*rusty::iter(numbers).rfind([](const int& number) { return number > 5; }) == 10, "find, find a number >5");
}

void test_position(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(rusty::iter(numbers).position([](const int& number) { return number == 5; }) == 4, "position, find 5");
    testCase(!rusty::iter(numbers).position([](const int& number) { return number == 15; }).has_value(), "position, find 15");
    testCase(!rusty::range(0, 0).position([](const int& number) { return true; }).has_value(), "position, empty iterator");
    testCase(rusty::iter(numbers).position([](const int& number) { return number % 2 == 0; }) == 1, "position, find an even number");
    testCase(rusty::iter(numbers).position([](const int& number) { return number > 5; }) == 5, "position, find a number >5");

    // test rposition
    testCase(rusty::iter(numbers).rposition([](const int& number) { return number == 5; }) == 5, "rposition, find 5");
    testCase(!rusty::iter(numbers).rposition([](const int& number) { return number == 15; }).has_value(), "rposition, find 15");
    testCase(!rusty::range(0, 0).rposition([](const int& number) { return true; }).has_value(), "rposition, empty iterator");
    testCase(rusty::iter(numbers).rposition([](const int& number) { return number % 2 == 0; }) == 0, "rposition, find an even number");
    testCase(rusty::iter(numbers).rposition([](const int& number) { return number > 5; }) == 0, "rposition, find a number >5");
}

void test_min(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).min() == 1, "min, from vector");
    testCase(*rusty::range(0, 10).min() == 0, "min, range");
    testCase(!rusty::range(0, 0).min().has_value(), "min, empty iterator");
}

void test_min_by(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).min_by([](const int& a, const int& b) { return a - b; }) == 1, "min_by, search minimum");
    testCase(*rusty::iter(numbers).min_by([](const int& a, const int& b) { return b - a; }) == 10, "min_by, search maximum");
    testCase(!rusty::range(0, 0).min_by([](const int& a, const int& b) { return a - b; }).has_value(), "min_by, empty iterator");
}

void test_max(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).max() == 10, "max, from vector");
    testCase(*rusty::range(0, 10).max() == 9, "max, range");
    testCase(!rusty::range(0, 0).max().has_value(), "max, empty iterator");
}

void test_max_by(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(*rusty::iter(numbers).max_by([](const int& a, const int& b) { return a - b; }) == 10, "max_by, search maximum");
    testCase(*rusty::iter(numbers).max_by([](const int& a, const int& b) { return b - a; }) == 1, "max_by, search minimum");
    testCase(!rusty::range(0, 0).max_by([](const int& a, const int& b) { return a - b; }).has_value(), "max_by, empty iterator");
}

void test_sum(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(rusty::iter(numbers).sum() == 55, "sum, from vector");
    testCase(rusty::range(0, 10).sum() == 45, "sum, range");
    testCase(rusty::range(0, 0).sum() == 0, "sum, empty iterator");
}

void test_product(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    testCase(rusty::iter(numbers).product() == 3628800, "product, from vector");
    testCase(rusty::range(0, 10).product() == 0, "product, range from 0");
    testCase(rusty::range(1, 10).product() == 362880, "product, range from 1");
    testCase(rusty::range(0, 0).product() == 1, "product, empty iterator");
}

void test_is_sorted_ascending(TestCase& testCase)
{
    std::vector<int> numbersAscending = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::vector<int> numbersDescending = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    std::vector<int> numbersMixed = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    testCase(rusty::iter(numbersAscending).is_sorted_ascending(), "is_sorted_ascending, ascending values");
    testCase(!rusty::iter(numbersDescending).is_sorted_ascending(), "is_sorted_ascending, descending values");
    testCase(!rusty::iter(numbersMixed).is_sorted_ascending(), "is_sorted_ascending, mixed values");
    testCase(rusty::range(0, 0).is_sorted_ascending(), "is_sorted_ascending, empty iterator");
}

void test_is_sorted_descending(TestCase& testCase)
{
    std::vector<int> numbersAscending = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::vector<int> numbersDescending = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    std::vector<int> numbersMixed = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    testCase(!rusty::iter(numbersAscending).is_sorted_descending(), "is_sorted_descending, ascending values");
    testCase(rusty::iter(numbersDescending).is_sorted_descending(), "is_sorted_descending, descending values");
    testCase(!rusty::iter(numbersMixed).is_sorted_descending(), "is_sorted_descending, mixed values");
    testCase(rusty::range(0, 0).is_sorted_descending(), "is_sorted_descending, empty iterator");
}

void test_is_sorted_by(TestCase& testCase)
{
    std::vector<int> numbersAscending = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::vector<int> numbersDescending = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    std::vector<int> numbersMixed = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    testCase(rusty::iter(numbersAscending).is_sorted_by([](const int& a, const int& b) { return a - b; }), "is_sorted_by, ascending values, test ascending");
    testCase(!rusty::iter(numbersAscending).is_sorted_by([](const int& a, const int& b) { return b - a; }), "is_sorted_by, ascending values, test descending");
    testCase(!rusty::iter(numbersDescending).is_sorted_by([](const int& a, const int& b) { return a - b; }), "is_sorted_by, descending values, test ascending");
    testCase(rusty::iter(numbersDescending).is_sorted_by([](const int& a, const int& b) { return b - a; }), "is_sorted_by, descending values, test descending");
    testCase(!rusty::iter(numbersMixed).is_sorted_by([](const int& a, const int& b) { return a - b; }), "is_sorted_by, mixed values, test ascending");
    testCase(!rusty::iter(numbersMixed).is_sorted_by([](const int& a, const int& b) { return b - a; }), "is_sorted_by, mixed values, test descending");
    testCase(rusty::range(0, 0).is_sorted_by([](const int& a, const int& b) { return a - b; }), "is_sorted_by, empty iterator");
}


void test_reverse(TestCase& testCase)
{
    testCase(test_iter(rusty::empty<int>().reverse(), std::vector<int>{ }), "reverse, empty iterator");
    testCase(test_iter(rusty::range(0, 10).reverse(), std::vector<int>{ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }), "reverse, simple range");
    testCase(test_iter(rusty::range(0, 10, 3).reverse(), std::vector<int>{ 9, 6, 3, 0 }), "reverse, range with step");
    testCase(test_iter(rusty::range(0, 10, 4).reverse(), std::vector<int>{ 8, 4, 0 }), "reverse, range with step, test case 2");

    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::vector<int> reversed = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    std::list<int> numbersList = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::list<int> reversedList = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    testCase(test_iter(rusty::iter(numbers).reverse(), reversed), "reverse, from vector");
    testCase(test_iter(rusty::iter(numbers).reverse().reverse(), numbers), "reverse 2x");
    testCase(test_collect_ordered(rusty::iter(numbersList).reverse(), reversedList), "reverse, from list");
}


void test_comparisons(TestCase& testCase)
{
    std::vector<int> shortRange = { 0, 1 };
    std::vector<int> longRange = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<int> smallRange = { 0, 1, 2, 3 };
    std::vector<int> bigRange = { 5, 6, 7, 8 };

    // cmp
    testCase(rusty::iter(shortRange).cmp(rusty::iter(shortRange)) == 0, "cmp, equal ranges");
    testCase(rusty::iter(shortRange).cmp(rusty::iter(longRange)) < 0, "cmp, first range shorter");
    testCase(rusty::iter(longRange).cmp(rusty::iter(shortRange)) > 0, "cmp, second range shorter");
    testCase(rusty::iter(bigRange).cmp(rusty::iter(smallRange)) > 0, "cmp, first range is bigger");
    testCase(rusty::iter(smallRange).cmp(rusty::iter(bigRange)) < 0, "cmp, second range is bigger");

    auto cmp = [](const int& a, const int& b) { return a - b; };
    auto cmpReverse = [](const int& a, const int& b) { return b - a; };

    // cmp_by
    testCase(rusty::iter(shortRange).cmp_by(rusty::iter(shortRange), cmp) == 0, "cmp_by, equal ranges");
    testCase(rusty::iter(shortRange).cmp_by(rusty::iter(longRange), cmp) < 0, "cmp_by, first range shorter");
    testCase(rusty::iter(longRange).cmp_by(rusty::iter(shortRange), cmp) > 0, "cmp_by, second range shorter");
    testCase(rusty::iter(bigRange).cmp_by(rusty::iter(smallRange), cmp) > 0, "cmp_by, first range is bigger");
    testCase(rusty::iter(smallRange).cmp_by(rusty::iter(bigRange), cmp) < 0, "cmp_by, second range is bigger");

    testCase(rusty::iter(shortRange).cmp_by(rusty::iter(shortRange), cmpReverse) == 0, "cmp_by, equal ranges, reverse compare");
    testCase(rusty::iter(shortRange).cmp_by(rusty::iter(longRange), cmpReverse) < 0, "cmp_by, first range shorter, reverse compare");
    testCase(rusty::iter(longRange).cmp_by(rusty::iter(shortRange), cmpReverse) > 0, "cmp_by, second range shorter, reverse compare");
    testCase(rusty::iter(bigRange).cmp_by(rusty::iter(smallRange), cmpReverse) < 0, "cmp_by, first range is bigger, reverse compare");
    testCase(rusty::iter(smallRange).cmp_by(rusty::iter(bigRange), cmpReverse) > 0, "cmp_by, second range is bigger, reverse compare");

    // partial_cmp
    std::vector<float> shortRangeFloat = { 0.0f, 1.0f };
    std::vector<float> longRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    std::vector<float> smallRangeFloat = { 0.0f, 1.0f, 2.0f, 3.0f };
    std::vector<float> bigRangeFloat = { 5.0f, 6.0f, 7.0f, 8.0f };
    std::vector<float> nonComparable = { 0.0f, std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f };
    testCase(*rusty::iter(shortRangeFloat).partial_cmp(rusty::iter(shortRangeFloat)) == 0, "partial_cmp, equal ranges");
    testCase(*rusty::iter(shortRangeFloat).partial_cmp(rusty::iter(longRangeFloat)) < 0, "partial_cmp, first range shorter");
    testCase(*rusty::iter(longRangeFloat).partial_cmp(rusty::iter(shortRangeFloat)) > 0, "partial_cmp, second range shorter");
    testCase(*rusty::iter(bigRangeFloat).partial_cmp(rusty::iter(smallRangeFloat)) > 0, "partial_cmp, first range is bigger");
    testCase(*rusty::iter(smallRangeFloat).partial_cmp(rusty::iter(bigRangeFloat)) < 0, "partial_cmp, second range is bigger");
    testCase(!rusty::iter(smallRangeFloat).partial_cmp(rusty::iter(nonComparable)).has_value(), "partial_cmp, equal ranges, not comparable");

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

    testCase(*rusty::iter(shortRangeFloat).partial_cmp_by(rusty::iter(shortRangeFloat), partialCmp) == 0, "partial_cmp_by, equal ranges");
    testCase(*rusty::iter(shortRangeFloat).partial_cmp_by(rusty::iter(longRangeFloat), partialCmp) < 0, "partial_cmp_by, first range shorter");
    testCase(*rusty::iter(longRangeFloat).partial_cmp_by(rusty::iter(shortRangeFloat), partialCmp) > 0, "partial_cmp_by, second range shorter");
    testCase(*rusty::iter(bigRangeFloat).partial_cmp_by(rusty::iter(smallRangeFloat), partialCmp) > 0, "partial_cmp_by, first range is bigger");
    testCase(*rusty::iter(smallRangeFloat).partial_cmp_by(rusty::iter(bigRangeFloat), partialCmp) < 0, "partial_cmp_by, second range is bigger");
    testCase(!rusty::iter(smallRangeFloat).partial_cmp_by(rusty::iter(nonComparable), partialCmp).has_value(), "partial_cmp_by, equal ranges, not comparable");

    // eq
    testCase(rusty::iter(shortRange).eq(rusty::iter(shortRange)), "eq, equal ranges");
    testCase(!rusty::iter(shortRange).eq(rusty::iter(longRange)), "eq, first range shorter");
    testCase(!rusty::iter(longRange).eq(rusty::iter(shortRange)), "eq, second range shorter");
    testCase(!rusty::iter(bigRange).eq(rusty::iter(smallRange)), "eq, first range is bigger");
    testCase(!rusty::iter(smallRange).eq(rusty::iter(bigRange)), "eq, second range is bigger");

    // eq_by
    std::vector<int> evenNumbersSmall = { 0, 2, 4 };
    std::vector<int> evenNumbersBig = { 6, 8, 10 };
    auto eq = [](const int& a, const int& b) { return a == b; };
    auto eqByParity = [](const int& a, const int& b) { return a % 2 == b % 2; };
    testCase(rusty::iter(shortRange).eq_by(rusty::iter(shortRange), eq), "eq_by, equal ranges");
    testCase(!rusty::iter(shortRange).eq_by(rusty::iter(longRange), eq), "eq_by, first range shorter");
    testCase(!rusty::iter(longRange).eq_by(rusty::iter(shortRange), eq), "eq_by, second range shorter");
    testCase(!rusty::iter(evenNumbersBig).eq_by(rusty::iter(evenNumbersSmall), eq), "eq_by, first range is bigger");
    testCase(!rusty::iter(evenNumbersSmall).eq_by(rusty::iter(evenNumbersBig), eq), "eq_by, second range is bigger");
    testCase(rusty::iter(shortRange).eq_by(rusty::iter(shortRange), eqByParity), "eq_by, equal ranges, check equality by parity");
    testCase(!rusty::iter(shortRange).eq_by(rusty::iter(longRange), eqByParity), "eq_by, first range shorter, check equality by parity");
    testCase(!rusty::iter(longRange).eq_by(rusty::iter(shortRange), eqByParity), "eq_by, second range shorter, check equality by parity");
    testCase(rusty::iter(evenNumbersBig).eq_by(rusty::iter(evenNumbersSmall), eqByParity), "eq_by, first range is bigger, check equality by parity");
    testCase(rusty::iter(evenNumbersSmall).eq_by(rusty::iter(evenNumbersBig), eqByParity), "eq_by, second range is bigger, check equality by parity");

    // ne
    testCase(!rusty::iter(shortRange).ne(rusty::iter(shortRange)), "ne, equal ranges");
    testCase(rusty::iter(shortRange).ne(rusty::iter(longRange)), "ne, first range shorter");
    testCase(rusty::iter(longRange).ne(rusty::iter(shortRange)), "ne, second range shorter");
    testCase(rusty::iter(bigRange).ne(rusty::iter(smallRange)), "ne, first range is bigger");
    testCase(rusty::iter(smallRange).ne(rusty::iter(bigRange)), "ne, second range is bigger");

    // lt
    testCase(!rusty::iter(shortRangeFloat).lt(rusty::iter(shortRangeFloat)), "lt, equal ranges");
    testCase(rusty::iter(shortRangeFloat).lt(rusty::iter(longRangeFloat)), "lt, first range shorter");
    testCase(!rusty::iter(longRangeFloat).lt(rusty::iter(shortRangeFloat)), "lt, second range shorter");
    testCase(!rusty::iter(bigRangeFloat).lt(rusty::iter(smallRangeFloat)), "lt, first range is bigger");
    testCase(rusty::iter(smallRangeFloat).lt(rusty::iter(bigRangeFloat)), "lt, second range is bigger");
    testCase(!rusty::iter(nonComparable).lt(rusty::iter(nonComparable)), "lt, equal ranges, not comparable");

    // le
    testCase(rusty::iter(shortRangeFloat).le(rusty::iter(shortRangeFloat)), "le, equal ranges");
    testCase(rusty::iter(shortRangeFloat).le(rusty::iter(longRangeFloat)), "le, first range shorter");
    testCase(!rusty::iter(longRangeFloat).le(rusty::iter(shortRangeFloat)), "le, second range shorter");
    testCase(!rusty::iter(bigRangeFloat).le(rusty::iter(smallRangeFloat)), "le, first range is bigger");
    testCase(rusty::iter(smallRangeFloat).le(rusty::iter(bigRangeFloat)), "le, second range is bigger");
    testCase(!rusty::iter(nonComparable).le(rusty::iter(nonComparable)), "le, equal ranges, not comparable");

    // gt
    testCase(!rusty::iter(shortRangeFloat).gt(rusty::iter(shortRangeFloat)), "gt, equal ranges");
    testCase(!rusty::iter(shortRangeFloat).gt(rusty::iter(longRangeFloat)), "gt, first range shorter");
    testCase(rusty::iter(longRangeFloat).gt(rusty::iter(shortRangeFloat)), "gt, second range shorter");
    testCase(rusty::iter(bigRangeFloat).gt(rusty::iter(smallRangeFloat)), "gt, first range is bigger");
    testCase(!rusty::iter(smallRangeFloat).gt(rusty::iter(bigRangeFloat)), "gt, second range is bigger");
    testCase(!rusty::iter(nonComparable).gt(rusty::iter(nonComparable)), "gt, equal ranges, not comparable");

    // ge
    testCase(rusty::iter(shortRangeFloat).ge(rusty::iter(shortRangeFloat)), "ge, equal ranges");
    testCase(!rusty::iter(shortRangeFloat).ge(rusty::iter(longRangeFloat)), "ge, first range shorter");
    testCase(rusty::iter(longRangeFloat).ge(rusty::iter(shortRangeFloat)), "ge, second range shorter");
    testCase(rusty::iter(bigRangeFloat).ge(rusty::iter(smallRangeFloat)), "ge, first range is bigger");
    testCase(!rusty::iter(smallRangeFloat).ge(rusty::iter(bigRangeFloat)), "ge, second range is bigger");
    testCase(!rusty::iter(nonComparable).ge(rusty::iter(nonComparable)), "ge, equal ranges, not comparable");
}


void test_random_stuff(TestCase& testCase)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::vector<std::string> texts =
    {
        "hello world",
        "foo",
        "bar",
        "test 1234"
    };

    int multiplier = 2;

    testCase(test_iter<int>(
        rusty::iter(numbers)
        .filter([](const int& num) { return num % 2 == 0; })
        .map([&](const int& num) { return num * multiplier; }),
        { 4, 8, 12, 16 }
    ), "filter + map");

    testCase(test_iter<int>(
        rusty::iter(numbers.begin(), numbers.end())
        .filter([](const int& num) { return num % 2 == 0; })
        .map([&](const int& num) { return num * multiplier; }),
        { 4, 8, 12, 16 }
    ), "filter + map with begin + end");

    auto numberToString = [](const int& num)
    {
        std::stringstream ss;
        ss << num;
        ss << " int";
        return ss.str();
    };

    testCase(test_iter<std::string>(
        rusty::iter(numbers).step_by(3).map(numberToString),
        { "1 int", "4 int", "7 int" }
    ), "map to a different type");

    testCase(test_iter<std::pair<int, int>>(
        rusty::range(0, 10).zip(rusty::range(20, 30)).step_by(2),
        { { 0, 20 }, { 2, 22 }, { 4, 24 }, { 6, 26 }, { 8, 28 } }
    ), "zipped ranges, step by 2");

    testCase(test_iter<int>(
        rusty::range(0, 10).step_by(2).chain(rusty::range(20, 30)).step_by(2),
        { 0, 4, 8, 21, 23, 25, 27, 29 }
    ), "chained ranges with step by");

    std::string suffix = " suffix";
    testCase(test_collect_ordered<std::string>(
        rusty::iter(texts)
        .map([&](const std::string& str) { return rusty::iter(str).chain(rusty::iter(suffix)); })
        .flatten(),
        std::string("hello world suffixfoo suffixbar suffixtest 1234 suffix")),
        "collect to string with flatten");

    auto collectString = [](const std::string& acc, const std::string& current)
    {
        return acc + current;
    };
    testCase(rusty::iter(texts).fold<std::string>("", collectString) == "hello worldfoobartest 1234", "fold into string");

    testCase(test_iter(
        rusty::iter(numbers)
        .map([&](const int& num) { return std::to_string(num) + "123"; })
        .map([&](const std::string& str) { return std::stoi(str); })
        .filter([](const int& num) { return num % 3 == 0; }),
        std::vector<int>{ 3123, 6123 }
    ), "complex - convert to string, then append \"123\", then convert back to string, keep the ones that are divisible by 3");

    testCase(rusty::iter(numbers)
        .filter_map([](const int& num) -> std::optional<int> { if (num < 5) return num * 2; else return { }; })
        .intersperse(-1)
        .chain(rusty::range(0, 100))
        .step_by(7)
        .map([](const int& num) { return std::to_string(num); })
        .enumerate<int>()
        .map([](const std::pair<int, std::string>& value) { return std::to_string(value.first) + "123" + value.second; })
        .skip(3)
        .map([](const std::string& str) { return std::stoi(str); })
        .cycle()
        .inspect([](const int&) { /* nothing */ })
        .take(30)
        .filter([](const int& num) { return num % 3 == 0; })
        .fold(123, [](const int& acc, const int& curr) { return (acc * 97 + curr) % 79; })
        == 14
        , "very complex"
    );
}

template <bool Test>
void test_compiler_error_messages()
{
    // this function is for testing if more helpful compiler error messages are displayed for common mistakes

    if constexpr (Test)
    {
        auto nothing = [](auto&&) { };

        // callbacks must take values by const references or by value

        // compiles
        rusty::range(0, 10).map([](int value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](const int value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](const int& value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](auto value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](const auto& value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](auto&& value) { return value; }).for_each(nothing);

        // doesn't compile
        rusty::range(0, 10).map([](int& value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](auto& value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](int* value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](const int* value) { return value; }).for_each(nothing);
        rusty::range(0, 10).map([](int&& value) { return value; }).for_each(nothing);
        rusty::range(0, 10).for_each([](int& val) { });
        rusty::range(0, 10).partition<std::vector<int>>([](int& val) { return val % 2 == 0; });
        rusty::range(0, 10).partition<std::vector<int>>([](const int& val) { return val % 2; });
        rusty::range(0, 10).reduce([](const int& a, int& b) { return a; });
        rusty::range(0, 10).reduce([](int& a, const int& b) { return a; });
        rusty::range(0, 10).reduce([](int& a, int& b) { return a; });
        rusty::range(0, 10).fold(0, [](int& a, int& b) { return a; });
        rusty::range(0, 10).all([](int&) { return true; });
        rusty::range(0, 10).any([](int&) { return true; });
        rusty::range(0, 10).find([](int&) { return true; });
        rusty::range(0, 10).position([](int&) { return true; });
        rusty::range(0, 10).min_by([](int&, int&) { return 0; });
        rusty::range(0, 10).max_by([](int&, int&) { return 0; });
        rusty::range(0, 10).is_sorted_by([](int&, int&) { return 0; });
        rusty::range(0, 10).cmp_by(rusty::range(0, 10), [](int&, int&) { return 0; });
        rusty::range(0, 10).eq_by(rusty::range(0, 10), [](int&, int&) { return 0; });
        rusty::range(0, 10).partial_cmp_by(rusty::range(0, 10), [](int&, int&) { return 0; });
        rusty::range(0, 10).map([](int& value) { return std::to_string(value); });
        rusty::range(0, 10).filter([](int& value) { return true; });
        rusty::range(0, 10).filter_map([](int& value) { return std::optional<int>(); });
        rusty::range(0, 10).skip_while([](int&) { return true; });
        rusty::range(0, 10).take_while([](int&) { return true; });
        rusty::range(0, 10).inspect([](int&) { });

        // TODO: better error message for this? we need to detect if the callback returns an std::optional
        rusty::range(0, 10).filter_map([](const int& value) { return value; });
    }
}


int main()
{
    test_compiler_error_messages<false>();

    TestCase testCase;

    auto runTests = [&]()
    {
        test_iter_functionality(testCase);
        test_generators(testCase);
        test_ranges(testCase);
        test_empty(testCase);
        test_once(testCase);
        test_once_with(testCase);
        test_repeat(testCase);
        test_successors(testCase);

        test_step_by(testCase);
        test_chain(testCase);
        test_zip(testCase);
        test_intersperse(testCase);
        test_map(testCase);
        test_filter(testCase);
        test_filter_map(testCase);
        test_peekable(testCase);
        test_skip_while(testCase);
        test_take_while(testCase);
        test_skip(testCase);
        test_take(testCase);
        test_enumerate(testCase);
        test_flatten(testCase);
        test_inspect(testCase);
        test_cycle(testCase);

        test_collect(testCase);
        test_partition(testCase);
        test_reduce(testCase);
        test_fold(testCase);
        test_count(testCase);
        test_last(testCase);
        test_nth(testCase);
        test_all(testCase);
        test_any(testCase);
        test_find(testCase);
        test_position(testCase);
        test_min(testCase);
        test_min_by(testCase);
        test_max(testCase);
        test_max_by(testCase);
        test_sum(testCase);
        test_product(testCase);
        test_is_sorted_ascending(testCase);
        test_is_sorted_descending(testCase);
        test_is_sorted_by(testCase);

        test_reverse(testCase);

        test_comparisons(testCase);

        test_random_stuff(testCase);
    };

    useForEachForIterTest = false;
    runTests();

    useForEachForIterTest = true;
    runTests();

    return testCase.summary();
}
