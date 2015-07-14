// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_PREDEF_HPP)
#define RXCPP_RX_PREDEF_HPP

#include "rx-includes.hpp"

namespace rxcpp {

//
// create a typedef for rxcpp_trace_type to override the default
//
inline auto trace_activity() -> decltype(rxcpp_trace_activity(trace_tag()))& {
    static decltype(rxcpp_trace_activity(trace_tag())) trace;
    return trace;
}


struct tag_action {};
template<class T, class C = rxu::types_checked>
struct is_action : public std::false_type {};

template<class T>
struct is_action<T, typename rxu::types_checked_from<typename T::action_tag>::type>
    : public std::is_convertible<typename T::action_tag*, tag_action*> {};


struct tag_worker {};
template<class T>
class is_worker
{
    struct not_void {};
    template<class C>
    static typename C::worker_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_worker*>::value;
};

struct tag_scheduler {};
template<class T>
class is_scheduler
{
    struct not_void {};
    template<class C>
    static typename C::scheduler_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_scheduler*>::value;
};

struct tag_schedulable {};
template<class T>
class is_schedulable
{
    struct not_void {};
    template<class C>
    static typename C::schedulable_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_schedulable*>::value;
};

namespace detail
{

struct stateless_observer_tag {};

}

// state with optional overrides
template<class T, class State = void, class OnNext = void, class OnError = void, class OnCompleted = void>
class observer;

// no state with optional overrides
template<class T, class OnNext, class OnError, class OnCompleted>
class observer<T, detail::stateless_observer_tag, OnNext, OnError, OnCompleted>;

// virtual functions forward to dynamically allocated shared observer instance.
template<class T>
class observer<T, void, void, void, void>;

struct tag_observer {};
template<class T>
class is_observer
{
    template<class C>
    static typename C::observer_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_observer*>::value;
};

struct tag_dynamic_observer {};
template<class T>
class is_dynamic_observer
{
    struct not_void {};
    template<class C>
    static typename C::dynamic_observer_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_dynamic_observer*>::value;
};

struct tag_subscriber {};
template<class T>
class is_subscriber
{
    struct not_void {};
    template<class C>
    static typename C::subscriber_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_subscriber*>::value;
};

struct tag_dynamic_observable {};
template<class T>
class is_dynamic_observable
{
    struct not_void {};
    template<class C>
    static typename C::dynamic_observable_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_dynamic_observable*>::value;
};

template<class T>
class dynamic_observable;

template<
    class T = void,
    class SourceObservable = typename std::conditional<std::is_same<T, void>::value,
        void, dynamic_observable<T>>::type>
class observable;

template<class T, class Source>
observable<T> make_observable_dynamic(Source&&);

template<class Selector, class Default, template<class... TN> class SO, class... AN>
struct defer_observable;

struct tag_observable {};
template<class T>
struct observable_base {
    typedef tag_observable observable_tag;
    typedef T value_type;
};
template<class T>
class is_observable
{
    template<class C>
    static typename C::observable_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_observable>::value;
};

struct tag_dynamic_connectable_observable : public tag_dynamic_observable {};

template<class T>
class is_dynamic_connectable_observable
{
    struct not_void {};
    template<class C>
    static typename C::dynamic_observable_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_dynamic_connectable_observable*>::value;
};

template<class T>
class dynamic_connectable_observable;

template<class T,
    class SourceObservable = typename std::conditional<std::is_same<T, void>::value,
        void, dynamic_connectable_observable<T>>::type>
class connectable_observable;

struct tag_connectable_observable : public tag_observable {};
template<class T>
class is_connectable_observable
{
    template<class C>
    static typename C::observable_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_connectable_observable>::value;
};

struct tag_dynamic_grouped_observable : public tag_dynamic_observable {};

template<class T>
class is_dynamic_grouped_observable
{
    struct not_void {};
    template<class C>
    static typename C::dynamic_observable_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_dynamic_grouped_observable*>::value;
};

template<class K, class T>
class dynamic_grouped_observable;

template<class K, class T,
    class SourceObservable = typename std::conditional<std::is_same<T, void>::value,
        void, dynamic_grouped_observable<K, T>>::type>
class grouped_observable;

template<class K, class T, class Source>
grouped_observable<K, T> make_dynamic_grouped_observable(Source&& s);

struct tag_grouped_observable : public tag_observable {};
template<class T>
class is_grouped_observable
{
    template<class C>
    static typename C::observable_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_grouped_observable>::value;
};

//
// this type is the default used by operators that subscribe to
// multiple sources. It assumes that the sources are already synchronized
//
struct identity_observable
{
    template<class Observable>
    auto operator()(Observable o)
        -> Observable {
        return      std::move(o);
        static_assert(is_observable<Observable>::value, "only support observables");
    }
};

template<class T>
struct identity_for
{
    T operator()(T t) {
        return      std::move(t);
    }
};

}

#endif
