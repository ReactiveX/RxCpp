// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVER_HPP)
#define RXCPP_RX_OBSERVER_HPP

#include "rx-includes.hpp"

namespace rxcpp {


template<class T>
struct observer_base
{
    using value_type = T;
    using observer_tag = tag_observer;
};

namespace detail {
template<class T>
struct OnNextEmpty
{
    void operator()(const T&) const {}
};
struct OnErrorEmpty
{
    void operator()(rxu::error_ptr) const {
        // error implicitly ignored, abort
        std::terminate();
    }
};
struct OnErrorIgnore
{
    void operator()(rxu::error_ptr) const {
    }
};
struct OnCompletedEmpty
{
    void operator()() const {}
};

template<class T, class State, class OnNext>
struct OnNextForward
{
    using state_t = rxu::decay_t<State>;
    using onnext_t = rxu::decay_t<OnNext>;
    OnNextForward() : onnext() {}
    explicit OnNextForward(onnext_t on) : onnext(std::move(on)) {}
    onnext_t onnext;
    void operator()(state_t& s, const T& t) const {
        onnext(s, t);
    }
    void operator()(state_t& s, T&& t) const {
        onnext(s, std::move(t));
    }
};
template<class T, class State>
struct OnNextForward<T, State, void>
{
    using state_t = rxu::decay_t<State>;
    OnNextForward() {}
    void operator()(state_t& s, const T& t) const {
        s.on_next(t);
    }
    void operator()(state_t& s, T&& t) const {
        s.on_next(std::move(t));
    }
};

template<class State, class OnError>
struct OnErrorForward
{
    using state_t = rxu::decay_t<State>;
    using onerror_t = rxu::decay_t<OnError>;
    OnErrorForward() : onerror() {}
    explicit OnErrorForward(onerror_t oe) : onerror(std::move(oe)) {}
    onerror_t onerror;
    void operator()(state_t& s, rxu::error_ptr ep) const {
        onerror(s, ep);
    }
};
template<class State>
struct OnErrorForward<State, void>
{
    using state_t = rxu::decay_t<State>;
    OnErrorForward() {}
    void operator()(state_t& s, rxu::error_ptr ep) const {
        s.on_error(ep);
    }
};

template<class State, class OnCompleted>
struct OnCompletedForward
{
    using state_t = rxu::decay_t<State>;
    using oncompleted_t = rxu::decay_t<OnCompleted>;
    OnCompletedForward() : oncompleted() {}
    explicit OnCompletedForward(oncompleted_t oc) : oncompleted(std::move(oc)) {}
    oncompleted_t oncompleted;
    void operator()(state_t& s) const {
        oncompleted(s);
    }
};
template<class State>
struct OnCompletedForward<State, void>
{
    OnCompletedForward() {}
    void operator()(State& s) const {
        s.on_completed();
    }
};

template<class T, class F>
struct is_on_next_of
{
    struct not_void {};
    template<class CT, class CF>
    static auto check(int) -> decltype(std::declval<CF>()(std::declval<CT>()));
    template<class CT, class CF>
    static not_void check(...);

    using detail_result = decltype(check<T, rxu::decay_t < F>>(0));
    static const bool value = rxcpp::is_same_v<detail_result, void>;
};

template<class F>
struct is_on_error
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype(std::declval<CF>()(std::declval<rxu::error_ptr>()));
    template<class CF>
    static not_void check(...);

    static const bool value = rxcpp::is_same_v<decltype(check<rxu::decay_t<F>>(0)), void>;
};

template<class State, class F>
struct is_on_error_for
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype(std::declval<CF>()(std::declval<State>(), std::declval<rxu::error_ptr>()));
    template<class CF>
    static not_void check(...);

    static const bool value = rxcpp::is_same_v<decltype(check<rxu::decay_t<F>>(0)), void>;
};

