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
            std::fill_n(reinterpret_cast<char*>(&storage), sizeof(T), 0);
        }
    }

    template<class U>
    void reset(U&& value) {
        reset();
        new (reinterpret_cast<T*>(&storage)) T(std::forward<U>(value));
        is_set = true;
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

#if RXCPP_USE_VARIADIC_TEMPLATES

template<int N, int Index>
struct select_arg
{
    template<class GArg, class... GArgN>
    static auto get(GArg&&, GArgN&&... gan)
        -> decltype(select_arg<N, Index + 1>::get(std::forward<GArgN>(gan)...)) {
        return      select_arg<N, Index + 1>::get(std::forward<GArgN>(gan)...);
    }
};
template<int N>
struct select_arg<N, N>
{
    template<class GArg, class... GArgN>
    static auto get(GArg&& ga, GArgN&&...)
        -> decltype(std::forward<GArg>(ga)) {
        return      std::forward<GArg>(ga);
    }
};

template<int N, class T>
struct resolved_arg
{
    typedef resolved_arg<N, T> this_type;
    static const int n = N;
    static const bool is_arg = true;
    typedef T result_type;
    result_type value;
    template<class Value>
    explicit resolved_arg(Value&& v)
    : value(std::forward<Value>(v))
    {
    }
    template<class... CArgN>
    static this_type make(CArgN&&... can) {
        return this_type(select_arg<n, 0>::get(std::forward<CArgN>(can)...));
    }
};

template<class T>
struct resolved_arg<-1, T>
{
    typedef resolved_arg<-1, T> this_type;
    static const int n = -1;
    static const bool is_arg = false;
    typedef T result_type;
    result_type value;
    template<class... CArgN>
    static this_type make(CArgN&&... can) {
        return this_type();
    }
};

template<int Size, int N, template<class Arg> class Predicate, class Default, class... ArgN>
struct arg_resolver_n;

template<int Size, int N, template<class Arg> class Predicate, class Default, class Arg, class... ArgN>
struct arg_resolver_n<Size, N, Predicate, Default, Arg, ArgN...>
{
    static const int n = N;
    typedef resolved_arg<n, Arg> resolved_type;
    typedef arg_resolver_n<Size, n, Predicate, Default, Arg, ArgN...> this_type;
    typedef arg_resolver_n<Size, n + 1, Predicate, Default, ArgN...> next_type;
    typedef typename std::conditional<Predicate<Arg>::value, this_type, typename next_type::type>::type type;
};

template<int Size, template<class Arg> class Predicate, class Default>
struct arg_resolver_n<Size, Size, Predicate, Default>
{
    static const int n = -1;
    typedef resolved_arg<n, Default> resolved_type;
    typedef arg_resolver_n<Size, Size, Predicate, Default> this_type;
    typedef this_type type;
};
#else
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
    arg_resolver_n(result_type v)
        : value(std::move(v))
    {
    }
    template<class R>
    static this_type make(const Arg0&, const Arg1&, const Arg2&, const Arg3&, const Arg4&, R&& a)
    {
        return this_type(std::forward<R>(a));
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
    arg_resolver_n(result_type v)
        : value(std::move(v))
    {
    }
    template<class R>
    static this_type make(const Arg0&, const Arg1&, const Arg2&, const Arg3&, R&& a, const Arg5&)
    {
        return this_type(std::forward<R>(a));
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
    arg_resolver_n(result_type v)
        : value(std::move(v))
    {
    }
    template<class R>
    static this_type make(const Arg0&, const Arg1&, const Arg2&, R&& a, const Arg4&, const Arg5&)
    {
        return this_type(std::forward<R>(a));
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
    arg_resolver_n(result_type v)
        : value(std::move(v))
    {
    }
    template<class R>
    static this_type make(const Arg0&, const Arg1&, R&& a, const Arg3&, const Arg4&, const Arg5&)
    {
        return this_type(std::forward<R>(a));
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
    arg_resolver_n(result_type v)
        : value(std::move(v))
    {
    }
    template<class R>
    static this_type make(const Arg0&, R&& a, const Arg2&, const Arg3&, const Arg4&, const Arg5&)
    {
        return this_type(std::forward<R>(a));
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
    arg_resolver_n(result_type v)
        : value(std::move(v))
    {
    }
    template<class R>
    static this_type make(R&& a, const Arg1&, const Arg2&, const Arg3&, const Arg4&, const Arg5&)
    {
        return this_type(std::forward<R>(a));
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
    static this_type make(const Arg0&, const Arg1&, const Arg2&, const Arg3&, const Arg4&, const Arg5&)
    {
        return this_type();
    }
};
#endif


#if RXCPP_USE_VARIADIC_TEMPLATES
template<int N, bool... PredicateN>
struct select_first;

template<int N, bool Predicate0, bool... PredicateN>
struct select_first<N, Predicate0, PredicateN...>
{
    static const int value = Predicate0 ? N : select_first<N + 1, PredicateN...>::value;
};
template<int N>
struct select_first<N>
{
    static const int value = -1;
};

template<int N, int Cursor, class... SArgN>
struct select_n
{
    struct type;
};

template<int N, int Cursor, class SArg0, class... SArgN>
struct select_n<N, Cursor, SArg0, SArgN...> : public select_n<N, Cursor + 1, SArgN...>
{
};

template<int N, class SArg0, class... SArgN>
struct select_n<N, N, SArg0, SArgN...>
{
    typedef SArg0 type;
};

template<template<class Arg> class Predicate, class Default, class... ArgN>
struct arg_resolver
{
    typedef select_first<0, Predicate<ArgN>::value...> match;
    typedef typename std::conditional<
        match::value != -1,
        typename select_n<match::value, 0, ArgN...>::type,
        Default>::type result_type;
    typedef resolved_arg<match::value, result_type> resolved_type;
};
#else
struct tag_unresolvable {};
template<template<class Arg> class Predicate, class Default, class Arg0 = tag_unresolvable, class Arg1 = tag_unresolvable, class Arg2 = tag_unresolvable, class Arg3 = tag_unresolvable, class Arg4 = tag_unresolvable, class Arg5 = tag_unresolvable>
struct arg_resolver
{
    typedef typename arg_resolver_n<5, Predicate, Default, typename std::decay<Arg0>::type, typename std::decay<Arg1>::type, typename std::decay<Arg2>::type, typename std::decay<Arg3>::type, typename std::decay<Arg4>::type, typename std::decay<Arg5>::type>::type type;
};
#endif

#if RXCPP_USE_VARIADIC_TEMPLATES
template<template<class Arg> class Predicate, class Default,
    class... ArgN>
auto resolve_arg(ArgN&&... an)
-> decltype(arg_resolver<Predicate, Default, ArgN...>::resolved_type::make(std::forward<ArgN>(an)...)) {
    return  arg_resolver<Predicate, Default, ArgN...>::resolved_type::make(std::forward<ArgN>(an)...);
}
#else
template<template<class Arg> class Predicate, class Default>
auto resolve_arg()
-> decltype(typename arg_resolver<Predicate, Default>::type::make(tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable())) {
    return  typename arg_resolver<Predicate, Default>::type::make(tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable());
}

template<template<class Arg> class Predicate, class Default,
    class Arg0>
auto resolve_arg(Arg0&& a0)
-> decltype(typename arg_resolver<Predicate, Default, Arg0>::type::make(std::forward<Arg0>(a0), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable())) {
    return  typename arg_resolver<Predicate, Default, Arg0>::type::make(std::forward<Arg0>(a0), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable());
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1>
auto resolve_arg(Arg0&& a0, Arg1&& a1)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable())) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), tag_unresolvable(), tag_unresolvable(), tag_unresolvable(), tag_unresolvable());
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2>
auto resolve_arg(Arg0&& a0, Arg1&& a1, Arg2&& a2)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), tag_unresolvable(), tag_unresolvable(), tag_unresolvable())) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), tag_unresolvable(), tag_unresolvable(), tag_unresolvable());
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2, class Arg3>
auto resolve_arg(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), tag_unresolvable(), tag_unresolvable())) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), tag_unresolvable(), tag_unresolvable());
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
auto resolve_arg(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), tag_unresolvable())) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), tag_unresolvable());
}

