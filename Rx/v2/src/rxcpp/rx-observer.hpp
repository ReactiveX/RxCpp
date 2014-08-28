// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVER_HPP)
#define RXCPP_RX_OBSERVER_HPP

#include "rx-includes.hpp"

namespace rxcpp {


template<class T>
struct observer_base
{
    typedef T value_type;
    typedef tag_observer observer_tag;
};

namespace detail {
template<class T>
struct OnNextEmpty
{
    void operator()(const T&) const {}
};
struct OnErrorEmpty
{
    void operator()(std::exception_ptr) const {
        // error implicitly ignored, abort
        abort();
    }
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

template<class T, class OnNext, class OnError = detail::OnErrorEmpty, class OnCompleted = detail::OnCompletedEmpty>
class static_observer
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

    explicit static_observer(on_next_t n, on_error_t e = on_error_t(), on_completed_t c = on_completed_t())
        : onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    static_observer(const this_type& o)
        : onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    static_observer(this_type&& o)
        : onnext(std::move(o.onnext))
        , onerror(std::move(o.onerror))
        , oncompleted(std::move(o.oncompleted))
    {
    }
    this_type& operator=(this_type o) {
        onnext = std::move(o.onnext);
        onerror = std::move(o.onerror);
        oncompleted = std::move(o.oncompleted);
        return *this;
    }

    // use V so that std::move can be used safely
    template<class V>
    void on_next(V v) const {
        onnext(std::move(v));
    }
    void on_error(std::exception_ptr e) const {
        onerror(e);
    }
    void on_completed() const {
        oncompleted();
    }
};

template<class T>
class dynamic_observer
{
public:
    typedef tag_dynamic_observer dynamic_observer_tag;

private:
    typedef dynamic_observer<T> this_type;
    typedef observer_base<T> base_type;

    struct virtual_observer : public std::enable_shared_from_this<virtual_observer>
    {
        virtual void on_next(T) const =0;
        virtual void on_error(std::exception_ptr e) const =0;
        virtual void on_completed() const =0;
    };

    template<class Observer>
    struct specific_observer : public virtual_observer
    {
        explicit specific_observer(Observer o)
            : destination(std::move(o))
        {
        }

        Observer destination;
        virtual void on_next(T t) const {
            destination.on_next(std::move(t));
        }
        virtual void on_error(std::exception_ptr e) const {
            destination.on_error(e);
        }
        virtual void on_completed() const {
            destination.on_completed();
        }
    };

    std::shared_ptr<virtual_observer> destination;

    template<class Observer>
    static auto make_destination(Observer o)
        -> std::shared_ptr<virtual_observer> {
        return std::make_shared<specific_observer<Observer>>(std::move(o));
    }

public:
    dynamic_observer()
    {
    }
    dynamic_observer(const this_type& o)
        : destination(o.destination)
    {
    }
    dynamic_observer(this_type&& o)
        : destination(std::move(o.destination))
    {
    }

    template<class Observer>
    explicit dynamic_observer(Observer o)
        : destination(make_destination(std::move(o)))
    {
    }

    this_type& operator=(this_type o) {
        destination = std::move(o.destination);
        return *this;
    }

    // perfect forwarding delays the copy of the value.
    template<class V>
    void on_next(V&& v) const {
        if (destination) {
            destination->on_next(std::forward<V>(v));
        }
    }
    void on_error(std::exception_ptr e) const {
        if (destination) {
            destination->on_error(e);
        }
    }
    void on_completed() const {
        if (destination) {
            destination->on_completed();
        }
    }
};


template<class T, class I>
class observer : public observer_base<T>
{
    typedef observer<T, I> this_type;
    typedef typename std::decay<I>::type inner_t;

    inner_t inner;

