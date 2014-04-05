// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_HPP)
#define RXCPP_OPERATORS_RX_TAKE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class TriggerObservable>
struct take_until : public operator_base<T>
{
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<TriggerObservable>::type trigger_source_type;
    struct values
    {
        values(source_type s, trigger_source_type t)
            : source(std::move(s))
            , trigger(std::move(t))
        {
        }
        source_type source;
        trigger_source_type trigger;
    };
    values initial;

    take_until(source_type s, trigger_source_type t)
        : initial(std::move(s), std::move(t))
    {
    }

    struct mode
    {
        enum type {
            taking,
            clear,
            triggered,
            errored,
            stopped
        };
    };

    template<class Subscriber>
    void on_subscribe(const Subscriber& s) {

        typedef Subscriber output_type;
        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(const values& i, const output_type& oarg)
                : values(i)
                , mode_value(mode::taking)
                , busy(0)
                , out(oarg)
            {
            }
            mutable std::atomic<typename mode::type> mode_value;
            mutable std::atomic<int> busy;
            mutable std::exception_ptr exception;
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::shared_ptr<state_type>(new state_type(initial, s));

        struct activity
        {
            const std::shared_ptr<state_type>& st;
            ~activity() {
                if (--st->busy == 0 && st->mode_value > mode::clear) {
                    // need to finish
                    if (st->mode_value < mode::stopped && ++st->busy == 1) {
                        // this is the finish owner
                        auto fin = st->mode_value.exchange(mode::stopped);
                        if (fin == mode::triggered) {
                            st->out.on_completed();
                        } else if (fin == mode::errored) {
                            st->out.on_error(st->exception);
                        }
                    }
                }
            }
            explicit activity(const std::shared_ptr<state_type>& st)
                : st(st)
            {
                ++st->busy;
            }

        };

        state->trigger.subscribe(
        // share subscription lifetime
            state->out,
        // on_next
            [state](const typename trigger_source_type::value_type&) {
                activity finisher(state);
                state->mode_value = mode::triggered;
            },
        // on_error
            [state](std::exception_ptr e) {
                activity finisher(state);
                state->exception = e;
                state->mode_value = mode::errored;
            },
        // on_completed
            [state]() {
                activity finisher(state);
                state->mode_value = mode::clear;
            }
        );

        state->source.subscribe(
        // share subscription lifetime
            state->out,
        // on_next
            [state](T t) {
                activity finisher(state);
                if (state->mode_value < mode::triggered) {
                    state->out.on_next(t);
                }
            },
        // on_error
            [state](std::exception_ptr e) {
                activity finisher(state);
                state->exception = e;
                state->mode_value = mode::errored;
            },
        // on_completed
            [state]() {
                activity finisher(state);
                state->mode_value = mode::triggered;
            }
        );
    }
};

template<class TriggerObservable>
class take_until_factory
{
    typedef typename std::decay<TriggerObservable>::type trigger_source_type;
    trigger_source_type trigger_source;
public:
    take_until_factory(trigger_source_type t) : trigger_source(std::move(t)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   take_until<typename std::decay<Observable>::type::value_type, Observable, trigger_source_type>> {
        return  observable<typename std::decay<Observable>::type::value_type,   take_until<typename std::decay<Observable>::type::value_type, Observable, trigger_source_type>>(
                                                                                take_until<typename std::decay<Observable>::type::value_type, Observable, trigger_source_type>(std::forward<Observable>(source), std::move(trigger_source)));
    }
};

}

template<class TriggerObservable>
auto take_until(TriggerObservable&& t)
    ->      detail::take_until_factory<TriggerObservable> {
    return  detail::take_until_factory<TriggerObservable>(std::forward<TriggerObservable>(t));
}

}

}

#endif
