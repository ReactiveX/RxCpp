// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_UNTIL_HPP)
#define RXCPP_OPERATORS_RX_TAKE_UNTIL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class TriggerObservable, class SourceFilter>
struct take_until : public operator_base<T>
{
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<TriggerObservable>::type trigger_source_type;
    typedef typename std::decay<SourceFilter>::type source_filter_type;
    struct values
    {
        values(source_type s, trigger_source_type t, source_filter_type sf)
            : source(std::move(s))
            , trigger(std::move(t))
            , sourceFilter(std::move(sf))
        {
        }
        source_type source;
        trigger_source_type trigger;
        source_filter_type sourceFilter;
    };
    values initial;

    take_until(source_type s, trigger_source_type t, source_filter_type sf)
        : initial(std::move(s), std::move(t), std::move(sf))
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
        struct take_until_state_type
            : public std::enable_shared_from_this<take_until_state_type>
            , public values
        {
            take_until_state_type(const values& i, const output_type& oarg)
                : values(i)
                , mode_value(mode::taking)
                , out(oarg)
            {
                out.add(trigger_lifetime);
                out.add(source_lifetime);
            }
            typename mode::type mode_value;
            composite_subscription trigger_lifetime;
            composite_subscription source_lifetime;
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::shared_ptr<take_until_state_type>(new take_until_state_type(initial, s));

        auto trigger = on_exception(
            [&](){return state->sourceFilter(state->trigger);},
            state->out);
        if (trigger.empty()) {
            return;
        }

        auto source = on_exception(
            [&](){return state->sourceFilter(state->source);},
            state->out);
        if (source.empty()) {
            return;
        }

        trigger.get().subscribe(
        // share parts of subscription
            state->out,
        // new lifetime
            state->trigger_lifetime,
        // on_next
            [state](const typename trigger_source_type::value_type&) {
                if (state->mode_value != mode::taking) {return;}
                state->mode_value = mode::triggered;
                state->out.on_completed();
            },
        // on_error
            [state](std::exception_ptr e) {
                if (state->mode_value != mode::taking) {return;}
                state->mode_value = mode::errored;
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (state->mode_value != mode::taking) {return;}
                state->mode_value = mode::clear;
            }
        );

        source.get().subscribe(
        // split subscription lifetime
            state->source_lifetime,
        // on_next
            [state](T t) {
                //
                // everything is crafted to minimize the overhead of this function.
                //
                if (state->mode_value < mode::triggered) {
                    state->out.on_next(t);
                }
            },
        // on_error
            [state](std::exception_ptr e) {
                if (state->mode_value > mode::clear) {return;}
                state->mode_value = mode::errored;
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (state->mode_value > mode::clear) {return;}
                state->mode_value = mode::triggered;
                state->out.on_completed();
            }
        );
    }
};

template<class TriggerObservable, class SourceFilter>
class take_until_factory
{
    typedef typename std::decay<TriggerObservable>::type trigger_source_type;
    typedef typename std::decay<SourceFilter>::type source_filter_type;

    trigger_source_type trigger_source;
    source_filter_type source_filter;
public:
    take_until_factory(trigger_source_type t, source_filter_type sf)
        : trigger_source(std::move(t))
        , source_filter(std::move(sf))
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   take_until<typename std::decay<Observable>::type::value_type, Observable, trigger_source_type, SourceFilter>> {
        return  observable<typename std::decay<Observable>::type::value_type,   take_until<typename std::decay<Observable>::type::value_type, Observable, trigger_source_type, SourceFilter>>(
                                                                                take_until<typename std::decay<Observable>::type::value_type, Observable, trigger_source_type, SourceFilter>(std::forward<Observable>(source), trigger_source, source_filter));
    }
};

}

template<class TriggerObservable, class SourceFilter>
auto take_until(TriggerObservable&& t, SourceFilter&& sf)
    ->      detail::take_until_factory<TriggerObservable, SourceFilter> {
    return  detail::take_until_factory<TriggerObservable, SourceFilter>(std::forward<TriggerObservable>(t), std::forward<SourceFilter>(sf));
}

}

}

#endif
