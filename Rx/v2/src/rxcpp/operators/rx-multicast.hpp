// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_MULTICAST_HPP)
#define RXCPP_OPERATORS_RX_MULTICAST_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Subject>
struct multicast : public operator_base<T>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Subject> subject_type;

    struct multicast_state : public std::enable_shared_from_this<multicast_state>
    {
        multicast_state(source_type o, subject_type sub)
            : source(std::move(o))
            , subject_value(std::move(sub))
        {
        }
        source_type source;
        subject_type subject_value;
        rxu::detail::maybe<typename composite_subscription::weak_subscription> connection;
    };

    std::shared_ptr<multicast_state> state;

    multicast(source_type o, subject_type sub)
        : state(std::make_shared<multicast_state>(std::move(o), std::move(sub)))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber&& o) const {
        state->subject_value.get_observable().subscribe(std::forward<Subscriber>(o));
    }
    void on_connect(composite_subscription cs) const {
        if (state->connection.empty()) {
            auto destination = state->subject_value.get_subscriber();

            // the lifetime of each connect is nested in the subject lifetime
            state->connection.reset(destination.add(cs));

            auto localState = state;

            // when the connection is finished it should shutdown the connection
            cs.add(
                [destination, localState](){
                    if (!localState->connection.empty()) {
                        destination.remove(localState->connection.get());
                        localState->connection.reset();
                    }
                });

            // use cs not destination for lifetime of subscribe.
            state->source.subscribe(cs, destination);
        }
    }
};

template<class Subject>
class multicast_factory
{
    Subject caster;
public:
    multicast_factory(Subject sub)
        : caster(std::move(sub))
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      connectable_observable<rxu::value_type_t<rxu::decay_t<Observable>>,   multicast<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Subject>> {
        return  connectable_observable<rxu::value_type_t<rxu::decay_t<Observable>>,   multicast<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Subject>>(
                                                                                      multicast<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Subject>(std::forward<Observable>(source), caster));
    }
};

}

template<class Subject>
inline auto multicast(Subject sub)
    ->      detail::multicast_factory<Subject> {
    return  detail::multicast_factory<Subject>(std::move(sub));
}

}

}

#endif
