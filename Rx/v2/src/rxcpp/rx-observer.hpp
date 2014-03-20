// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVER_HPP)
#define RXCPP_RX_OBSERVER_HPP

#include "rx-includes.hpp"

namespace rxcpp {


template<class T>
struct observer_root : public subscription_base
{
    typedef T value_type;
    typedef composite_subscription subscription_type;
    typedef typename subscription_type::weak_subscription weak_subscription;
    typedef tag_observer observer_tag;
};
template<class T>
struct observer_base : public observer_root<T>
{
private:
    typedef observer_base this_type;

    mutable typename this_type::subscription_type s;

public:
    observer_base()
    {
    }
    observer_base(typename this_type::subscription_type s)
        : s(std::move(s))
    {
    }
    observer_base(const observer_base& o)
        : s(o.s)
    {
    }
    observer_base(observer_base&& o)
        : s(std::move(o.s))
    {
    }
    observer_base& operator=(observer_base o) {
        swap(o);
        return *this;
    }
    void swap(observer_base& o) {
        using std::swap;
        swap(s, o.s);
    }
    const typename this_type::subscription_type& get_subscription() const {
        return s;
    }
    bool is_subscribed() const {
        return s.is_subscribed();
    }
    typename this_type::weak_subscription add(dynamic_subscription ds) const {
        return s.add(std::move(ds));
    }
    void remove(typename this_type::weak_subscription ws) const {
        s.remove(ws);
    }
    void unsubscribe() const {
        s.unsubscribe();
    }
};

namespace detail {
template<class T>
struct OnNextEmpty
{
    void operator()(const T&) const {}
};
struct OnErrorEmpty
{
    void operator()(std::exception_ptr) const {}
};
struct OnCompletedEmpty
{
    void operator()() const {}
};

template<class T, class F>
struct is_on_next_of
{
    struct not_void {};
    template<class CT, class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(CT*)nullptr));
    template<class CT, class CF>
    static not_void check(...);

    typedef decltype(check<T, typename std::decay<F>::type>(0)) detail_result;
    static const bool value = std::is_same<detail_result, void>::value;
};

template<class F>
struct is_on_error
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(std::exception_ptr*)nullptr));
    template<class CF>
    static not_void check(...);

    static const bool value = std::is_same<decltype(check<typename std::decay<F>::type>(0)), void>::value;
};

template<class F>
struct is_on_completed
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)());
    template<class CF>
    static not_void check(...);

    static const bool value = std::is_same<decltype(check<typename std::decay<F>::type>(0)), void>::value;
};

}

template<class T>
class dynamic_observer : public observer_base<T>
{
public:
    typedef tag_dynamic_observer dynamic_observer_tag;

    typedef std::function<void(T)> on_next_t;
    typedef std::function<void(std::exception_ptr)> on_error_t;
    typedef std::function<void()> on_completed_t;
private:
    typedef observer_base<T> base_type;

    on_next_t onnext;
    on_error_t onerror;
    on_completed_t oncompleted;

public:
    dynamic_observer()
    {
    }

    template<class OnNext, class OnError, class OnCompleted>
    dynamic_observer(composite_subscription cs, OnNext&& n, OnError&& e, OnCompleted&& c)
        : base_type(std::move(cs))
        , onnext(std::forward<OnNext>(n))
        , onerror(std::forward<OnError>(e))
        , oncompleted(std::forward<OnCompleted>(c))
    {
        static_assert(detail::is_on_next_of<T, OnNext>::value || std::is_same<OnNext, std::nullptr_t>::value,
                "Function supplied for on_next must be a function with the signature void(T);");
        static_assert(detail::is_on_error<OnError>::value || std::is_same<OnError, std::nullptr_t>::value,
                "Function supplied for on_error must be a function with the signature void(std::exception_ptr);");
        static_assert(detail::is_on_completed<OnCompleted>::value || std::is_same<OnCompleted, std::nullptr_t>::value,
                "Function supplied for on_completed must be a function with the signature void();");
    }

    dynamic_observer& operator=(dynamic_observer o) {
        swap(o);
        return *this;
    }
    void swap(dynamic_observer& o) {
        using std::swap;
        observer_base<T>::swap(o);
        swap(onnext, o.onnext);
        swap(onerror, o.onerror);
        swap(oncompleted, o.oncompleted);
    }

    void on_next(T t) const {
        if (onnext) {
            onnext(std::move(t));
        }
    }
    void on_error(std::exception_ptr e) const {
        if (onerror) {
            onerror(e);
        }
    }
    void on_completed() const {
        if (oncompleted) {
            oncompleted();
        }
    }
};

