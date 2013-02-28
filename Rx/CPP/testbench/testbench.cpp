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
    bool done = false;
    auto dispatcher = std::make_shared<rxcpp::ObserveOnDispatcherOp>();
    auto values = rxcpp::Range(2); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values)
        .where(IsPrime)
        .select([](int x) { return std::make_pair(x,  x*x); })
        .take(n)
        .on_dispatcher(dispatcher)
        .subscribe(
            [](pair<int, int> p) {
                cout << p.first << " =square=> " << p.second << endl;
            },
            [&done](){done = true;}, 
            [&done](const std::exception_ptr&){done = true;});

    std::cout << "first " << n << " primes squared" << endl;
    while(!done){dispatcher->dispatch_one();}
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

