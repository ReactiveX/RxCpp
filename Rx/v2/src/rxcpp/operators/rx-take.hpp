// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_HPP)
#define RXCPP_OPERATORS_RX_TAKE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Count>
struct take : public operator_base<T>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Count> count_type;
    struct values
    {
        values(source_type s, count_type t)
            : source(std::move(s))
            , count(std::move(t))
        {
        }
        source_type source;
        count_type count;
    };
    values initial;

    take(source_type s, count_type t)
        : initial(std::move(s), std::move(t))
    {
    }

    struct mode
    {
        enum type {
            taking,    // capture messages
            triggered, // ignore messages
            errored,   // error occured
            stopped    // observable completed
        };
    };

    template<class Subscriber>
    void on_subscribe(const Subscriber& s) const {

        typedef Subscriber output_type;
        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(const values& i, const output_type& oarg)
                : values(i)
                , mode_value(mode::taking)
                , out(oarg)
            {
            }
            typename mode::type mode_value;
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::make_shared<state_type>(initial, s);

        composite_subscription source_lifetime;

        s.add(source_lifetime);

        state->source.subscribe(
        // split subscription lifetime
            source_lifetime,
        // on_next
            [state, source_lifetime](T t) {
                if (state->mode_value < mode::triggered) {
                    if (--state->count > 0) {
                        state->out.on_next(t);
                    } else {
                        state->mode_value = mode::triggered;
                        state->out.on_next(t);
                        // must shutdown source before signaling completion
                        source_lifetime.unsubscribe();
                        state->out.on_completed();
                    }
                }
            },
        // on_error
            [state](std::exception_ptr e) {
                state->mode_value = mode::errored;
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                state->mode_value = mode::stopped;
                state->out.on_completed();
            }
        );
    }
};

template<class T>
class take_factory
{
    typedef rxu::decay_t<T> count_type;
    count_type count;
public:
    take_factory(count_type t) : count(std::move(t)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>>(
                                                                          take<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>(std::forward<Observable>(source), count));
    }
};

}

template<class T>
auto take(T&& t)
    ->      detail::take_factory<T> {
    return  detail::take_factory<T>(std::forward<T>(t));
}

}

}

#endif
