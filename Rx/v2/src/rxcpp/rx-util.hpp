// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_UTIL_HPP)
#define RXCPP_RX_UTIL_HPP

#include "rx-includes.hpp"

#if !defined(RXCPP_THREAD_LOCAL)
#if defined(_MSC_VER)
#define RXCPP_THREAD_LOCAL __declspec(thread)
#else
#define RXCPP_THREAD_LOCAL __thread
#endif
#endif

#if !defined(RXCPP_SELECT_ANY)
#if defined(_MSC_VER)
#define RXCPP_SELECT_ANY __declspec(selectany)
#else
#define RXCPP_SELECT_ANY __attribute__((weak))
#endif
#endif

#define RXCPP_CONCAT(Prefix, Suffix) Prefix ## Suffix
#define RXCPP_CONCAT_EVALUATE(Prefix, Suffix) RXCPP_CONCAT(Prefix, Suffix)

#define RXCPP_MAKE_IDENTIFIER(Prefix) RXCPP_CONCAT_EVALUATE(Prefix, __LINE__)

namespace rxcpp {

namespace util {

template<class T, size_t size>
std::vector<T> to_vector(const T (&arr) [size]) {
    return std::vector<T>(std::begin(arr), std::end(arr));
}

namespace detail {

template <class T>
class maybe
{
    bool is_set;
    typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type
        storage;
public:
    maybe()
    : is_set(false)
    {
    }

    maybe(T value)
    : is_set(false)
    {
        new (reinterpret_cast<T*>(&storage)) T(value);
        is_set = true;
    }

    maybe(const maybe& other)
    : is_set(false)
    {
        if (other.is_set) {
            new (reinterpret_cast<T*>(&storage)) T(*other.get());
            is_set = true;
        }
    }
    maybe(maybe&& other)
    : is_set(false)
    {
        if (other.is_set) {
            new (reinterpret_cast<T*>(&storage)) T(std::move(*other.get()));
            is_set = true;
            other.reset();
        }
    }

    ~maybe()
    {
        reset();
    }

    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    bool empty() const {
        return !is_set;
    }

    size_t size() const {
        return is_set ? 1 : 0;
    }

    iterator begin() {
        return reinterpret_cast<T*>(&storage);
    }
    const_iterator begin() const {
        return reinterpret_cast<T*>(&storage);
    }

    iterator end() {
        return reinterpret_cast<T*>(&storage) + size();
    }
    const_iterator end() const {
        return reinterpret_cast<T*>(&storage) + size();
    }

    T* operator->() {
        if (!is_set) abort();
        return reinterpret_cast<T*>(&storage);
    }
    const T* operator->() const {
        if (!is_set) abort();
        return reinterpret_cast<T*>(&storage);
    }

    T& operator*() {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }
    const T& operator*() const {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }

    T& value() {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }
    const T& value() const {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }

    void reset()
    {
        if (is_set) {
            is_set = false;
            reinterpret_cast<T*>(&storage)->~T();
        }
    }

    void reset(T value) {
        if (is_set) {
            *reinterpret_cast<T*>(&storage) = std::move(value);
        } else {
            new (reinterpret_cast<T*>(&storage)) T(std::move(value));
            is_set = true;
        }
    }

    maybe& operator=(const T& other) {
        set(other);
        return *this;
    }
    maybe& operator=(const maybe& other) {
        if (const T* pother = other.get()) {
            set(*pother);
        } else {
            reset();
        }
        return *this;
    }
};

template<typename Function>
class unwinder
{
public:
    ~unwinder()
    {
        if (!!function)
        {
            try {
                (*function)();
            } catch (...) {
                std::unexpected();
            }
        }
    }

    explicit unwinder(Function* functionArg)
        : function(functionArg)
    {
    }

    void dismiss()
    {
        function = nullptr;
    }

private:
    unwinder();
    unwinder(const unwinder&);
    unwinder& operator=(const unwinder&);