template<class T, class OnNext, class OnError, class OnCompleted>
class static_observer : public observer_base<T>
{
public:
    typedef static_observer<T, OnNext, OnError, OnCompleted> this_type;
    typedef typename std::decay<OnNext>::type on_next_t;
    typedef typename std::decay<OnError>::type on_error_t;
    typedef typename std::decay<OnCompleted>::type on_completed_t;

private:
    on_next_t onnext;
    on_error_t onerror;
    on_completed_t oncompleted;

public:
    static_assert(detail::is_on_next_of<T, on_next_t>::value,     "Function supplied for on_next must be a function with the signature void(T);");
    static_assert(detail::is_on_error<on_error_t>::value,         "Function supplied for on_error must be a function with the signature void(std::exception_ptr);");
    static_assert(detail::is_on_completed<on_completed_t>::value, "Function supplied for on_completed must be a function with the signature void();");

    explicit static_observer(composite_subscription cs, on_next_t n, on_error_t e, on_completed_t c)
        : observer_base<T>(std::move(cs))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    static_observer(const this_type& o)
        : observer_base<T>(o)
        , onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    static_observer(this_type&& o)
        : observer_base<T>(std::move(o))
        , onnext(std::move(o.onnext))
        , onerror(std::move(o.onerror))
        , oncompleted(std::move(o.oncompleted))
    {
    }
    static_observer& operator=(this_type o) {
        swap(o);
        return *this;
    }
    void swap(this_type& o) {
        using std::swap;
        observer_base<T>::swap(o);
        swap(onnext, o.onnext);
        swap(onerror, o.onerror);
        swap(oncompleted, o.oncompleted);
    }

    void on_next(T t) const {
        onnext(std::move(t));
    }
    void on_error(std::exception_ptr e) const {
        onerror(e);
    }
    void on_completed() const {
        oncompleted();
    }
};

template<class T, class I>
class observer : public observer_root<T>
{
    typedef observer<T, I> this_type;
    typedef typename std::conditional<is_observer<I>::value, typename std::decay<I>::type, dynamic_observer<T>>::type inner_t;

    struct detacher
    {
        ~detacher()
        {
            if (that) {
                that->unsubscribe();
            }
        }
        detacher(const observer<T,I>* that)
            : that(that)
        {
        }
        const observer<T,I>* that;
    };

    inner_t inner;
public:
    ~observer()
    {
    }
    observer()
    {
    }
    observer(inner_t inner)
        : inner(std::move(inner))
    {
    }
    void on_next(T t) const {
        if (is_subscribed()) {
            detacher protect(this);
            inner.on_next(std::move(t));
            protect.that = nullptr;
        }
    }
    void on_error(std::exception_ptr e) const {
        if (is_subscribed()) {
            detacher protect(this);
            inner.on_error(e);
        }
    }
    void on_completed() const {
        if (is_subscribed()) {
            detacher protect(this);
            inner.on_completed();
        }
    }
    const typename this_type::subscription_type& get_subscription() const {
        return inner.get_subscription();
    }
    bool is_subscribed() const {
        return inner.is_subscribed();
    }
    typename this_type::weak_subscription add(dynamic_subscription ds) const {
        return inner.add(std::move(ds));
    }
    void remove(typename this_type::weak_subscription ws) const {
        inner.remove(ws);
    }
    void unsubscribe() const {
        inner.unsubscribe();
    }
};
template<class T>
class observer<T, void> : public observer_root<T>
{
    typedef observer this_type;
public:
    observer()
    {
    }
    void on_next(T&&) const {
    }
    void on_error(std::exception_ptr) const {
    }
    void on_completed() const {
    }
    const typename this_type::subscription_type& get_subscription() const {
        static typename this_type::subscription_type result;
        result.unsubscribe();
        return result;
    }
    bool is_subscribed() const {
        return false;
    }
    typename this_type::weak_subscription add(dynamic_subscription ds) const {
        ds.unsubscribe();
        return typename this_type::weak_subscription();
    }
    void remove(typename this_type::weak_subscription) const {
    }
    void unsubscribe() const {
    }
};

template<class T>
auto make_observer()
    ->     observer<T, void> {
    return observer<T, void>();
}

