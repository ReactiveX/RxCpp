
// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SOURCES_HPP)
#define RXCPP_RX_SOURCES_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace sources {

struct tag_source {};
template<class T>
struct source_base
{
    typedef T value_type;
    typedef tag_source source_tag;
};
template<class T>
class is_source
{
    template<class C>
    static typename C::source_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_source>::value;
};

}
namespace rxs=sources;

template<class T>
class dynamic_observable
    : public rxs::source_base<T>
{
    struct state_type
        : public std::enable_shared_from_this<state_type>
    {
        typedef std::function<void(observer<T>)> onsubscribe_type;

        onsubscribe_type on_subscribe;
    };
    std::shared_ptr<state_type> state;

    template<class SO>
    void construct(SO so, rxs::tag_source&&) {
        state->on_subscribe = [so](observer<T> o) mutable {
            so.on_subscribe(std::move(o));
        };
    }

    struct tag_function {};
    template<class F>
    void construct(F&& f, tag_function&&) {
        state->on_subscribe = std::forward<F>(f);
    }

public:

    dynamic_observable()
    {
    }

    template<class SOF>
    explicit dynamic_observable(SOF&& sof)
        : state(std::make_shared<state_type>())
    {
        construct(std::forward<SOF>(sof),
            typename std::conditional<rxs::is_source<SOF>::value || rxo::is_operator<SOF>::value, rxs::tag_source, tag_function>::type());
    }

    void on_subscribe(observer<T> o) const {
        state->on_subscribe(std::move(o));
    }

    template<class Observer>
    typename std::enable_if<!std::is_same<typename std::decay<Observer>::type, observer<T>>::value, void>::type
    on_subscribe(Observer o) const {
        auto so = std::make_shared<Observer>(o);
        state->on_subscribe(make_observer_dynamic<T>(
            so->get_subscription(),
        // on_next
            [so](T t){
                so->on_next(t);
            },
        // on_error
            [so](std::exception_ptr e){
                so->on_error(e);
            },
        // on_completed
            [so](){
                so->on_completed();
            }));
    }
};

template<class T, class Source>
observable<T> make_dynamic_observable(Source&& s) {
    return observable<T>(dynamic_observable<T>(std::forward<Source>(s)));
}

}

#include "sources/rx-range.hpp"

#endif
