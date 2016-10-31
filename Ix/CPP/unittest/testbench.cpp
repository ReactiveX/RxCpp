// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <string>
#include <complex>

#include <ctime>
#include <cstddef>

#include <boost/lambda/core.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/iterator.hpp>

#include "cpplinq/linq.hpp"

#include "testbench.hpp"

using namespace std;
using namespace cpplinq;

struct int_iter
    : std::iterator<std::random_access_iterator_tag, int, std::ptrdiff_t, int*, int>
{
    int_iter(value_type i = 0, value_type step = 1) : value(i), step(step)
    {}

    value_type operator*() const { 
        return value; 
    }
    int_iter& operator++() { 
        value+=step; return *this; 
    }
    int_iter& operator--() {
        value-=step; return *this;
    }
    int_iter& operator+=(std::ptrdiff_t offset) {
        value += step*offset; return *this;
    }
    int_iter& operator-=(std::ptrdiff_t offset) {
        value -= step*offset; return *this;
    }
    std::ptrdiff_t operator-(int_iter rhs) const {
        return std::ptrdiff_t((value - rhs.value)/step);
    }
    bool operator==(int_iter other) const {
        return value == other.value; 
    }
    bool operator!=(int_iter other) const {
        return !(*this == other);
    }
    bool operator<(int_iter other) const { return (*this - other) < 0; }
    bool operator>(int_iter other) const { return (*this - other) > 0; }
    bool operator<=(int_iter other) const { return (*this - other) <= 0; }
    bool operator>=(int_iter other) const { return (*this - other) >= 0; }

    value_type value;
    value_type step;
};
int_iter operator+(int_iter lhs, std::ptrdiff_t rhs) {
    return lhs+=rhs;
}
int_iter operator+(std::ptrdiff_t lhs, int_iter rhs) {
    return rhs+=lhs;
}
int_iter operator-(int_iter lhs, std::ptrdiff_t rhs) {
    return lhs-=rhs;
}
struct int_range
{
    typedef int_iter iterator;
    typedef int_iter::value_type value_type;
    int_range(value_type begin, value_type end, value_type step = 1) 
        : b(begin, step), e(end, step)
    {
        if (step == 0) {
            throw std::logic_error("bad step");
        }
        if (abs(end - begin) % abs(step) != 0) {
            end -= (end-begin)%abs(step);
        }
    }
    int_iter begin() const { return int_iter(b);}
    int_iter end() const { return int_iter(e); }

    iterator b, e;
};

vector<int> vector_range(int start, int end)
{
    vector<int> v;
    for (int i = start; i < end; ++i)
        v.push_back(i);
    return v;
}

TEST(test_selection)
{
    vector<int> v = vector_range(0, 10);

    auto v2 = from(v)
             .select([](int x) { return x*2; });

    auto result = accumulate(begin(v2), end(v2), int(0));

    VERIFY_EQ(90, result);
}

TEST(test_where)
{
    vector<int> v = vector_range(0, 10);
    auto v2 = from(v)
             .where([](int x) { return x % 2;});

    VERIFY_EQ(1, *v2.begin());

    auto result = accumulate(begin(v2), end(v2), int(0));

    VERIFY_EQ(25, result);
}


bool is_prime(int x) {
    if (x < 2) {return false;}
    if (x == 2) {return true;}
    for (int i = x/2; i >= 2; --i) {
        if (x % i == 0) { return false;}
    }
    return true;
};

template <class It>
void display(It start, It end)
{
    int i = 0;
    for_each(start, end, [&](typename iterator_traits<It>::value_type x){
        if (++i % 10 == 0) {
            cout << endl;
        }
        cout << x << " ";
    });
    cout << endl;
}

TEST(test_whereselect)
{
    auto xs = int_range(0,100);
    auto ys = from(xs)
             .where(is_prime)
             .select([](int x){ return x*x; });
    auto result = accumulate(begin(ys), end(ys), int(0));

    //display(begin(ys), end(ys));

    // primes < 100
    VERIFY_EQ(65796, result);
}
TEST(test_where_modification)
{
    vector<int> xs = vector_range(0, 100);

    auto ys = from(xs)
             .where(is_prime);
    std::fill(begin(ys), end(ys), int(0));

    auto result = accumulate(begin(xs), end(xs), int(0));

    //display(begin(ys), end(ys));

    // non-primes < 100
    VERIFY_EQ(3890, result);
}

