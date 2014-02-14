
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
struct dynamic_observable
    : public rxs::source_base<T>
{
    struct state_type
        : public std::enable_shared_from_this<state_type>
    {
        typedef std::function<void(observer<T, dynamic_observer<T>>)> onsubscribe_type;

        onsubscribe_type on_subscribe;
    };
    std::shared_ptr<state_type> state;

    dynamic_observable()
    {
    }

    template<class Source>
    explicit dynamic_observable(Source source)
        : state(std::make_shared<state_type>())
    {
        state->on_subscribe = [source](observer<T, dynamic_observer<T>> o) mutable {
            source.on_subscribe(std::move(o));
        };
    }

    void on_subscribe(observer<T, dynamic_observer<T>> o) const {
        state->on_subscribe(std::move(o));
    }

    template<class Observer>
    typename std::enable_if<!std::is_same<typename std::decay<Observer>::type, observer<T, dynamic_observer<T>>>::value, void>::type
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

}

#include "sources/rx-range.hpp"

#endif