namespace detail {

template<class T, class ResolvedArgSet>
auto make_observer_resolved(ResolvedArgSet&& rs)
    ->      observer<T,                 static_observer<T, typename std::decay<decltype(std::get<1>(std::forward<ResolvedArgSet>(rs)))>::type::result_type, typename std::decay<decltype(std::get<2>(std::forward<ResolvedArgSet>(rs)))>::type::result_type, typename std::decay<decltype(std::get<3>(std::forward<ResolvedArgSet>(rs)))>::type::result_type>> {
    return  make_observer_resolved<T,   static_observer<T, typename std::decay<decltype(std::get<1>(std::forward<ResolvedArgSet>(rs)))>::type::result_type, typename std::decay<decltype(std::get<2>(std::forward<ResolvedArgSet>(rs)))>::type::result_type, typename std::decay<decltype(std::get<3>(std::forward<ResolvedArgSet>(rs)))>::type::result_type>>(std::forward<ResolvedArgSet>(rs));
}
template<class T, class ResolvedArgSet>
auto make_observer_dynamic_resolved(ResolvedArgSet&& rs)
    ->      observer<T, dynamic_observer<T>> {
    return  make_observer_resolved<T, dynamic_observer<T>>(std::forward<ResolvedArgSet>(rs));
}

template<class T, class I, class ResolvedArgSet>
auto make_observer_resolved(ResolvedArgSet&& rs)
    ->      observer<T, I> {
    typedef I inner_type;
    return  observer<T, inner_type>(inner_type(
        std::get<0>(std::forward<ResolvedArgSet>(rs)).value,
        std::move(std::get<1>(std::forward<ResolvedArgSet>(rs)).value),
        std::move(std::get<2>(std::forward<ResolvedArgSet>(rs)).value),
        std::move(std::get<3>(std::forward<ResolvedArgSet>(rs)).value)));

    typedef typename std::decay<decltype(std::get<1>(std::forward<ResolvedArgSet>(rs)))>::type rn_t;
    typedef typename std::decay<decltype(std::get<2>(std::forward<ResolvedArgSet>(rs)))>::type re_t;
    typedef typename std::decay<decltype(std::get<3>(std::forward<ResolvedArgSet>(rs)))>::type rc_t;

    static_assert(rn_t::is_arg, "onnext is a required parameter");
    static_assert(!(rn_t::is_arg && re_t::is_arg) || rn_t::n + 1 == re_t::n, "onnext, onerror parameters must be together and in order");
    static_assert(!(re_t::is_arg && rc_t::is_arg) || re_t::n + 1 == rc_t::n, "onerror, oncompleted parameters must be together and in order");
    static_assert(!(rn_t::is_arg && rc_t::is_arg  && !re_t::is_arg) || rn_t::n + 1 == rc_t::n, "onnext, oncompleted parameters must be together and in order");
}

struct tag_subscription_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_subscriber<LHS>::value && !is_observer<LHS>::value && is_subscription<LHS>::value;
    };
    typedef composite_subscription default_type;
};

template<class T>
struct tag_onnext_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = is_on_next_of<T, LHS>::value;
    };
    typedef OnNextEmpty<T> default_type;
};

struct tag_onerror_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = is_on_error<LHS>::value;
    };
    typedef OnErrorEmpty default_type;
};

struct tag_oncompleted_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = is_on_completed<LHS>::value;
    };
    typedef OnCompletedEmpty default_type;
};

// types to disambiguate
// on_next and optional on_error, on_completed +
// optional subscription
//

template<class T>
struct tag_observer_set
    : public    rxu::detail::tag_set<tag_subscription_resolution,
                rxu::detail::tag_set<tag_onnext_resolution<T>,
                rxu::detail::tag_set<tag_onerror_resolution,
                rxu::detail::tag_set<tag_oncompleted_resolution>>>>
{
};


}

#if RXCPP_USE_VARIADIC_TEMPLATES
template<class T, class Arg0, class... ArgN>
auto make_observer(Arg0&& a0, ArgN&&... an)
    -> decltype(detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...))) {
    return      detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
}
#else
template<class T, class Arg0>
auto make_observer(Arg0&& a0)
    -> decltype(detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0)))) {
    return      detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0)));
}
template<class T, class Arg0, class Arg1>
auto make_observer(Arg0&& a0, Arg1&& a1)
    -> decltype(detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
    return      detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
}
template<class T, class Arg0, class Arg1, class Arg2>
auto make_observer(Arg0&& a0, Arg1&& a1, Arg2&& a2)
    -> decltype(detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
    return      detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3>
auto make_observer(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    -> decltype(detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
    return      detail::make_observer_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
}
#endif

#if RXCPP_USE_VARIADIC_TEMPLATES
template<class T, class Arg0, class... ArgN>
auto make_observer_dynamic(Arg0&& a0, ArgN&&... an)
    -> decltype(detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...))) {
    return      detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
}
#else
template<class T, class Arg0>
auto make_observer_dynamic(Arg0&& a0)
    -> decltype(detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0)))) {
    return      detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0)));
}
template<class T, class Arg0, class Arg1>
auto make_observer_dynamic(Arg0&& a0, Arg1&& a1)
    -> decltype(detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
    return      detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
}
template<class T, class Arg0, class Arg1, class Arg2>
auto make_observer_dynamic(Arg0&& a0, Arg1&& a1, Arg2&& a2)
    -> decltype(detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
    return      detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3>
auto make_observer_dynamic(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    -> decltype(detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
    return      detail::make_observer_dynamic_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_observer_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
}
#endif

}

#endif
