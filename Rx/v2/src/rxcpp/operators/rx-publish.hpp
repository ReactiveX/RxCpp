// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_PUBLISH_HPP)
#define RXCPP_OPERATORS_RX_PUBLISH_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Subject>
struct publish : public operator_base<T>
{
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Subject>::type subject_type;
    source_type source;
    subject_type subject_value;
    rxu::detail::maybe<typename composite_subscription::weak_subscription> connection;

    explicit publish(source_type o)
        : source(std::move(o))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber&& o) {
        subject_value.get_observable().subscribe(std::forward<Subscriber>(o));
    }
    void on_connect(composite_subscription cs) {
        if (connection.empty()) {
            // the lifetime of each connect is independent
            auto destination = subject_value.get_subscriber();

            // when the paramter is unsubscribed it should
            // unsubscribe the most recent connection
            connection.reset(cs.add(destination.get_subscription()));

            // when the connection is finished it should shutdown the connection
            destination.add(make_subscription(
                [cs, this](){
                    cs.remove(this->connection.get());
                    this->connection.reset();
                }));

            source.subscribe(destination);
        }
    }
};

template<template<class T> class Subject>
class publish_factory
{
public:
    publish_factory() {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      connectable_observable<typename std::decay<Observable>::type::value_type,   publish<typename std::decay<Observable>::type::value_type, Observable, Subject<typename std::decay<Observable>::type::value_type>>> {
        return  connectable_observable<typename std::decay<Observable>::type::value_type,   publish<typename std::decay<Observable>::type::value_type, Observable, Subject<typename std::decay<Observable>::type::value_type>>>(
                                                                                            publish<typename std::decay<Observable>::type::value_type, Observable, Subject<typename std::decay<Observable>::type::value_type>>(std::forward<Observable>(source)));
    }
};

}

inline auto publish()
    ->      detail::publish_factory<rxsub::subject> {
    return  detail::publish_factory<rxsub::subject>();
}

}

}

#endif
