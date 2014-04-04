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
            new (reinterpret_cast<T*>(&storage)) T(other.get());
            is_set = true;
        }
    }
    maybe(maybe&& other)
    : is_set(false)
    {
        if (other.is_set) {
            new (reinterpret_cast<T*>(&storage)) T(std::move(other.get()));
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

    T& get() {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }
    const T& get() const {
        if (!is_set) abort();
        return *reinterpret_cast<const T*>(&storage);
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