template<class F>
struct is_on_completed
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype(std::declval<CF>()());
    template<class CF>
    static not_void check(...);

    static const bool value = rxcpp::is_same_v<decltype(check<rxu::decay_t<F>>(0)), void>;
};

}


/*!
    \brief consumes values from an observable using `State` that may implement on_next, on_error and on_completed with optional overrides of each function.

    \tparam T            - the type of value in the stream
    \tparam State        - the type of the stored state
    \tparam OnNext       - the type of a function that matches `void(State&, T)`. Called 0 or more times. If `void` State::on_next will be called.
    \tparam OnError      - the type of a function that matches `void(State&, rxu::error_ptr)`. Called 0 or 1 times, no further calls will be made. If `void` State::on_error will be called.
    \tparam OnCompleted  - the type of a function that matches `void(State&)`. Called 0 or 1 times, no further calls will be made. If `void` State::on_completed will be called.

    \ingroup group-core

*/
template<class T, class State, class OnNext, class OnError, class OnCompleted>
class observer : public observer_base<T>
{
public:
    using this_type = observer<T, State, OnNext, OnError, OnCompleted>;
    using state_t = rxu::decay_t<State>;
    using on_next_t = typename std::conditional_t<!rxcpp::is_same_v<void, OnNext>, rxu::decay_t<OnNext>, detail::OnNextForward<T, State, OnNext>>;
    using on_error_t = typename std::conditional_t<!rxcpp::is_same_v<void, OnError>, rxu::decay_t<OnError>, detail::OnErrorForward<State, OnError>>;
    using on_completed_t = typename std::conditional_t<!rxcpp::is_same_v<void, OnCompleted>, rxu::decay_t<OnCompleted>, detail::OnCompletedForward<State, OnCompleted>>;

private:
    mutable state_t state;
    on_next_t onnext;
    on_error_t onerror;
    on_completed_t oncompleted;

public:

    explicit observer(state_t s, on_next_t n = on_next_t(), on_error_t e = on_error_t(), on_completed_t c = on_completed_t())
        : state(std::move(s))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    explicit observer(state_t s, on_next_t n, on_completed_t c)
        : state(std::move(s))
        , onnext(std::move(n))
        , onerror(on_error_t())
        , oncompleted(std::move(c))
    {
    }
    observer(const this_type& o)
        : state(o.state)
        , onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    observer(this_type&& o)
        : state(std::move(o.state))
        , onnext(std::move(o.onnext))
        , onerror(std::move(o.onerror))
        , oncompleted(std::move(o.oncompleted))
    {
    }
    this_type& operator=(this_type o) {
        state = std::move(o.state);
        onnext = std::move(o.onnext);
        onerror = std::move(o.onerror);
        oncompleted = std::move(o.oncompleted);
        return *this;
    }
    void on_next(const T& t) const {
        onnext(state, t);
    }
    void on_next(T&& t) const {
        onnext(state, std::move(t));
    }
    void on_error(rxu::error_ptr e) const {
        onerror(state, e);
    }
    void on_completed() const {
        oncompleted(state);
    }
    observer<T> as_dynamic() const {
        return observer<T>(*this);
    }
};

