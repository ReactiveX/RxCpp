// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_WHILE_HPP)
#define RXCPP_OPERATORS_RX_TAKE_WHILE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Predicate>
struct take_while : public operator_base<T>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Predicate> test_type;
    struct values
    {
        values(source_type s, test_type t)
            : source(std::move(s))
            , test(std::move(t))
        {
        }
        source_type source;
        test_type test;
    };
    values initial;

    take_while(source_type s, test_type t)
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
                    if (state->test(t)) {
                        state->out.on_next(t);
                    } else {
                        state->mode_value = mode::triggered;
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
class take_while_factory
{
    typedef rxu::decay_t<T> test_type;
    test_type test;
public:
    take_while_factory(test_type t) : test(std::move(t)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take_while<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, test_type>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take_while<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, test_type>>(
                                                                          take_while<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, test_type>(std::forward<Observable>(source), test));
    }
};

}

template<class T>
auto take_while(T&& t)
    ->      detail::take_while_factory<T> {
    return  detail::take_while_factory<T>(std::forward<T>(t));
}

}

}

#endif
