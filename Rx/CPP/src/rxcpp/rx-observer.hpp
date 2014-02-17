// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVER_HPP)
#define RXCPP_RX_OBSERVER_HPP

#include "rx-includes.hpp"

namespace rxcpp {


struct tag_observer {};
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
    typename this_type::subscription_type get_subscription() {
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
template<class T>
class is_observer
{
    template<class C>
    static typename C::observer_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<T>(0)), tag_observer>::value;
};

namespace detail {
struct OnErrorEmpty
{
    void operator()(std::exception_ptr) const {}
};
struct OnCompletedEmpty
{
    void operator()() const {}
};
}

template<class T>
class dynamic_observer : public observer_base<T>
{
public:
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
    dynamic_observer(composite_subscription cs, on_next_t n = nullptr, on_error_t e = nullptr, on_completed_t c = nullptr)
        : base_type(std::move(cs))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    dynamic_observer(on_next_t n, on_error_t e = nullptr, on_completed_t c = nullptr)
        : base_type(composite_subscription())
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    dynamic_observer(const dynamic_observer& o)
        : base_type(o)
        , onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    dynamic_observer(dynamic_observer&& o)
        : base_type(std::move(o))
        , onnext(std::move(o.onnext))
        , onerror(std::move(o.onerror))
        , oncompleted(std::move(o.oncompleted))
    {
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
    typedef OnNext on_next_t;
    typedef OnError on_error_t;
    typedef OnCompleted on_completed_t;

private:
    on_next_t onnext;
    on_error_t onerror;
    on_completed_t oncompleted;

public:
    static_observer()
    {
    }
    static_observer(composite_subscription cs, on_next_t n = nullptr, on_error_t e = nullptr, on_completed_t c = nullptr)
        : observer_base<T>(std::move(cs))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    static_observer(on_next_t n, on_error_t e = nullptr, on_completed_t c = nullptr)
        : observer_base<T>(composite_subscription())
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    static_observer(const static_observer& o)
        : observer_base<T>(o)
        , onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    static_observer(static_observer&& o)
        : observer_base<T>(std::move(o))
        , onnext(std::move(o.onnext))
        , onerror(std::move(o.onerror))
        , oncompleted(std::move(o.oncompleted))
    {
    }
    static_observer& operator=(static_observer o) {
        swap(o);
        return *this;
    }
    void swap(static_observer& o) {
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
    typedef observer this_type;
    typedef typename std::conditional<is_observer<I>::value, I, dynamic_observer<T>>::type inner_t;

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
            protect.that = nullptr;
        }
        unsubscribe();
    }
    void on_completed() const {
        if (is_subscribed()) {
            detacher protect(this);
            inner.on_completed();
            protect.that = nullptr;
        }
        unsubscribe();
    }
    typename this_type::subscription_type get_subscription() {
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
    typename this_type::subscription_type get_subscription() {
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

struct tag_require_observer {};

struct tag_require_function {};

template<class I>
auto make_observer(I i, tag_observer&&)
    ->      observer<typename I::value_type, I> {
    return  observer<typename I::value_type, I>(std::move(i));
}

struct tag_function {};
template<class T, class OnNext>
auto make_observer(OnNext n, tag_function&&)
    ->      observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>> {
    return  observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>>(
                        static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>(std::move(n), detail::OnErrorEmpty(), detail::OnCompletedEmpty()));
}

template<class T, class OnNext, class OnError>
auto make_observer(OnNext n, OnError e, tag_function&&)
    ->      observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>> {
    return  observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>>(
                        static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>(std::move(n), std::move(e), detail::OnCompletedEmpty()));
}

template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer(OnNext n, OnError e, OnCompleted c, tag_function&&)
    ->      observer<T, static_observer<T, OnNext, OnError, OnCompleted>> {
    return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                        static_observer<T, OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c)));
}

template<class T, class OnNext>
auto make_observer(composite_subscription cs, OnNext n, tag_subscription&&)
    ->      observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>> {
    return  observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>>(
                        static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>(std::move(cs), std::move(n), detail::OnErrorEmpty(), detail::OnCompletedEmpty()));
}

template<class T, class OnNext, class OnError>
auto make_observer(composite_subscription cs, OnNext n, OnError e, tag_subscription&&)
    ->      observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>> {
    return  observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>>(
                        static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>(std::move(cs), std::move(n), std::move(e), detail::OnCompletedEmpty()));
}

}

template<class Arg>
auto make_observer(Arg a)
    -> decltype(detail::make_observer(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, detail::tag_require_observer>::type())) {
    return      detail::make_observer(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, detail::tag_require_observer>::type());
}

template<class T, class Arg>
auto make_observer(Arg a)
    -> decltype(detail::make_observer<T>(std::move(a), typename std::conditional<is_observer<Arg>::value, detail::tag_require_function, detail::tag_function>::type())) {
    return      detail::make_observer<T>(std::move(a), typename std::conditional<is_observer<Arg>::value, detail::tag_require_function, detail::tag_function>::type());
}

template<class T, class Arg1, class Arg2>
auto make_observer(Arg1 a1, Arg2 a2)
    -> decltype(detail::make_observer<T>(std::move(a1), std::move(a2), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type())) {
    return      detail::make_observer<T>(std::move(a1), std::move(a2), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type());
}

template<class T, class Arg1, class Arg2, class Arg3>
auto make_observer(Arg1 a1, Arg2 a2, Arg3 a3)
    -> decltype(detail::make_observer<T>(std::move(a1), std::move(a2), std::move(a3), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type())) {
    return      detail::make_observer<T>(std::move(a1), std::move(a2), std::move(a3), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type());
}

template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer(composite_subscription cs, OnNext n, OnError e, OnCompleted c)
    ->      observer<T, static_observer<T, OnNext, OnError, OnCompleted>> {
    return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                        static_observer<T, OnNext, OnError, OnCompleted>(std::move(cs), std::move(n), std::move(e), std::move(c)));
}

template<class T>
auto make_observer_dynamic(typename dynamic_observer<T>::on_next_t n, typename dynamic_observer<T>::on_error_t e = nullptr, typename dynamic_observer<T>::on_completed_t c = nullptr)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(n), std::move(e), std::move(c)));
}

template<class T>
auto make_observer_dynamic(composite_subscription cs, typename dynamic_observer<T>::on_next_t n, typename dynamic_observer<T>::on_error_t e = nullptr, typename dynamic_observer<T>::on_completed_t c = nullptr)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(cs), std::move(n), std::move(e), std::move(c)));
}

}

#endif
