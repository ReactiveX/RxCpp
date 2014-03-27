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
            //std::fill_n(reinterpret_cast<char*>(&storage), sizeof(T), 0);
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

template<int N, int Index>
struct select_arg
{
    template<class GArg, class... GArgN>
    static auto get(const GArg&, const GArgN&... gan)
        -> decltype(select_arg<N, Index + 1>::get(gan...)) {
        return      select_arg<N, Index + 1>::get(gan...);
    }
};
template<int N>
struct select_arg<N, N>
{
    template<class GArg, class... GArgN>
    static auto get(const GArg& ga, const GArgN&...)
        -> decltype(ga) {
        return      ga;
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
    struct tag_value {};
    template<class Value>
    resolved_arg(const Value& v, tag_value&&)
        : value(v)
    {
    }
    template<class... CArgN>
    static this_type make(const CArgN&... can) {
        return this_type(select_arg<n, 0>::get(can...), tag_value());
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
    static this_type make(const CArgN&... can) {
        return this_type();
    }
};


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

template<template<class Arg> class Predicate, class Default, class... ArgN>
auto resolve_arg(const ArgN&... an)
-> typename arg_resolver<Predicate, Default, ArgN...>::resolved_type
{
    return  arg_resolver<Predicate, Default, ArgN...>::resolved_type::make(an...);
}

template<class... TagN>
struct tag_set {};

template<class TagSet>
struct arg_resolver_set;

template<class Tag0, class... TagN>
struct arg_resolver_set<tag_set<Tag0, TagN...>>
{
    template<class Tag, class... ArgN>
    struct expand
    {
        typedef typename arg_resolver<Tag::template predicate, typename Tag::default_type, ArgN...>::resolved_type type;
    };

    template<class Result>
    struct expanded;

    template<class Resolved0, class... ResolvedN>
    struct expanded<std::tuple<Resolved0, ResolvedN...>>
    {
        typedef std::tuple<Resolved0, ResolvedN...> result_type;
        template<class... ArgN>
        static result_type make(const ArgN&... an) {
            return result_type(Resolved0::make(an...), ResolvedN::make(an...)...);
        }
    };

    template<class... ArgN>
    auto operator()(const ArgN&... an)
    ->          std::tuple< typename expand<Tag0, ArgN...>::type,
                            typename expand<TagN, ArgN...>::type...> {
        typedef std::tuple< typename expand<Tag0, ArgN...>::type,
                            typename expand<TagN, ArgN...>::type...> out_type;
        static_assert(std::tuple_size<out_type>::value == (sizeof...(TagN) + 1), "tuple must have a value per tag");
        return  expanded<out_type>::make(an...);
    }
};

template<class TagSet, class... ArgN>
auto resolve_arg_set(const TagSet&, const ArgN&... an)
    -> decltype(arg_resolver_set<TagSet>()(an...)) {
    return      arg_resolver_set<TagSet>()(an...);
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
