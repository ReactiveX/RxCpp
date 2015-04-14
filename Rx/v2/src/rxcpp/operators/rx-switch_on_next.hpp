// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SWITCH_ON_NEXT_HPP)
#define RXCPP_OPERATORS_RX_SWITCH_ON_NEXT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Coordination>
struct switch_on_next
    : public operator_base<rxu::value_type_t<rxu::decay_t<T>>>
{
    //static_assert(is_observable<Observable>::value, "switch_on_next requires an observable");
    //static_assert(is_observable<T>::value, "switch_on_next requires an observable that contains observables");

    typedef switch_on_next<T, Observable, Coordination> this_type;

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Observable> source_type;

    typedef typename source_type::source_operator_type source_operator_type;

    typedef source_value_type collection_type;
    typedef typename collection_type::value_type collection_value_type;

    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

    struct values
    {
        values(source_operator_type o, coordination_type sf)
            : source_operator(std::move(o))
            , coordination(std::move(sf))
        {
        }
        source_operator_type source_operator;
        coordination_type coordination;
    };
    values initial;

    switch_on_next(const source_type& o, coordination_type sf)
        : initial(o.source_operator, std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

        struct switch_state_type
            : public std::enable_shared_from_this<switch_state_type>
            , public values
        {
            switch_state_type(values i, coordinator_type coor, output_type oarg)
                : values(i)
                , source(i.source_operator)
                , pendingCompletions(0)
                , coordinator(std::move(coor))
                , out(std::move(oarg))
            {
            }
            observable<source_value_type, source_operator_type> source;
            // on_completed on the output must wait until all the
            // subscriptions have received on_completed
            int pendingCompletions;
            coordinator_type coordinator;
            composite_subscription inner_lifetime;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<switch_state_type>(initial, std::move(coordinator), std::move(scbr));

        composite_subscription outercs;

        // when the out observer is unsubscribed all the
        // inner subscriptions are unsubscribed as well
        state->out.add(outercs);

        auto source = on_exception(
            [&](){return state->coordinator.in(state->source);},
            state->out);
        if (source.empty()) {
            return;
        }

        ++state->pendingCompletions;
        // this subscribe does not share the observer subscription
        // so that when it is unsubscribed the observer can be called
        // until the inner subscriptions have finished
        auto sink = make_subscriber<collection_type>(
            state->out,
            outercs,
        // on_next
            [state](collection_type st) {

                state->inner_lifetime.unsubscribe();

                state->inner_lifetime = composite_subscription();

                // when the out observer is unsubscribed all the
                // inner subscriptions are unsubscribed as well
                auto innerlifetimetoken = state->out.add(state->inner_lifetime);

                state->inner_lifetime.add(make_subscription([state, innerlifetimetoken](){
                    state->out.remove(innerlifetimetoken);
                    --state->pendingCompletions;
                }));

                auto selectedSource = state->coordinator.in(st);

                // this subscribe does not share the source subscription
                // so that when it is unsubscribed the source will continue
                auto sinkInner = make_subscriber<collection_value_type>(
                    state->out,
                    state->inner_lifetime,
                // on_next
                    [state, st](collection_value_type ct) {
                        state->out.on_next(std::move(ct));
                    },
                // on_error
                    [state](std::exception_ptr e) {
                        state->out.on_error(e);
                    },
                //on_completed
                    [state](){
                        if (state->pendingCompletions == 1) {
                            state->out.on_completed();
                        }
                    }
                );

                auto selectedSinkInner = state->coordinator.out(sinkInner);
                ++state->pendingCompletions;
                selectedSource.subscribe(std::move(selectedSinkInner));
            },
        // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (--state->pendingCompletions == 0) {
                    state->out.on_completed();
                }
            }
        );

        auto selectedSink = on_exception(
            [&](){return state->coordinator.out(sink);},
            state->out);
        if (selectedSink.empty()) {
            return;
        }

        source->subscribe(std::move(selectedSink.get()));

    }
};

template<class Coordination>
class switch_on_next_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;
public:
    switch_on_next_factory(coordination_type sf)
        : coordination(std::move(sf))
    {
    }

    template<class Observable>
    auto operator()(Observable source)
        ->      observable<rxu::value_type_t<switch_on_next<rxu::value_type_t<Observable>, Observable, Coordination>>,  switch_on_next<rxu::value_type_t<Observable>, Observable, Coordination>> {
        return  observable<rxu::value_type_t<switch_on_next<rxu::value_type_t<Observable>, Observable, Coordination>>,  switch_on_next<rxu::value_type_t<Observable>, Observable, Coordination>>(
                                                                                                                        switch_on_next<rxu::value_type_t<Observable>, Observable, Coordination>(std::move(source), coordination));
    }
};

}

template<class Coordination>
auto switch_on_next(Coordination&& sf)
    ->      detail::switch_on_next_factory<Coordination> {
    return  detail::switch_on_next_factory<Coordination>(std::forward<Coordination>(sf));
}

}

}

#endif
