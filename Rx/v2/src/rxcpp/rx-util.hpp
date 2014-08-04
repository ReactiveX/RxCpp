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

template<class T>
std::vector<T> to_vector(std::initializer_list<T> il) {
    return std::vector<T>(il);
}

template<class T0, class... TN>
typename std::enable_if<!std::is_array<T0>::value && std::is_pod<T0>::value, std::vector<T0>>::type to_vector(T0 t0, TN... tn) {
    return to_vector({t0, tn...});
}

template<class T, T... ValueN>
struct values {};

template<class T, int Remaining, T Step = 1, T Cursor = 0, T... ValueN>
struct values_from;

template<class T, T Step, T Cursor, T... ValueN>
struct values_from<T, 0, Step, Cursor, ValueN...>
{
    typedef values<T, ValueN...> type;
};

template<class T, int Remaining, T Step, T Cursor, T... ValueN>
struct values_from
{
    typedef typename values_from<T, Remaining - 1, Step, Cursor + Step, ValueN..., Cursor>::type type;
};

template<bool... BN>
struct all_true;

template<bool B0>
struct all_true<B0>
{
    static const bool value = B0;
};
template<bool B0, bool... BN>
struct all_true<B0, BN...>
{
    static const bool value = B0 && all_true<BN...>::value;
};

namespace detail {

template<class F, class... ParamN, int... IndexN>
auto apply(std::tuple<ParamN...> p, values<int, IndexN...>, F& f)
    -> decltype(f(std::forward<ParamN>(std::get<IndexN>(p))...)) {
    return      f(std::forward<ParamN>(std::get<IndexN>(p))...);
}
template<class F, class... ParamN, int... IndexN>
auto apply(std::tuple<ParamN...> p, values<int, IndexN...>, const F& f)
    -> decltype(f(std::forward<ParamN>(std::get<IndexN>(p))...)) {
    return      f(std::forward<ParamN>(std::get<IndexN>(p))...);
}

}

template<class F, class... ParamN>
auto apply(std::tuple<ParamN...> p, F& f)
    -> decltype(detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f)) {
    return      detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f);
}
template<class F, class... ParamN>
auto apply(std::tuple<ParamN...> p, const F& f)
    -> decltype(detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f)) {
    return      detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f);
}

namespace detail {

template<class F>
struct apply_to
{
    F to;

    explicit apply_to(F f)
        : to(std::move(f))
    {
    }

    template<class... ParamN>
    auto operator()(std::tuple<ParamN...> p)
        -> decltype(rxcpp::util::apply(std::move(p), to)) {
        return      rxcpp::util::apply(std::move(p), to);
    }
    template<class... ParamN>
    auto operator()(std::tuple<ParamN...> p) const
        -> decltype(rxcpp::util::apply(std::move(p), to)) {
        return      rxcpp::util::apply(std::move(p), to);
    }
};

}

template<class F>
auto apply_to(F f)
    ->      detail::apply_to<F> {
    return  detail::apply_to<F>(std::move(f));
}

namespace detail {

struct pack
{
    template<class... ParamN>
    auto operator()(ParamN... pn)
        -> decltype(std::make_tuple(std::move(pn)...)) {
        return      std::make_tuple(std::move(pn)...);
    }
    template<class... ParamN>
    auto operator()(ParamN... pn) const
        -> decltype(std::make_tuple(std::move(pn)...)) {
        return      std::make_tuple(std::move(pn)...);
    }
};

}

inline auto pack()
    ->      detail::pack {
    return  detail::pack();
}

template <class D>
struct resolve_type;

template <template<class... TN> class Deferred, class... AN>
struct defer_trait
{
    template<bool R>
    struct tag_valid {static const bool valid = true; static const bool value = R;};
    struct tag_not_valid {static const bool valid = false; static const bool value = false;};
    typedef Deferred<typename resolve_type<AN>::type...> resolved_type;
    template<class... CN>
    static auto check(int) -> tag_valid<resolved_type::value>;
    template<class... CN>
    static tag_not_valid check(...);

    typedef decltype(check<AN...>(0)) tag_type;
    static const bool valid = tag_type::valid;
    static const bool value = tag_type::value;
    static const bool not_value = valid && !value;
};

template <template<class... TN> class Deferred, class... AN>
struct defer_type
{
    template<class R>
    struct tag_valid {typedef R type; static const bool value = true;};
    struct tag_not_valid {typedef void type; static const bool value = false;};
    typedef Deferred<typename resolve_type<AN>::type...> resolved_type;
    template<class... CN>
    static auto check(int) -> tag_valid<resolved_type>;
    template<class... CN>
    static tag_not_valid check(...);

    typedef decltype(check<AN...>(0)) tag_type;
    typedef typename tag_type::type type;
    static const bool value = tag_type::value;
};

template <template<class... TN> class Deferred, class... AN>
struct defer_value_type
{
    template<class R>
    struct tag_valid {typedef R type; static const bool value = true;};
    struct tag_not_valid {typedef void type; static const bool value = false;};
    typedef Deferred<typename resolve_type<AN>::type...> resolved_type;
    template<class... CN>
    static auto check(int) -> tag_valid<typename resolved_type::value_type>;
    template<class... CN>
    static tag_not_valid check(...);