/*!
    \brief consumes values from an observable using default empty method implementations with optional overrides of each function.

    \tparam T            - the type of value in the stream
    \tparam OnNext       - the type of a function that matches `void(T)`. Called 0 or more times. If `void` OnNextEmpty<T> is used.
    \tparam OnError      - the type of a function that matches `void(rxu::error_ptr)`. Called 0 or 1 times, no further calls will be made. If `void` OnErrorEmpty is used.
    \tparam OnCompleted  - the type of a function that matches `void()`. Called 0 or 1 times, no further calls will be made. If `void` OnCompletedEmpty is used.

    \ingroup group-core

*/
template<class T, class OnNext, class OnError, class OnCompleted>
class observer<T, detail::stateless_observer_tag, OnNext, OnError, OnCompleted> : public observer_base<T>
{
public:
    using this_type = observer<T, detail::stateless_observer_tag, OnNext, OnError, OnCompleted>;
    using on_next_t = typename std::conditional_t<!rxcpp::is_same_v<void, OnNext>, rxu::decay_t<OnNext>, detail::OnNextEmpty<T>>;
    using on_error_t = typename std::conditional_t<!rxcpp::is_same_v<void, OnError>, rxu::decay_t<OnError>, detail::OnErrorEmpty>;
    using on_completed_t = typename std::conditional_t<!rxcpp::is_same_v<void, OnCompleted>, rxu::decay_t<OnCompleted>, detail::OnCompletedEmpty>;

private:
    on_next_t onnext;
    on_error_t onerror;
    on_completed_t oncompleted;

public:
    static_assert(detail::is_on_next_of<T, on_next_t>::value,     "Function supplied for on_next must be a function with the signature void(T);");
    static_assert(detail::is_on_error<on_error_t>::value,         "Function supplied for on_error must be a function with the signature void(rxu::error_ptr);");
    static_assert(detail::is_on_completed<on_completed_t>::value, "Function supplied for on_completed must be a function with the signature void();");

    observer()
        : onnext(on_next_t())
        , onerror(on_error_t())
        , oncompleted(on_completed_t())
    {
    }

    explicit observer(on_next_t n, on_error_t e = on_error_t(), on_completed_t c = on_completed_t())
        : onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {
    }
    observer(const this_type& o)
        : onnext(o.onnext)
        , onerror(o.onerror)
        , oncompleted(o.oncompleted)
    {
    }
    observer(this_type&& o)
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
    void on_next(const T& t) const {
        onnext(t);
    }
    void on_next(T&& t) const {
        onnext(std::move(t));
    }
    void on_error(rxu::error_ptr e) const {
        onerror(e);
    }
    void on_completed() const {
        oncompleted();
    }
    observer<T> as_dynamic() const {
        return observer<T>(*this);
    }
};

namespace detail
{

template<class T>
struct virtual_observer : public std::enable_shared_from_this<virtual_observer<T>>
{
    virtual ~virtual_observer() {}
    virtual void on_next(const T&) const {};
    virtual void on_next(T&&) const {};
    virtual void on_error(rxu::error_ptr) const {};
    virtual void on_completed() const {};
};

template<class T, class Observer>
struct specific_observer : public virtual_observer<T>
{
    explicit specific_observer(Observer o)
        : destination(std::move(o))
    {
    }

    Observer destination;
    void on_next(const T& t) const override {
        destination.on_next(t);
    }
    void on_next(T&& t) const override{
        destination.on_next(std::move(t));
    }
    void on_error(rxu::error_ptr e) const override{
        destination.on_error(e);
    }
    void on_completed() const override {
        destination.on_completed();
    }
};

}

/*!
    \brief consumes values from an observable using type-forgetting (shared allocated state with virtual methods)

    \tparam T            - the type of value in the stream

    \ingroup group-core

*/
template<class T>
class observer<T, void, void, void, void> : public observer_base<T>
{
public:
    using dynamic_observer_tag = tag_dynamic_observer;

private:
    using this_type = observer<T, void, void, void, void>;
    using base_type = observer_base<T>;
    using virtual_observer = detail::virtual_observer<T>;

    std::shared_ptr<virtual_observer> destination;

    template<class Observer>
    static auto make_destination(Observer o)
        -> std::shared_ptr<virtual_observer> {
        return std::make_shared<detail::specific_observer<T, Observer>>(std::move(o));
    }

public:
    observer()
    {
    }
    observer(const this_type& o)
        : destination(o.destination)
    {
    }
    observer(this_type&& o)
        : destination(std::move(o.destination))
    {
    }

    template<class Observer>
    explicit observer(Observer o)
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
    void on_error(rxu::error_ptr e) const {
        if (destination) {
            destination->on_error(e);
        }
    }
    void on_completed() const {
        if (destination) {
            destination->on_completed();
        }
    }

