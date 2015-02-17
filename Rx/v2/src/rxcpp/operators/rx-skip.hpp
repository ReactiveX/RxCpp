// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SKIP_HPP)
#define RXCPP_OPERATORS_RX_SKIP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Count>
struct skip : public operator_base<T>
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

    skip(source_type s, count_type t)
        : initial(std::move(s), std::move(t))
    {
    }

    struct mode
    {
        enum type {
            skipping,  // ignore messages
            triggered, // capture messages
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
                , mode_value(i.count > 0 ? mode::skipping : mode::triggered)
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
            [state](T t) {
                if (state->mode_value == mode::skipping) {
                    if (--state->count == 0) {
                        state->mode_value = mode::triggered;
                    }
                } else {
                    state->out.on_next(t);
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
class skip_factory
{
    typedef rxu::decay_t<T> count_type;
    count_type count;
public:
    skip_factory(count_type t) : count(std::move(t)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>, skip<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>, skip<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>>(
                                                                        skip<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>(std::forward<Observable>(source), count));
    }
};

}

template<class T>
auto skip(T&& t)
    ->      detail::skip_factory<T> {
    return  detail::skip_factory<T>(std::forward<T>(t));
}

}

}

#endif
