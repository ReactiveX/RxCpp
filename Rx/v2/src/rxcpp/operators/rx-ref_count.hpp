// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_REF_COUNT_HPP)
#define RXCPP_OPERATORS_RX_REF_COUNT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class ConnectableObservable>
struct ref_count : public operator_base<T>
{
    typedef rxu::decay_t<ConnectableObservable> source_type;

    struct ref_count_state : public std::enable_shared_from_this<ref_count_state>
    {
        explicit ref_count_state(source_type o)
            : source(std::move(o))
            , subscribers(0)
        {
        }

        source_type source;
        std::mutex lock;
        long subscribers;
        composite_subscription connection;
    };
    std::shared_ptr<ref_count_state> state;

    explicit ref_count(source_type o)
        : state(std::make_shared<ref_count_state>(std::move(o)))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber&& o) const {
        std::unique_lock<std::mutex> guard(state->lock);
        auto needConnect = ++state->subscribers == 1;
        auto keepAlive = state;
        guard.unlock();
        o.add(
            [keepAlive](){
                std::unique_lock<std::mutex> guard_unsubscribe(keepAlive->lock);
                if (--keepAlive->subscribers == 0) {
                    keepAlive->connection.unsubscribe();
                    keepAlive->connection = composite_subscription();
                }
            });
        keepAlive->source.subscribe(std::forward<Subscriber>(o));
        if (needConnect) {
            keepAlive->source.connect(keepAlive->connection);
        }
    }
};

class ref_count_factory
{
public:
    ref_count_factory() {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   ref_count<rxu::value_type_t<rxu::decay_t<Observable>>, Observable>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   ref_count<rxu::value_type_t<rxu::decay_t<Observable>>, Observable>>(
                                                                          ref_count<rxu::value_type_t<rxu::decay_t<Observable>>, Observable>(std::forward<Observable>(source)));
    }
};

}

inline auto ref_count()
    ->      detail::ref_count_factory {
    return  detail::ref_count_factory();
}

}

}

#endif