    observer<T> as_dynamic() const {
        return *this;
    }
};

template<class T, class DefaultOnError = detail::OnErrorEmpty>
auto make_observer()
    ->      observer<T, detail::stateless_observer_tag, detail::OnNextEmpty<T>, DefaultOnError> {
    return  observer<T, detail::stateless_observer_tag, detail::OnNextEmpty<T>, DefaultOnError>();
}

template<class T, class DefaultOnError = detail::OnErrorEmpty, class U, class State, class OnNext, class OnError, class OnCompleted>
auto make_observer(observer<U, State, OnNext, OnError, OnCompleted> o)
    ->      observer<T, State, OnNext, OnError, OnCompleted> {
    return  observer<T, State, OnNext, OnError, OnCompleted>(std::move(o));
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class Observer>
auto make_observer(Observer ob)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, Observer>::value &&
        !detail::is_on_error<Observer>::value &&
        is_observer<Observer>::value,
            Observer>::type {
    return  std::move(ob);
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class Observer>
auto make_observer(Observer ob)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, Observer>::value &&
        !detail::is_on_error<Observer>::value &&
        !is_observer<Observer>::value,
            observer<T, Observer>>::type {
    return  observer<T, Observer>(std::move(ob));
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class OnNext>
auto make_observer(OnNext on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            observer<T, detail::stateless_observer_tag, OnNext, DefaultOnError>>::type {
    return  observer<T, detail::stateless_observer_tag, OnNext, DefaultOnError>(
                        std::move(on));
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class OnError>
auto make_observer(OnError oe)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, OnError>::value &&
        detail::is_on_error<OnError>::value,
            observer<T, detail::stateless_observer_tag, detail::OnNextEmpty<T>, OnError>>::type {
    return  observer<T, detail::stateless_observer_tag, detail::OnNextEmpty<T>, OnError>(
                        detail::OnNextEmpty<T>(), std::move(oe));
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class OnNext, class OnError>
auto make_observer(OnNext on, OnError oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            observer<T, detail::stateless_observer_tag, OnNext, OnError>>::type {
    return  observer<T, detail::stateless_observer_tag, OnNext, OnError>(
                        std::move(on), std::move(oe));
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class OnNext, class OnCompleted>
auto make_observer(OnNext on, OnCompleted oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, detail::stateless_observer_tag, OnNext, DefaultOnError, OnCompleted>>::type {
    return  observer<T, detail::stateless_observer_tag, OnNext, DefaultOnError, OnCompleted>(
                        std::move(on), DefaultOnError(), std::move(oc));
}
template<class T, class DefaultOnError = detail::OnErrorEmpty, class OnNext, class OnError, class OnCompleted>
auto make_observer(OnNext on, OnError oe, OnCompleted oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T, detail::stateless_observer_tag, OnNext, OnError, OnCompleted>>::type {
    return  observer<T, detail::stateless_observer_tag, OnNext, OnError, OnCompleted>(
                        std::move(on), std::move(oe), std::move(oc));
}


template<class T, class State, class OnNext>
auto make_observer(State os, OnNext on)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, State>::value &&
        !detail::is_on_error<State>::value,
            observer<T, State, OnNext>>::type {
    return  observer<T, State, OnNext>(
                        std::move(os), std::move(on));
}
template<class T, class State, class OnError>
auto make_observer(State os, OnError oe)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, State>::value &&
        !detail::is_on_error<State>::value &&
        detail::is_on_error_for<State, OnError>::value,
            observer<T, State, detail::OnNextEmpty<T>, OnError>>::type {
    return  observer<T, State, detail::OnNextEmpty<T>, OnError>(
                        std::move(os), detail::OnNextEmpty<T>(), std::move(oe));
}
template<class T, class State, class OnNext, class OnError>
auto make_observer(State os, OnNext on, OnError oe)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, State>::value &&
        !detail::is_on_error<State>::value &&
        detail::is_on_error_for<State, OnError>::value,
            observer<T, State, OnNext, OnError>>::type {
    return  observer<T, State, OnNext, OnError>(
                        std::move(os), std::move(on), std::move(oe));
}
template<class T, class State, class OnNext, class OnCompleted>
auto make_observer(State os, OnNext on, OnCompleted oc)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, State>::value &&
        !detail::is_on_error<State>::value,
            observer<T, State, OnNext, void, OnCompleted>>::type {
    return  observer<T, State, OnNext, void, OnCompleted>(
                        std::move(os), std::move(on), std::move(oc));
}
template<class T, class State, class OnNext, class OnError, class OnCompleted>
auto make_observer(State os, OnNext on, OnError oe, OnCompleted oc)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, State>::value &&
        !detail::is_on_error<State>::value &&
        detail::is_on_error_for<State, OnError>::value,
            observer<T, State, OnNext, OnError, OnCompleted>>::type {
    return  observer<T, State, OnNext, OnError, OnCompleted>(
                        std::move(os), std::move(on), std::move(oe), std::move(oc));
}

template<class T, class Observer>
auto make_observer_dynamic(Observer o)
    -> typename std::enable_if<
        !detail::is_on_next_of<T, Observer>::value,
            observer<T>>::type {
    return  observer<T>(std::move(o));
}
template<class T, class OnNext>
auto make_observer_dynamic(OnNext&& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            observer<T>>::type {
    return  observer<T>(
                make_observer<T>(std::forward<OnNext>(on)));
}
template<class T, class OnNext, class OnError>
auto make_observer_dynamic(OnNext&& on, OnError&& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            observer<T>>::type {
    return  observer<T>(
                make_observer<T>(std::forward<OnNext>(on), std::forward<OnError>(oe)));
}
template<class T, class OnNext, class OnCompleted>
auto make_observer_dynamic(OnNext&& on, OnCompleted&& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T>>::type {
    return  observer<T>(
                make_observer<T>(std::forward<OnNext>(on), std::forward<OnCompleted>(oc)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_observer_dynamic(OnNext&& on, OnError&& oe, OnCompleted&& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            observer<T>>::type {
    return  observer<T>(
                make_observer<T>(std::forward<OnNext>(on), std::forward<OnError>(oe), std::forward<OnCompleted>(oc)));
}

namespace detail {

template<class F>
struct maybe_from_result
{
    using decl_result_type = decltype(std::declval<F>()());
    using result_type = rxu::decay_t<decl_result_type>;
    using type = rxu::maybe<result_type>;
};

}

template<class F, class OnError>
auto on_exception(const F& f, const OnError& c)
    ->  typename std::enable_if<detail::is_on_error<OnError>::value, typename detail::maybe_from_result<F>::type>::type {
    RXCPP_TRY {
        return f();
    } RXCPP_CATCH(...) {
        c(rxu::current_exception());
    }
    return {};
}

template<class F, class Subscriber>
auto on_exception(const F& f, const Subscriber& s)
    ->  typename std::enable_if<is_subscriber<Subscriber>::value, typename detail::maybe_from_result<F>::type>::type {
    RXCPP_TRY {
        return f();
    } RXCPP_CATCH(...) {
        s.on_error(rxu::current_exception());
    }
    return {};
}

template<class F, class OnError>
auto on_exception_no_return(const F& f, const OnError& c)
    ->  typename std::enable_if<detail::is_on_error<OnError>::value, void>::type {
    RXCPP_TRY {
        f();
    } RXCPP_CATCH(...) {
        c(rxu::current_exception());
    };
}

template<class F, class Subscriber>
auto on_exception_no_return(const F& f, const Subscriber& s)
    ->  typename std::enable_if<is_subscriber<Subscriber>::value, void>::type {
    RXCPP_TRY {
        f();
    } RXCPP_CATCH(...) {
        s.on_error(rxu::current_exception());
    }
}

}

#endif