TEST(test_where_any)
{
    using namespace boost::lambda;

    vector<int> xs(200);
    fill(begin(xs), end(xs), int(0));
    auto it = xs.begin();
    *it = 2;

    for(;;) {
        auto last = *it++;
        auto primes = from(int_range(last+1, -1))
                      .where([&](int x){ 
                                 return from(begin(xs), it)
                                       //.all([&](int d){return x%d;});
                                       .all(x % boost::lambda::_1);
                             });
        *it = *primes.begin();
        if ((*it)>=100) {
            break;
        }
    };
    xs.erase(it, xs.end());

    auto result = accumulate(begin(xs), end(xs), int(0));

    //display(begin(xs), end(xs));

    // primes < 100
    VERIFY_EQ(1060, result);
}

TEST(test_take)
{
    auto zero_one = from(int_range(0, -1))
                    .take(2);

    VERIFY_EQ(0, zero_one[0]);
    VERIFY_EQ(1, zero_one[1]);

    auto ten = from(int_range(0, -1))
               .skip(10);

    VERIFY_EQ(10, ten[0]);
    VERIFY_EQ(11, ten[1]);
}

vector<int> some_primes(std::size_t howMany)
{
    auto xs = from(int_range(0, -1))
             .where(is_prime)
             .take(howMany);
    auto v = vector<int>(begin(xs), end(xs));
    return v;
}

TEST(test_groupby)
{
    auto xs = some_primes(40);
    //display(begin(xs), end(xs));

    auto grouped = 
        from(xs)
        .groupby([](int i){return i % 10; });

    VERIFY_EQ(6, from(grouped).count());
    for(auto group = begin(grouped); group != end(grouped); ++group) {
        //cout << "key = " << group->key << endl 
        //     << "| ";
        for (auto elem = group->begin(); elem != group->end(); ++elem) {
            //cout << *elem << "  ";
        }
        //cout << endl;

        switch(group->key) {
        case 2: VERIFY_EQ(1, from(*group).count()); break;
        case 3: VERIFY_EQ(11, from(*group).count()); break;
        case 5: VERIFY_EQ(1, from(*group).count()); break;
        case 7: VERIFY_EQ(11, from(*group).count()); break;
        case 1: VERIFY_EQ(8, from(*group).count()); break;
        case 9: VERIFY_EQ(8, from(*group).count()); break;
        }
    }
}

TEST(test_symbolname)
{
    auto complexQuery = 
        from(int_range(0,100000))
        .select([](int x){ return x*2;})
        .where([](int x){ return x%7; })
        .skip(20);

    //cout << " type name: " << typeid(complexQuery1).name() << endl;

    
    //auto complexQuery =
    //    complexQuery1.groupby([](int x) { return x%5; })
    //    .take(3)
    //    ;
    
    
    cout << "type name: " << typeid(complexQuery).name() << endl;
    cout << "type name length: " << strlen(typeid(complexQuery).name()) << endl;

    auto iter = complexQuery.begin();
    cout << "iterator name: " << typeid(iter).name() << endl;
    cout << "iterator name length: " << strlen(typeid(iter).name()) << endl;
}

TEST(test_cast)
{
    auto q = from(int_range(0,10))
             .cast<bool>();
    VERIFY_EQ(false, q[0]);
    VERIFY_EQ(true, q[1]);
    VERIFY_EQ(true, q[2]);
    VERIFY((std::is_same<decltype(q[0]), bool>::value));
}

TEST(test_contains)
{
    auto q = from(int_range(0,10))
             .select([](int x){return x*2;});
    VERIFY(q.contains(4));
    VERIFY(!q.contains(5));
}

TEST(test_element_accessors)
{
    vector<int> v(int_iter(0), int_iter(10));
    auto q = from(v)
             .where([](int x){return x%2==0;});

    VERIFY_EQ(0, q.first());
    VERIFY_EQ(8, q.last());
    VERIFY_EQ(6, q.element_at(3));

    bool thrown = false;
    try { q.element_at(5); } catch (std::logic_error&) { thrown = true; }
    VERIFY(thrown);

    q.first() = 1;
    q.last() = 42;

    // note: because the vector now contains { 1, 1, 2, 3, ... 7, 8, 42 }, the first
    //  even number is now '2'
    VERIFY_EQ(2, q.first());
    VERIFY_EQ(42, q.last());
}

