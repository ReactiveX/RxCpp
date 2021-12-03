// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-amb.hpp

    \brief For each item from only the first of the given observables deliver from the new observable that is returned, on the specified scheduler.

           There are 2 variants of the operator:
           - The source observable emits nested observables, one of the nested observables is selected.
           - The source observable and the arguments v0...vn are used to provide the observables to select from.

    \tparam Coordination  the type of the scheduler (optional).
    \tparam Value0        ... (optional).
    \tparam ValueN        types of source observables (optional).

    \param  cn  the scheduler to synchronize sources from different contexts (optional).
    \param  v0  ... (optional).
    \param  vn  source observables (optional).

    \return  Observable that emits the same sequence as whichever of the source observables first emitted an item or sent a termination notification.

    If scheduler is omitted, identity_current_thread is used.

    \sample
    \snippet amb.cpp threaded implicit amb sample
    \snippet output.txt threaded implicit amb sample

    \snippet amb.cpp implicit amb sample
    \snippet output.txt implicit amb sample

    \snippet amb.cpp amb sample
    \snippet output.txt amb sample

    \snippet amb.cpp threaded amb sample
    \snippet output.txt threaded amb sample
*/

#if !defined(RXCPP_OPERATORS_RX_AMB_HPP)
#define RXCPP_OPERATORS_RX_AMB_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {
    
template<class T, class Observable, class Coordination>
struct amb
    : public operator_base<rxu::value_type_t<T>>
{
    //static_assert(is_observable<Observable>::value, "amb requires an observable");
    //static_assert(is_observable<T>::value, "amb requires an observable that contains observables");

    typedef amb<T, Observable, Coordination> this_type;

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

    amb(const source_type& o, coordination_type sf)
        : initial(o.source_operator, std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

        struct amb_state_type
            : public std::enable_shared_from_this<amb_state_type>
            , public values
        {
            amb_state_type(values i, coordinator_type coor, output_type oarg)
                : values(i)
                , source(i.source_operator)
                , coordinator(std::move(coor))
                , out(std::move(oarg))
                , pendingObservables(0)
                , firstEmitted(false)
            {
            }
            observable<source_value_type, source_operator_type> source;
            coordinator_type coordinator;
            output_type out;
            int pendingObservables;
            bool firstEmitted;
            std::vector<composite_subscription> innerSubscriptions;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<amb_state_type>(initial, std::move(coordinator), std::move(scbr));

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

        // this subscribe does not share the observer subscription
        // so that when it is unsubscribed the observer can be called
        // until the inner subscriptions have finished
        auto sink = make_subscriber<source_value_type>(
            state->out,
            outercs,
        // on_next
            [state](source_value_type st) {

                if (state->firstEmitted)
                    return;

                composite_subscription innercs;

                state->innerSubscriptions.push_back(innercs);

                // when the out observer is unsubscribed all the
                // inner subscriptions are unsubscribed as well
                auto innercstoken = state->out.add(innercs);

                innercs.add(make_subscription([state, innercstoken](){
                    state->out.remove(innercstoken);
                }));

                auto selectedSource = state->coordinator.in(st);

                auto current_id = state->pendingObservables++;

                // this subscribe does not share the source subscription
                // so that when it is unsubscribed the source will continue
                auto sinkInner = make_subscriber<value_type>(
                    state->out,
                    innercs,
                // on_next
                    [state, st, current_id](auto&& ct) {
                        state->out.on_next(std::forward<decltype(ct)>(ct));
                        if (!state->firstEmitted) {
                            state->firstEmitted = true;
                            auto do_unsubscribe = [](composite_subscription cs) {
                                cs.unsubscribe();
                            };
                            std::for_each(state->innerSubscriptions.begin(), state->innerSubscriptions.begin() + current_id, do_unsubscribe);
                            std::for_each(state->innerSubscriptions.begin() + current_id + 1, state->innerSubscriptions.end(), do_unsubscribe);
                        }
                    },
                // on_error
                    [state](rxu::error_ptr e) {
                        state->out.on_error(e);
                    },
                //on_completed
                    [state](){
                        state->out.on_completed();
                    }
                );

                auto selectedSinkInner = state->coordinator.out(sinkInner);
                selectedSource.subscribe(std::move(selectedSinkInner));
            },
        // on_error
            [state](rxu::error_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (state->pendingObservables == 0) {
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

}

/*! @copydoc rx-amb.hpp
*/
template<class... AN>
auto amb(AN&&... an)
    ->     operator_factory<amb_tag, AN...> {
    return operator_factory<amb_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}
}
}
#endif
