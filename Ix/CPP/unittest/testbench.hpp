// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#include <iostream>
#include <iomanip>
#include <exception>
#include <sstream>
#include <utility>
#include <memory>
#include <cstddef>

struct empty_testcase{ void run(){} const char* name(){return 0;} };

template <std::size_t offset>
struct testcase : empty_testcase{};


template <std::size_t begin, std::size_t end>
struct testrange {
    void run(std::size_t& pass, std::size_t& fail) 
    {
        using namespace std;
        {   testcase<begin> a_case;
            if (a_case.name()) {
                std::size_t p=0, f=0;
                cout << "TEST: Running " << a_case.name() << endl;
                try {
                    a_case.run();
                    ++p;
                } catch (logic_error& e) {
                    cerr << "ERRORS:" << endl;
                    cerr << "  " << e.what() << endl;
                    ++f;
                }
                pass += p; fail += f;
            }
        }
        const std::size_t rem = (end-begin-1);
        testrange<begin+1, begin+1+rem/2>().run(pass, fail);
        testrange<begin+1+rem/2, end>().run(pass, fail);
    }
};

template <std::size_t begin>
struct testrange<begin,begin> {
    void run(std::size_t& pass, std::size_t& fail) {};
};

#define TEST(fun_name) \
void fun_name (); \
template <> \
struct testcase<__LINE__> { \
    const char* name() { return(#fun_name); } \
    void run() { fun_name(); } \
}; \
void fun_name()

#define Q_(e) #e
#define Q(e)  Q_(e)
#define TASSERT(expr) \
    { auto e = (expr); if (!e) { throw std::logic_error(__FILE__ "(" Q(__LINE__) "): TASSERT("#expr")"); }  }

struct errmsg 
{
    std::shared_ptr<std::stringstream> msg;
    errmsg() : msg(new std::stringstream)
    {}

    template <class T>
    errmsg& operator<<(T value)
    {
        (*msg) << value;
        return *this;
    }
    std::string str() { return msg->str(); }
};

#define TEST_WHERE  __FILE__ "(" Q(__LINE__) "): "
#define VERIFY(expr) \
    { auto e = (expr); if (!e) { throw std::logic_error(TEST_WHERE "VERIFY("#expr")"); }  }
#define VERIFY_EQ(expected, actual) \
    { auto e = (expected); auto a = (actual); \
      if (!(e == a)) { \
        throw std::logic_error( \
          (errmsg() << TEST_WHERE << "(" << e << ")!=(" << a << ") in VERIFY_EQ("#expected","#actual")").str() );}}


