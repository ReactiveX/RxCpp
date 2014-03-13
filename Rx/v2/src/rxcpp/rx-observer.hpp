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

    typedef decltype(check<T, F>(0)) detail_result;
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

    static const bool value = std::is_same<decltype(check<F>(0)), void>::value;
};

template<class F>
struct is_on_completed
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)());
    template<class CF>
    static not_void check(...);

    static const bool value = std::is_same<decltype(check<F>(0)), void>::value;
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

    struct tag_this_type {};
    struct tag_function {};

    template<class Arg>
    struct resolve_tag
    {
        typedef typename std::conditional<std::is_same<typename std::decay<Arg>::type, dynamic_observer<T>>::value, tag_this_type,
                    typename std::conditional<is_subscription<Arg>::value, tag_subscription, tag_function>::type
                >::type type;
    };

    template<class Arg, class Tag = typename resolve_tag<Arg>::type>
    struct resolve_cs;
    template<class Arg>
    struct resolve_cs<Arg, tag_this_type>
    {
        auto operator()(const Arg& a)
            -> decltype(a.cs) {
            return a.cs;
        }
        auto operator()(Arg&& a)
            -> decltype(a.cs) {
            return std::move(a.cs);
        }
    };
    template<class Arg>
    struct resolve_cs<Arg, tag_subscription>
    {
        Arg operator()(Arg a) {
            return std::move(a);
        }
    };
    template<class Arg>
    struct resolve_cs<Arg, tag_function>
    {
        composite_subscription operator()(const Arg& a) {
            return composite_subscription();
        }
    };

    template<class Arg, class Tag = typename resolve_tag<Arg>::type>
    struct resolve_onnext;
    template<class Arg>
    struct resolve_onnext<Arg, tag_this_type>
    {
        auto operator()(const Arg& a)
            -> decltype(a.onnext) {
            return a.onnext;
        }
        auto operator()(Arg&& a)
            -> decltype(a.onnext) {
            return std::move(a.onnext);
        }
    };
    template<class Arg>
    struct resolve_onnext<Arg, tag_subscription>
    {
        template<class Arg1>
        on_next_t operator()(const Arg1& a1) {
            return on_next_t();
        }
        template<class Arg1, class Arg2>
        Arg2 operator()(const Arg1&, Arg2 a2) {
            static_assert(detail::is_on_next_of<T, Arg2>::value || std::is_same<Arg2, std::nullptr_t>::value,
                "Function supplied for on_next must be a function with the signature void(T);");
            return std::move(a2);
        }
    };
    template<class Arg>
    struct resolve_onnext<Arg, tag_function>
    {
        template<class Arg1>
        Arg1 operator()(Arg1 a1) {
            static_assert(detail::is_on_next_of<T, Arg1>::value || std::is_same<Arg1, std::nullptr_t>::value,
                "Function supplied for on_next must be a function with the signature void(T);");
            return std::move(a1);
        }
        template<class Arg1, class Arg2>
        Arg operator()(Arg1 a1, const Arg2&) {
            static_assert(detail::is_on_next_of<T, Arg1>::value || std::is_same<Arg1, std::nullptr_t>::value,
                "Function supplied for on_next must be a function with the signature void(T);");
            return std::move(a1);
        }
    };

    template<class Arg, class Tag = typename resolve_tag<Arg>::type>
    struct resolve_onerror;
    template<class Arg>
    struct resolve_onerror<Arg, tag_this_type>
    {
        auto operator()(const Arg& a)
            -> decltype(a.onerror) {
            return a.onerror;
        }
        auto operator()(Arg&& a)
            -> decltype(a.onerror) {
            return std::move(a.onerror);
        }
    };
    template<class Arg>
    struct resolve_onerror<Arg, tag_subscription>
    {
        template<class Arg1>
        on_error_t operator()(const Arg1& a1) {
            return on_error_t();
        }
        template<class Arg1, class Arg2>
        Arg2 operator()(const Arg1&, Arg2 a2) {
            static_assert(detail::is_on_error<Arg2>::value || std::is_same<Arg2, std::nullptr_t>::value,
                "Function supplied for on_error must be a function with the signature void(std::exception_ptr);");
            return std::move(a2);
        }
    };
    template<class Arg>
    struct resolve_onerror<Arg, tag_function>
    {
        template<class Arg1>
        on_error_t operator()(const Arg1& a1) {
            return on_error_t();
        }
        template<class Arg1, class Arg2>
        Arg1 operator()(Arg1 a1, const Arg2&) {
            static_assert(detail::is_on_error<Arg1>::value || std::is_same<Arg1, std::nullptr_t>::value,
                "Function supplied for on_error must be a function with the signature void(std::exception_ptr);");
            return std::move(a1);
        }
    };

    template<class Arg, class Tag = typename resolve_tag<Arg>::type>
    struct resolve_oncompleted;
    template<class Arg>
    struct resolve_oncompleted<Arg, tag_this_type>
    {
        auto operator()(const Arg& a)
            -> decltype(a.oncompleted) {
            return a.oncompleted;
        }
        auto operator()(Arg&& a)
            -> decltype(a.oncompleted) {
            return std::move(a.oncompleted);
        }
    };
    template<class Arg>
    struct resolve_oncompleted<Arg, tag_subscription>
    {
        template<class Arg1>
        on_completed_t operator()(const Arg1& a1) {
            return on_completed_t();
        }
        template<class Arg1, class Arg2>
        Arg2 operator()(const Arg1&, Arg2 a2) {
            static_assert(detail::is_on_completed<Arg2>::value || std::is_same<Arg2, std::nullptr_t>::value,
                "Function supplied for on_completed must be a function with the signature void();");
            return std::move(a2);
        }
    };
    template<class Arg>
    struct resolve_oncompleted<Arg, tag_function>
    {
        template<class Arg1>
        on_completed_t operator()(const Arg1& a1) {
            return on_completed_t();
        }
        template<class Arg1, class Arg2>
        Arg1 operator()(Arg1 a1, const Arg2&) {
            static_assert(detail::is_on_completed<Arg1>::value || std::is_same<Arg1, std::nullptr_t>::value,
                "Function supplied for on_completed must be a function with the signature void();");
            return std::move(a1);
        }
    };