    typedef decltype(check<AN...>(0)) tag_type;
    typedef typename tag_type::type type;
    static const bool value = tag_type::value;
};

template <template<class... TN> class Deferred, class... AN>
struct defer_seed_type
{
    template<class R>
    struct tag_valid {typedef R type; static const bool value = true;};
    struct tag_not_valid {typedef void type; static const bool value = false;};
    typedef Deferred<typename resolve_type<AN>::type...> resolved_type;
    template<class... CN>
    static auto check(int) -> tag_valid<typename resolved_type::seed_type>;
    template<class... CN>
    static tag_not_valid check(...);

    typedef decltype(check<AN...>(0)) tag_type;
    typedef typename tag_type::type type;
    static const bool value = tag_type::value;
};

template <class D>
struct resolve_type
{
    typedef D type;
};
template <template<class... TN> class Deferred, class... AN>
struct resolve_type<defer_type<Deferred, AN...>>
{
    typedef typename defer_type<Deferred, AN...>::type type;
};
template <template<class... TN> class Deferred, class... AN>
struct resolve_type<defer_value_type<Deferred, AN...>>
{
    typedef typename defer_value_type<Deferred, AN...>::type type;
};
template <template<class... TN> class Deferred, class... AN>
struct resolve_type<defer_seed_type<Deferred, AN...>>
{
    typedef typename defer_seed_type<Deferred, AN...>::type type;
};

struct plus
{
    template <class LHS, class RHS>
    auto operator()(LHS&& lhs, RHS&& rhs) const
        -> decltype(std::forward<LHS>(lhs) + std::forward<RHS>(rhs))
        { return std::forward<LHS>(lhs) + std::forward<RHS>(rhs); }
};

struct less
{
    template <class LHS, class RHS>
    auto operator()(LHS&& lhs, RHS&& rhs) const
        -> decltype(std::forward<LHS>(lhs) < std::forward<RHS>(rhs))
        { return std::forward<LHS>(lhs) < std::forward<RHS>(rhs); }
};

namespace detail {
template<class OStream, class Delimit>
struct print_function
{
    OStream& os;
    Delimit delimit;
    print_function(OStream& os, Delimit d) : os(os), delimit(std::move(d)) {}

    template<class... TN>
    void operator()(const TN&... tn) const {
        bool inserts[] = {(os << tn, true)...};
        delimit();
    }

    template<class... TN>
    void operator()(const std::tuple<TN...>& tpl) const {
        rxcpp::util::apply(tpl, *this);
    }
};

template<class OStream>
struct endline
{
    OStream& os;
    endline(OStream& os) : os(os) {}
    void operator()() const {
        os << std::endl;
    }
};

template<class OStream, class ValueType>
struct insert_value
{
    OStream& os;
    ValueType value;
    insert_value(OStream& os, ValueType v) : os(os), value(std::move(v)) {}
    void operator()() const {
        os << value;
    }
};

template<class OStream, class Function>
struct insert_function
{
    OStream& os;
    Function call;
    insert_function(OStream& os, Function f) : os(os), call(std::move(f)) {}
    void operator()() const {
        call(os);
    }
};

template<class OStream, class Delimit>
auto print_followed_with(OStream& os, Delimit d)
    ->      detail::print_function<OStream, Delimit> {
    return  detail::print_function<OStream, Delimit>(os, std::move(d));
}

}

template<class OStream>
auto endline(OStream& os)
    -> detail::endline<OStream> {
    return detail::endline<OStream>(os);
}

template<class OStream>
auto println(OStream& os)
    -> decltype(detail::print_followed_with(os, endline(os))) {
    return      detail::print_followed_with(os, endline(os));
}
template<class OStream, class Delimit>
auto print_followed_with(OStream& os, Delimit d)
    -> decltype(detail::print_followed_with(os, detail::insert_function<OStream, Delimit>(os, std::move(d)))) {
    return      detail::print_followed_with(os, detail::insert_function<OStream, Delimit>(os, std::move(d)));
}
template<class OStream, class DelimitValue>
auto print_followed_by(OStream& os, DelimitValue dv)
    -> decltype(detail::print_followed_with(os, detail::insert_value<OStream, DelimitValue>(os, std::move(dv)))) {
    return      detail::print_followed_with(os, detail::insert_value<OStream, DelimitValue>(os, std::move(dv)));
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
        reset(other);
        return *this;
    }
    maybe& operator=(const maybe& other) {
        if (!other.empty()) {
            reset(other.get());
        } else {
            reset();
        }
        return *this;
    }
};

}
using detail::maybe;

namespace detail {
    struct surely
    {
        template<class... T>
        auto operator()(T... t)
            -> decltype(std::make_tuple(t.get()...)) {
            return      std::make_tuple(t.get()...);
        }
        template<class... T>
        auto operator()(T... t) const
            -> decltype(std::make_tuple(t.get()...)) {
            return      std::make_tuple(t.get()...);
        }
    };
}

template<class... T>
inline auto surely(const std::tuple<T...>& tpl)
    -> decltype(apply(tpl, detail::surely())) {
    return      apply(tpl, detail::surely());
}

namespace detail {

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