    Function* function;
};

template<int N, template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n;

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<5, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = 5;
    static const bool is_arg = true;
    typedef Arg5 result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef arg_resolver_n<n - 1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> prev_type;
    typedef typename std::conditional<Predicate<result_type>::value, this_type, typename prev_type::type>::type type;
    result_type value;
    arg_resolver_n(const Arg0&, const Arg1&, const Arg2&, const Arg3&, const Arg4&, result_type a, ...)
        : value(std::move(a)) {
    }
};

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<4, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = 4;
    static const bool is_arg = true;
    typedef Arg4 result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef arg_resolver_n<n - 1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> prev_type;
    typedef typename std::conditional<Predicate<result_type>::value, this_type, typename prev_type::type>::type type;
    result_type value;
    arg_resolver_n(const Arg0&, const Arg1&, const Arg2&, const Arg3&, result_type a, ...)
        : value(std::move(a)) {
    }
};

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<3, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = 3;
    static const bool is_arg = true;
    typedef Arg3 result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef arg_resolver_n<n - 1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> prev_type;
    typedef typename std::conditional<Predicate<result_type>::value, this_type, typename prev_type::type>::type type;
    result_type value;
    arg_resolver_n(const Arg0&, const Arg1&, const Arg2&, result_type a, ...)
        : value(std::move(a)) {
    }
};

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<2, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = 2;
    static const bool is_arg = true;
    typedef Arg2 result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef arg_resolver_n<n - 1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> prev_type;
    typedef typename std::conditional<Predicate<result_type>::value, this_type, typename prev_type::type>::type type;
    result_type value;
    arg_resolver_n(const Arg0&, const Arg1&, result_type a, ...)
        : value(std::move(a)) {
    }
};

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = 1;
    static const bool is_arg = true;
    typedef Arg1 result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef arg_resolver_n<n - 1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> prev_type;
    typedef typename std::conditional<Predicate<result_type>::value, this_type, typename prev_type::type>::type type;
    result_type value;
    arg_resolver_n(const Arg0&, result_type a, ...)
        : value(std::move(a)) {
    }
};

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<0, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = 0;
    static const bool is_arg = true;
    typedef Arg0 result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef arg_resolver_n<n - 1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> prev_type;
    typedef typename std::conditional<Predicate<result_type>::value, this_type, typename prev_type::type>::type type;
    result_type value;
    explicit arg_resolver_n(result_type a, ...)
        : value(std::move(a)) {
    }
};

template<template<class Arg> class Predicate, class Default, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct arg_resolver_n<-1, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
    static const int n = -1;
    static const bool is_arg = false;
    typedef Default result_type;
    typedef arg_resolver_n<n, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
    typedef this_type type;
    result_type value;
    explicit arg_resolver_n(...)
        : value() {
    }
};


struct tag_unresolvable {};
template<template<class Arg> class Predicate, class Default, class Arg0 = tag_unresolvable, class Arg1 = tag_unresolvable, class Arg2 = tag_unresolvable, class Arg3 = tag_unresolvable, class Arg4 = tag_unresolvable, class Arg5 = tag_unresolvable>
struct arg_resolver
    : public arg_resolver_n<5, Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>
{
};


template<template<class Arg> class Predicate, class Default>
auto resolve_arg()
-> decltype(typename arg_resolver<Predicate, Default>::type()) {
    return  typename arg_resolver<Predicate, Default>::type();
}

template<template<class Arg> class Predicate, class Default,
    class Arg0>
