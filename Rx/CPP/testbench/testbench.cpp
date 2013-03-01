// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

// testbench.cpp : Defines the entry point for the console application.
//

#include "cpprx/rx.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <exception>
#include <regex>

using namespace std;

bool IsPrime(int x);

void PrintPrimes(int n)
{
    std::cout << "first " << n << " primes squared" << endl;
    auto values = rxcpp::Range(2); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values)
        .where(IsPrime)
        .select([](int x) { return std::make_pair(x,  x*x); })
        .take(n)
        .subscribe(
            [](pair<int, int> p) {
                cout << p.first << " =square=> " << p.second << endl;
            });
}


int main(int argc, char* argv[])
{
    PrintPrimes(20);
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

