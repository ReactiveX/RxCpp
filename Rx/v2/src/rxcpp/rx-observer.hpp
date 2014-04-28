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
class dynamic_observer
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
    dynamic_observer(OnNext&& n, OnError&& e, OnCompleted&& c)
        : onnext(std::forward<OnNext>(n))
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
        swap(onnext, o.onnext);
        swap(onerror, o.onerror);
        swap(oncompleted, o.oncompleted);
    }

    template<class V>
    void on_next(V&& v) const {
        if (onnext) {
            onnext(std::forward<V>(v));
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
    static_observer& operator=(this_type o) {
        swap(o);
        return *this;
    }
    void swap(this_type& o) {
        using std::swap;
        swap(onnext, o.onnext);
        swap(onerror, o.onerror);
        swap(oncompleted, o.oncompleted);
    }

    template<class V>
    void on_next(V&& v) const {
        onnext(std::forward<V>(v));
    }
    void on_error(std::exception_ptr e) const {
        onerror(e);
    }
    void on_completed() const {
        oncompleted();
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
    explicit observer(inner_t inner)
        : inner(std::move(inner))
    {
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

template<class T, class I>
auto make_observer(const observer<T, I>& o)
    ->      observer<T, I> {
    return  observer<T, I>(
                        I(o));
}
template<class T, class I>
auto make_observer(observer<T, I>&& o)
    ->      observer<T, I> {
    return  observer<T, I>(
                        I(std::move(o)));
}
template<class T, class OnNext>
auto make_observer(const OnNext& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            observer<T, static_observer<T, OnNext>>>::type {
    return  observer<T, static_observer<T, OnNext>>(
                        static_observer<T, OnNext>(on));
}
template<class T, class OnNext, class OnError>
auto make_observer(const OnNext& on, const OnError& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            observer<T, static_observer<T, OnNext, OnError>>>::type {
    return  observer<T, static_observer<T, OnNext, OnError>>(
                        static_observer<T, OnNext, OnError>(on, oe));
}
template<class T, class OnNext, class OnCompleted>
auto make_observer(const OnNext& on, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>::type {
    return  observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>(
                        static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>(on, detail::OnErrorEmpty(), oc));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer(const OnNext& on, const OnError& oe, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>::type {
    return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                        static_observer<T, OnNext, OnError, OnCompleted>(on, oe, oc));
}

template<class T>
auto make_observer_dynamic(const observer<T>& o)
    ->      observer<T> {
    return  observer<T>(
                dynamic_observer<T>(o));
}
template<class T>
auto make_observer_dynamic(observer<T>&& o)
    ->      observer<T> {
    return  observer<T>(
                dynamic_observer<T>(std::move(o)));
}
template<class T, class OnNext>
auto make_observer_dynamic(OnNext&& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(std::forward<OnNext>(on), nullptr, nullptr));
}
template<class T, class OnNext, class OnError>
auto make_observer_dynamic(OnNext&& on, OnError&& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(std::forward<OnNext>(on), std::forward<OnError>(oe), nullptr));
}
template<class T, class OnNext, class OnCompleted>
auto make_observer_dynamic(OnNext&& on, OnCompleted&& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(std::forward<OnNext>(on), nullptr, std::forward<OnCompleted>(oc)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer_dynamic(OnNext&& on, OnError&& oe, OnCompleted&& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, dynamic_observer<T>>>::type {
    return  observer<T, dynamic_observer<T>>(
                        dynamic_observer<T>(std::forward<OnNext>(on), std::forward<OnError>(oe), std::forward<OnCompleted>(oc)));
}

}

#endif