auto resolve_arg(Arg0 a0)
-> decltype(typename arg_resolver<Predicate, Default, Arg0>::type(std::move(a0))) {
    return  typename arg_resolver<Predicate, Default, Arg0>::type(std::move(a0));
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1>
auto resolve_arg(Arg0 a0, Arg1 a1)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1>::type(
        std::move(a0), std::move(a1))) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1>::type(
        std::move(a0), std::move(a1));
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2>
auto resolve_arg(Arg0 a0, Arg1 a1, Arg2 a2)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2>::type(
        std::move(a0), std::move(a1), std::move(a2))) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2>::type(
        std::move(a0), std::move(a1), std::move(a2));
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2, class Arg3>
auto resolve_arg(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3>::type(
        std::move(a0), std::move(a1), std::move(a2), std::move(a3))) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3>::type(
        std::move(a0), std::move(a1), std::move(a2), std::move(a3));
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
auto resolve_arg(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4>::type(
        std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4))) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4>::type(
        std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4));
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto resolve_arg(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>::type(
        std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4), std::move(a5))) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>::type(
        std::move(a0), std::move(a1), std::move(a2), std::move(a3), std::move(a4), std::move(a5));
}

struct arg_resolver_term {};

//
// use to build a set of tags
//
template<class Base, class Next = arg_resolver_term>
struct tag_set : Base
{
    typedef Next next_tag;
};

template<class Tag>
struct arg_resolver_set;

template<>
struct arg_resolver_set<arg_resolver_term>
{
    std::tuple<> operator()(...){
        return std::tuple<>();
    }
};

template<class Tag>
struct arg_resolver_set
{
    typedef arg_resolver_set<typename Tag::next_tag> next_set;
    auto operator()()
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>()),
                            next_set()())) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>()),
                            next_set()());
    }
    template<class Arg0>
    auto operator()(Arg0&& a0)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0))),
                            next_set()(std::forward<Arg0>(a0)))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0))),
                            next_set()(std::forward<Arg0>(a0)));
    }
    template<class Arg0, class Arg1>
    auto operator()(Arg0&& a0, Arg1&& a1)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
    }
    template<class Arg0, class Arg1, class Arg2>
    auto operator()(Arg0&& a0, Arg1&& a1, Arg2&& a2)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
    }
    template<class Arg0, class Arg1, class Arg2, class Arg3>
    auto operator()(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
    }
    template<class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
    auto operator()(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)));
    }
    template<class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
    auto operator()(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5))),
                            next_set()(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)));
    }
};

std::tuple<> resolve_arg_set(arg_resolver_term&&, ...){
    return std::tuple<>();
}

template<class Tag>
auto resolve_arg_set(Tag&&)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>()),
                    resolve_arg_set(typename Tag::next_tag()))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>()),
                    resolve_arg_set(typename Tag::next_tag()));
}
template<class Tag, class Arg0>
auto resolve_arg_set(Tag&&, Arg0&& a0)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0)))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0)));
}
template<class Tag, class Arg0, class Arg1>
auto resolve_arg_set(Tag&&, Arg0&& a0, Arg1&& a1)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
}
template<class Tag, class Arg0, class Arg1, class Arg2>
auto resolve_arg_set(Tag&&, Arg0&& a0, Arg1&& a1, Arg2&& a2)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
}
template<class Tag, class Arg0, class Arg1, class Arg2, class Arg3>
auto resolve_arg_set(Tag&&, Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
}
template<class Tag, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
auto resolve_arg_set(Tag&&, Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)));
}
template<class Tag, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto resolve_arg_set(Tag&&, Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5))),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)));
}

}

}
namespace rxu=util;

}

#define RXCPP_UNWIND(Name, Function) \
    RXCPP_UNWIND_EXPLICIT(uwfunc_ ## Name, Name, Function)

#define RXCPP_UNWIND_AUTO(Function) \
    RXCPP_UNWIND_EXPLICIT(RXCPP_MAKE_IDENTIFIER(uwfunc_), RXCPP_MAKE_IDENTIFIER(unwind_), Function)

#define RXCPP_UNWIND_EXPLICIT(FunctionName, UnwinderName, Function) \
    auto FunctionName = (Function); \
    rxcpp::util::detail::unwinder<decltype(FunctionName)> UnwinderName(std::addressof(FunctionName))

#endif
