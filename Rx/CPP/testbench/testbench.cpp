// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

// testbench.cpp : Defines the entry point for the console application.
//

#include "cpprx/rx.hpp"

#include <iostream>
#include <iomanip>

using namespace std;

bool IsPrime(int x);

int main(int argc, char* argv[])
{
    const int n = 20;
    std::cout << "first " << n << " primes squared\n";

    rxcpp::DefaultScheduler::Instance().Schedule(
    [=]
    {
        auto values = rxcpp::Range(2); // infinite (until overflow) stream of integers
        auto s1 = rxcpp::from(values)
            .where(IsPrime)
            .select([](int x) { return std::make_pair(x,  x*x); })
            .take(n)
            .subscribe(
            [](pair<int, int> p) {
                cout << p.first << " =square=> " << p.second << endl;
            });
    });
}

bool IsPrime(int x)
{
    if (x < 2) return false;
    for (int i = 2; i <= x/2; ++i)
    {
        if (x % i == 0) 
            return false;
    }
    return true;
}