public:
    dynamic_observer()
    {
    }

    template<class Arg>
    explicit dynamic_observer(Arg a)
        : base_type(resolve_cs<Arg>()(a))
        , onnext(resolve_onnext<Arg>()(a))
        , onerror(resolve_onerror<Arg>()(a))
        , oncompleted(resolve_oncompleted<Arg>()(a))
    {
    }

    template<class Arg1, class Arg2>
    dynamic_observer(Arg1 a1, Arg2 a2)
        : base_type(resolve_cs<Arg1>()(a1))
        , onnext(resolve_onnext<Arg1>()(a1, a2))
        , onerror(resolve_onerror<Arg1>()(a2, on_error_t()))
        , oncompleted(resolve_oncompleted<Arg1>()(on_completed_t(), on_completed_t()))
    {
    }

    template<class Arg1, class Arg2, class Arg3>
    dynamic_observer(Arg1 a1, Arg2 a2, Arg3 a3)
        : base_type(resolve_cs<Arg1>()(a1))
        , onnext(resolve_onnext<Arg1>()(a1, a2))
        , onerror(resolve_onerror<Arg1>()(a2, a3))
        , oncompleted(resolve_oncompleted<Arg1>()(a3, on_completed_t()))
    {
    }

    template<class OnNext, class OnError, class OnCompleted>
    dynamic_observer(composite_subscription cs, OnNext n, OnError e, OnCompleted c)
        : base_type(std::move(cs))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
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

template<class T, class OnNext, class OnError = detail::OnErrorEmpty, class OnCompleted = detail::OnCompletedEmpty>
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
    static_assert(detail::is_on_next_of<T, on_next_t>::value,     "Function supplied for on_next must be a function with the signature void(T);");
    static_assert(detail::is_on_error<on_error_t>::value,         "Function supplied for on_error must be a function with the signature void(std::exception_ptr);");
    static_assert(detail::is_on_completed<on_completed_t>::value, "Function supplied for on_completed must be a function with the signature void();");

    explicit static_observer(on_next_t n = on_next_t(), on_error_t e = on_error_t(), on_completed_t c = on_completed_t())
        : observer_base<T>(composite_subscription())
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    explicit static_observer(composite_subscription cs, on_next_t n = on_next_t(), on_error_t e = on_error_t(), on_completed_t c = on_completed_t())
        : observer_base<T>(std::move(cs))
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
        }
    }
    void on_completed() const {
        if (is_subscribed()) {
            detacher protect(this);
            inner.on_completed();
        }
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
    ->      observer<T, static_observer<T, OnNext>> {
    return  observer<T, static_observer<T, OnNext>>(
                        static_observer<T, OnNext>(std::move(n)));
}

template<class T, class OnNext, class OnError>
auto make_observer(OnNext n, OnError e, tag_function&&)
    ->      observer<T, static_observer<T, OnNext, OnError>> {
    return  observer<T, static_observer<T, OnNext, OnError>>(
                        static_observer<T, OnNext, OnError>(std::move(n), std::move(e)));
}

template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer(OnNext n, OnError e, OnCompleted c, tag_function&&)
    ->      observer<T, static_observer<T, OnNext, OnError, OnCompleted>> {
    return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                        static_observer<T, OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c)));
}

template<class T, class OnNext>
auto make_observer(composite_subscription cs, OnNext n, tag_subscription&&)
    ->      observer<T, static_observer<T, OnNext>> {
    return  observer<T, static_observer<T, OnNext>>(
                        static_observer<T, OnNext>(std::move(cs), std::move(n)));
}

template<class T, class OnNext, class OnError>
auto make_observer(composite_subscription cs, OnNext n, OnError e, tag_subscription&&)
    ->      observer<T, static_observer<T, OnNext, OnError>> {
    return  observer<T, static_observer<T, OnNext, OnError>>(
                        static_observer<T, OnNext, OnError>(std::move(cs), std::move(n), std::move(e)));
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

template<class T, class OnNext>
auto make_observer_dynamic(OnNext n)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(n)));
}
template<class T, class OnNext, class OnError>
auto make_observer_dynamic(OnNext n, OnError e)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(n), std::move(e)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer_dynamic(OnNext n, OnError e, OnCompleted c)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(n), std::move(e), std::move(c)));
}

template<class T, class OnNext>
auto make_observer_dynamic(composite_subscription cs, OnNext n)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(cs), std::move(n)));
}
template<class T, class OnNext, class OnError>
auto make_observer_dynamic(composite_subscription cs, OnNext n, OnError e)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(cs), std::move(n), std::move(e)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer_dynamic(composite_subscription cs, OnNext n, OnError e, OnCompleted c)
    ->      observer<T> {
    return  observer<T>(dynamic_observer<T>(std::move(cs), std::move(n), std::move(e), std::move(c)));
}

}

#endif
