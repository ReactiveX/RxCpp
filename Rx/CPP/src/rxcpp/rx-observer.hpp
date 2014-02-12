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
template<class T, class SelectVoid>
struct observer_base : public observer_root<T>
{
private:
    typedef observer_base this_type;

    mutable typename this_type::subscription_type s;

    observer_base();
public:
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
struct observer_base<T, void> : public observer_root<T>
{
private:
    typedef observer_base this_type;

public:
    void swap(observer_base&) {
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
class dynamic_observer : public observer_base<T, int>
{
public:
    typedef std::function<void(T)> on_next_t;
    typedef std::function<void(std::exception_ptr)> on_error_t;
    typedef std::function<void()> on_completed_t;

private:
    typedef observer_base<T, int> base_type;

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
        observer_base<T, int>::swap(o);
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
class static_observer : public observer_base<T, int>
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
        : observer_base<T, int>(std::move(cs))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    static_observer(on_next_t n, on_error_t e = nullptr, on_completed_t c = nullptr)
        : observer_base<T, int>(composite_subscription())
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    static_observer(const static_observer& o)
        : observer_base<T, int>(o)
        , onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    static_observer(static_observer&& o)
        : observer_base<T, int>(std::move(o))
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
        observer_base<T, int>::swap(o);
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
    typedef typename std::conditional<is_observer<I>::value, I, dynamic_observer<T>>::type

    inner_t;
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
            RXCPP_UNWIND(unsubscriber, [this](){
                this->unsubscribe();
            });
            inner.on_next(std::move(t));
            unsubscriber.dismiss();
        }
    }
    void on_error(std::exception_ptr e) const {
        if (is_subscribed()) {
            RXCPP_UNWIND_AUTO([this](){
                this->unsubscribe();
            });
            inner.on_error(e);
        }
        unsubscribe();
    }
    void on_completed() const {
        if (is_subscribed()) {
            RXCPP_UNWIND_AUTO([this](){
                this->unsubscribe();
            });
            inner.on_completed();
        }
        unsubscribe();
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
class observer<T, void> : public observer_base<T, void>
{
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
};
template<class T>
auto make_observer()
    ->     observer<T, void> {
    return observer<T, void>();
}
template<class I>
auto make_observer(I i)
    -> typename std::enable_if<is_observer<I>::value, observer<typename I::value_type, I>>::type {
    return                                            observer<typename I::value_type, I>(std::move(i));
}
template<class T>
auto make_observer(typename dynamic_observer<T>::on_next_t n, typename dynamic_observer<T>::on_error_t e = nullptr, typename dynamic_observer<T>::on_completed_t c = nullptr)
    ->      observer<T, dynamic_observer<T>> {
    return  observer<T, dynamic_observer<T>>(dynamic_observer<T>(std::move(n), std::move(e), std::move(c)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer(OnNext n, OnError e, OnCompleted c)
    ->      observer<T, static_observer<T, OnNext, OnError, OnCompleted>> {
    return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                        static_observer<T, OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c)));
}
template<class T, class OnNext, class OnError>
auto make_observer(OnNext n, OnError e)
    ->      observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>> {
    return  observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>>(
                        static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>(std::move(n), std::move(e), detail::OnCompletedEmpty()));
}
template<class T, class OnNext>
auto make_observer(OnNext n)
    -> typename std::enable_if<!is_observer<OnNext>::value,
            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>>>::type {
    return  observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>>(
                        static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>(std::move(n), detail::OnErrorEmpty(), detail::OnCompletedEmpty()));
}

}

#endif