//////////////////// New style cursors ////////////////////

TEST(test_cursor_dynamic)
{
    dynamic_cursor<int> dc(int_iter(0), int_iter(2));

    VERIFY(!dc.empty());
    VERIFY_EQ(0, dc.get());
    dc.inc();
    VERIFY_EQ(1, dc.get());
    dc.inc();
    VERIFY(dc.empty());
}

TEST(test_selectmany)
{
    int_range range1(0, 3);
    auto range2 = 

        from(range1)
        .select_many( 
        [](int x)
        {
            return int_range(0, x+1);
        });

    auto cur = range2.get_cursor();

    // expected: 0, 0, 1, 0, 1, 2.
    VERIFY(!cur.empty());

    VERIFY_EQ(0, cur.get()); 
    cur.inc();
    VERIFY(!cur.empty());

    VERIFY_EQ(0, cur.get()); 
    cur.inc();
    VERIFY(!cur.empty());
    
    VERIFY_EQ(1, cur.get()); 
    cur.inc();
    VERIFY(!cur.empty());

    VERIFY_EQ(0, cur.get()); 
    cur.inc();
    VERIFY(!cur.empty());

    VERIFY_EQ(1, cur.get()); 
    cur.inc();
    VERIFY(!cur.empty());

    VERIFY_EQ(2, cur.get()); 
    cur.inc();
    VERIFY(cur.empty());
}

TEST(test_cursor_selectmany2)
{
    int_range range1(0, 3);
    auto range2 = from(range1)
        .select_many(
        [](int x) 
        {
            return int_range(0, x+1);
        });

    // expected: 0, 0, 1, 0, 1, 2.
    int expect[] = { 0, 0, 1, 0, 1, 2 };

    VERIFY_EQ(_countof(expect), std::distance(range2.begin(), range2.end()));
    VERIFY_EQ(_countof(expect), std::distance(range2.begin(), range2.end()));

    auto result = std::mismatch(expect, expect + _countof(expect), range2.begin());
    if (result.second != range2.end()) {
        cout << "mismatch: " << *result.first << " != " << *result.second << endl;
    }
    VERIFY( result.second == range2.end());
}

TEST(test_late_bind)
{
    int_range range1(0, 100);
    linq_driver<dynamic_collection<int>> range2 = from(range1).late_bind();

    VERIFY_EQ(1, range2.element_at(1));

    auto q1 = from(range1).select([](int x){ return x*2; }).where([](int x){ return x%10!=0; });

    cout << "typeof q1 ==> " << typeid(q1).name() << endl;
    cout << "typeof q1.late_bind() ==> " << typeid(q1.late_bind()).name() << endl;
}

struct stopwatch
{
    time_t t0, t1;
    void start() {
        t1 = t0 = clock();
    }
    void stop() {
        t1 = clock();
    }
    double value() const {
        return double(t1-t0)/CLOCKS_PER_SEC;
    }
};

template <class Fn>
void test_perf(Fn fn)
{
    // warmup
    fn(10);
    
    int n = 100;
    stopwatch sw;
    for(;;) 
    {
        cout << "trying n=" << n << endl;
        sw.start();
        fn(n);
        sw.stop();
        if (sw.value() > 2.0) {
            break;
        }
        n *= 2;
    }
    cout << "time   = " << sw.value() << " s\n";
    cout << "steps  = " << n << "\n";
    cout << "t/step = " << (sw.value() * 1e9 / n) << " ns\n";
    cout << "step/t = " << (n / sw.value()) << " Hz\n";
}

