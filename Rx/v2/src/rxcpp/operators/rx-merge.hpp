// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_MERGE_HPP)
#define RXCPP_OPERATORS_RX_MERGE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Coordination>
struct merge
    : public operator_base<rxu::value_type_t<rxu::decay_t<T>>>
{
    //static_assert(is_observable<Observable>::value, "merge requires an observable");
    //static_assert(is_observable<T>::value, "merge requires an observable that contains observables");

    typedef merge<T, Observable, Coordination> this_type;

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Observable> source_type;

    typedef typename source_type::source_operator_type source_operator_type;
    typedef typename source_value_type::value_type value_type;

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

    merge(const source_type& o, coordination_type sf)
        : initial(o.source_operator, std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

        struct merge_state_type
            : public std::enable_shared_from_this<merge_state_type>
            , public values
        {
            merge_state_type(values i, coordinator_type coor, output_type oarg)
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
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<merge_state_type>(initial, std::move(coordinator), std::move(scbr));

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
        auto sink = make_subscriber<source_value_type>(
            state->out,
            outercs,
        // on_next
            [state](source_value_type st) {

                composite_subscription innercs;

                // when the out observer is unsubscribed all the
                // inner subscriptions are unsubscribed as well
                auto innercstoken = state->out.add(innercs);

                innercs.add(make_subscription([state, innercstoken](){
                    state->out.remove(innercstoken);
                }));

                auto selectedSource = state->coordinator.in(st);

                ++state->pendingCompletions;
                // this subscribe does not share the source subscription
                // so that when it is unsubscribed the source will continue
                auto sinkInner = make_subscriber<value_type>(
                    state->out,
                    innercs,
                // on_next
                    [state, st](value_type ct) {
                        state->out.on_next(std::move(ct));
                    },
                // on_error
                    [state](std::exception_ptr e) {
                        state->out.on_error(e);
                    },
                //on_completed
                    [state](){
                        if (--state->pendingCompletions == 0) {
                            state->out.on_completed();
                        }
                    }
                );

                auto selectedSinkInner = state->coordinator.out(sinkInner);
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
class merge_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;
public:
    merge_factory(coordination_type sf)
        : coordination(std::move(sf))
    {
    }

    template<class Observable>
    auto operator()(Observable source)
        ->      observable<rxu::value_type_t<merge<rxu::value_type_t<Observable>, Observable, Coordination>>,   merge<rxu::value_type_t<Observable>, Observable, Coordination>> {
        return  observable<rxu::value_type_t<merge<rxu::value_type_t<Observable>, Observable, Coordination>>,   merge<rxu::value_type_t<Observable>, Observable, Coordination>>(
                                                                                                                merge<rxu::value_type_t<Observable>, Observable, Coordination>(std::move(source), coordination));
    }
};

}

inline auto merge()
    ->      detail::merge_factory<identity_one_worker> {
    return  detail::merge_factory<identity_one_worker>(identity_current_thread());
}

template<class Coordination>
auto merge(Coordination&& sf)
    ->      detail::merge_factory<Coordination> {
    return  detail::merge_factory<Coordination>(std::forward<Coordination>(sf));
}

}

}

#endif