template<template<class Arg> class Predicate, class Default,
    class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto resolve_arg(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
-> decltype(typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5))) {
    return  typename arg_resolver<Predicate, Default, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5>::type::make(
        std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5));
}
#endif

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
    inline std::tuple<> operator()(...){
        return std::tuple<>();
    }
};

template<class Tag>
struct arg_resolver_set
{
    typedef arg_resolver_set<typename Tag::next_tag> next_set;
#if RXCPP_USE_VARIADIC_TEMPLATES
    template<class... ArgN>
    auto operator()(ArgN&&... an)
    -> decltype(std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<ArgN>(an)...)),
                            next_set()(std::forward<ArgN>(an)...))) {
        return  std::tuple_cat(
                            std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(std::forward<ArgN>(an)...)),
                            next_set()(std::forward<ArgN>(an)...));
    }
#else
    inline auto operator()()
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
#endif
};

#if RXCPP_USE_VARIADIC_TEMPLATES
template<class... ArgN>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, ArgN&&... ) {
    return std::tuple<>();
}
#else
inline std::tuple<> resolve_arg_set(arg_resolver_term&&) {
    return std::tuple<>();
}

template<class Arg0>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, Arg0&& ) {
    return std::tuple<>();
}

template<class Arg0, class Arg1>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, Arg0&& , Arg1&& ) {
    return std::tuple<>();
}

template<class Arg0, class Arg1, class Arg2>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, Arg0&& , Arg1&& , Arg2&& ) {
    return std::tuple<>();
}

template<class Arg0, class Arg1, class Arg2, class Arg3>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, Arg0&& , Arg1&& , Arg2&& , Arg3&& ) {
    return std::tuple<>();
}

template<class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, Arg0&& , Arg1&& , Arg2&& , Arg3&& , Arg4&& ) {
    return std::tuple<>();
}

template<class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
inline std::tuple<> resolve_arg_set(arg_resolver_term&&, Arg0&& , Arg1&& , Arg2&& , Arg3&& , Arg4&& , Arg5&& ) {
    return std::tuple<>();
}
#endif

#if RXCPP_USE_VARIADIC_TEMPLATES
template<class Tag, class... ArgN>
auto resolve_arg_set(Tag&&, ArgN&&... an)
    -> decltype(std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<ArgN>(an)...)),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<ArgN>(an)...))) {
    return      std::tuple_cat(
                    std::make_tuple(resolve_arg<Tag::template predicate, typename Tag::default_type>(
                        std::forward<ArgN>(an)...)),
                    resolve_arg_set(typename Tag::next_tag(),
                        std::forward<ArgN>(an)...));
}
#else
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
#endif

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