TEST(test_performance)
{
    // http://projecteuler.net/problem=8
    // 
    // Find the greatest product of five consecutive digits in the 1000-digit number.
    //

    static const char num[] = 
         "73167176531330624919225119674426574742355349194934"
         "96983520312774506326239578318016984801869478851843"
         "85861560789112949495459501737958331952853208805511"
         "12540698747158523863050715693290963295227443043557"
         "66896648950445244523161731856403098711121722383113"
         "62229893423380308135336276614282806444486645238749"
         "30358907296290491560440772390713810515859307960866"
         "70172427121883998797908792274921901699720888093776"
         "65727333001053367881220235421809751254540594752243"
         "52584907711670556013604839586446706324415722155397"
         "53697817977846174064955149290862569321978468622482"
         "83972241375657056057490261407972968652414535100474"
         "82166370484403199890008895243450658541227588666881"
         "16427171479924442928230863465674813919123162824586"
         "17866458359124566529476545682848912883142607690042"
         "24219022671055626321111109370544217506941658960408"
         "07198403850962455444362981230987879927244284909188"
         "84580156166097919133875499200524063689912560717606"
         "05886116467109405077541002256983155200055935729725"
         "71636269561882670428252483600823257530420752963450";

    auto task = [&](int n){
        for (int i = 0; i < n; ++i) {
            auto range1 = int_range(0, _countof(num)-5); // 5 digit numbers, plus null terminator

            auto products = from(range1)
                .select([&](int i){ return num+i;})
                .where([&](const char* s){ return !from(s, s+5).contains('0'); })
                .select([&](const char* s) { return from(s, s+5).select([](char c){ return c - '0'; })
                                                                .aggregate(std::multiplies<int>()); });
            
            auto result = products.max();
            if (n == 1) {
                cout << "result = " << result << endl;
            }
        }
    };
    cout << "length of input: " << (_countof(num)-1) << endl;

    task(1);
    cout << endl;

#ifdef PERF
    test_perf(task);
    cout << endl;
#endif
}

// SUM TESTS

TEST(test_sum_ints)
{
    vector<int> numbers{1, 2, 3, 4, 5};

	auto result = cpplinq::from(numbers);
    auto r2 = result.sum();

	VERIFY_EQ(15, r2);
}

TEST(test_sum_ints_with_seed)
{
    vector<int> numbers{1, 2, 3, 4, 5};

	auto result = cpplinq::from(numbers).sum(10);

	VERIFY_EQ(25, result);
}

TEST(test_sum_floats) {
	vector<float> numbers{ 1.0f,2.0f,3.0f,4.0f,5.0f };

	auto result = cpplinq::from(numbers).sum();

	VERIFY_EQ(15.0f, result);
}

TEST(test_sum_doubles) {
	vector<double> numbers { 1.0,2.0,3.0,4.0,5.0 };

	auto result = cpplinq::from(numbers).sum();

	VERIFY_EQ(15.0, result);
}

TEST(test_sum_complex) {
	using namespace std::complex_literals;

	vector<complex<double>> numbers{ 1i, 1.0 + 2i, 2.0 + 3i };

	auto sum = cpplinq::from(numbers).sum();

	VERIFY_EQ(3.0, sum.real());
	VERIFY_EQ(6.0, sum.imag());
}

TEST(test_sum_with_projection_lambda) {
	vector<tuple<int>> numbers { std::tuple<int>(0), std::tuple<int>(1), std::tuple<int>(2) };

	auto result = cpplinq::from(numbers).sum([](std::tuple<int>& x){
                return std::get<0>(x);
        });

	VERIFY_EQ(3.0, result);
}

TEST(test_sum_with_projection_lambda_and_seed) {
	vector<tuple<int>> numbers { std::tuple<int>(0), std::tuple<int>(1), std::tuple<int>(2) };

	auto result = cpplinq::from(numbers).sum([](std::tuple<int>& x){
                return std::get<0>(x);
        }, 10);

	VERIFY_EQ(13.0, result);
}

int getValue(std::tuple<int> x)
{
    return std::get<0>(x);
}

TEST(test_sum_with_projection_function_pointer) {
	vector<tuple<int>> numbers { std::tuple<int>(0), std::tuple<int>(1), std::tuple<int>(2) };

	auto result = cpplinq::from(numbers).sum(getValue);

	VERIFY_EQ(3.0, result);
}

int main(int argc, char** argv)
{
    std::size_t pass = 0, fail = 0;
    testrange<0,__LINE__>().run(pass, fail);


    cout << "pass: " << pass << ", fail: " << fail << endl;
    if (fail){
        cerr << "ERRORS PRESENT." << endl;
    } else if (!pass) {
        cerr << "ERROR, no tests run" << endl;
    }
}