    observer();
public:
    ~observer()
    {
    }
    observer(const this_type& o)
        : inner(o.inner)
    {
    }
    observer(this_type&& o)
        : inner(std::move(o.inner))
    {
    }
    explicit observer(inner_t inner)
        : inner(std::move(inner))
    {
    }
    this_type& operator=(this_type o) {
        inner = std::move(o.inner);
        return *this;
    }
    template<class V>
    void on_next(V&& v) const {
        inner.on_next(std::forward<V>(v));
    }
    void on_error(std::exception_ptr e) const {
        inner.on_error(e);
    }
    void on_completed() const {
        inner.on_completed();
    }
    observer<T> as_dynamic() const {
        return observer<T>(dynamic_observer<T>(inner));
    }
};
template<class T>
class observer<T, void> : public observer_base<T>
{
    typedef observer this_type;
public:
    observer()
    {
    }
    template<class V>
    void on_next(V&&) const {
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

template<class T, class U, class I>
auto make_observer(observer<U, I> o)
    ->      observer<T, I> {
    return  observer<T, I>(std::move(o));
}
template<class T, class Observer>
auto make_observer(Observer ob)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, Observer>::value &&
        !is_observer<Observer>::value,
            observer<T, Observer>>::type {
    return  observer<T, Observer>(std::move(ob));
}
template<class T, class OnNext>
auto make_observer(OnNext on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            observer<T, static_observer<T, OnNext>>>::type {
    return  observer<T, static_observer<T, OnNext>>(
                        static_observer<T, OnNext>(std::move(on)));
}
template<class T, class OnNext, class OnError>
auto make_observer(OnNext on, OnError oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            observer<T, static_observer<T, OnNext, OnError>>>::type {
    return  observer<T, static_observer<T, OnNext, OnError>>(
                        static_observer<T, OnNext, OnError>(std::move(on), std::move(oe)));
}
template<class T, class OnNext, class OnCompleted>
auto make_observer(OnNext on, OnCompleted oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>::type {
    return  observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>(
                        static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>(std::move(on), detail::OnErrorEmpty(), std::move(oc)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer(OnNext on, OnError oe, OnCompleted oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>::type {
    return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                        static_observer<T, OnNext, OnError, OnCompleted>(std::move(on), std::move(oe), std::move(oc)));
}


template<class T, class Observer>
auto make_observer_dynamic(Observer o)
    -> typename std::enable_if<
        is_observer<Observer>::value,
            observer<T>>::type {
    return  observer<T>(dynamic_observer<T>(std::move(o)));
}
template<class T, class OnNext>
auto make_observer_dynamic(OnNext&& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(make_observer<T>(std::forward<OnNext>(on))));
}
template<class T, class OnNext, class OnError>
auto make_observer_dynamic(OnNext&& on, OnError&& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(make_observer<T>(std::forward<OnNext>(on), std::forward<OnError>(oe))));
}
template<class T, class OnNext, class OnCompleted>
auto make_observer_dynamic(OnNext&& on, OnCompleted&& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(make_observer<T>(std::forward<OnNext>(on), std::forward<OnCompleted>(oc))));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer_dynamic(OnNext&& on, OnError&& oe, OnCompleted&& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(make_observer<T>(std::forward<OnNext>(on), std::forward<OnError>(oe), std::forward<OnCompleted>(oc))));
}

namespace detail {

template<class F>
struct maybe_from_result
{
    typedef decltype((*(F*)nullptr)()) decl_result_type;
    typedef typename std::decay<decl_result_type>::type result_type;
    typedef rxu::maybe<result_type> type;
};

}

template<class F, class OnError>
auto on_exception(const F& f, const OnError& c)
    ->  typename std::enable_if<detail::is_on_error<OnError>::value, typename detail::maybe_from_result<F>::type>::type {
    typename detail::maybe_from_result<F>::type r;
    try {
        r.reset(f());
    } catch (...) {
        c(std::current_exception());
    }
    return r;
}

template<class F, class Subscriber>
auto on_exception(const F& f, const Subscriber& s)
    ->  typename std::enable_if<is_subscriber<Subscriber>::value, typename detail::maybe_from_result<F>::type>::type {
    typename detail::maybe_from_result<F>::type r;
    try {
        r.reset(f());
    } catch (...) {
        s.on_error(std::current_exception());
    }
    return r;
}

}

#endif
