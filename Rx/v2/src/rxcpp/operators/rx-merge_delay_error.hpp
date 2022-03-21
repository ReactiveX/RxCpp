// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-merge_delay_error.hpp

        \brief For each given observable subscribe.
                   For each item emitted from all of the given observables, deliver from the new observable that is returned.
                   The first error to occure is hold off until all of the given non-error-emitting observables have finished their emission.

                   There are 2 variants of the operator:
                   - The source observable emits nested observables, nested observables are merged.
                   - The source observable and the arguments v0...vn are used to provide the observables to merge.

        \tparam Coordination  the type of the scheduler (optional).
        \tparam Value0  ... (optional).
        \tparam ValueN  types of source observables (optional).

        \param  cn      the scheduler to synchronize sources from different contexts (optional).
        \param  v0      ... (optional).
        \param  vn      source observables (optional).

        \return                                                                                                              Observable that emits items that are the result of flattening the observables emitted by the source observable.

        If scheduler is omitted, identity_current_thread is used.

        \sample
        \snippet merge_delay_error.cpp threaded implicit merge sample
        \snippet output.txt threaded implicit merge sample

        \sample
        \snippet merge_delay_error.cpp implicit merge sample
        \snippet output.txt implicit merge sample

        \sample
        \snippet merge_delay_error.cpp merge sample
        \snippet output.txt merge sample

        \sample
        \snippet merge_delay_error.cpp threaded merge sample
        \snippet output.txt threaded merge sample
*/

#if !defined(RXCPP_OPERATORS_RX_MERGE_DELAY_ERROR_HPP)
#define RXCPP_OPERATORS_RX_MERGE_DELAY_ERROR_HPP

#include "rx-merge.hpp"

#include "../rx-composite_exception.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Coordination>
struct merge_delay_error
        : public operator_base<rxu::value_type_t<rxu::decay_t<T>>>
{
        //static_assert(is_observable<Observable>::value, "merge requires an observable");
        //static_assert(is_observable<T>::value, "merge requires an observable that contains observables");

    using this_type = merge_delay_error<T, Observable, Coordination>;

    using source_value_type = rxu::decay_t<T>;
    using source_type = rxu::decay_t<Observable>;

    using source_operator_type = typename source_type::source_operator_type;
    using value_type = typename source_value_type::value_type;

    using coordination_type = rxu::decay_t<Coordination>;
    using coordinator_type = typename coordination_type::coordinator_type;

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

        merge_delay_error(const source_type& o, coordination_type sf)
                : initial(o.source_operator, std::move(sf))
        {
        }

        template<class Subscriber>
        void on_subscribe(Subscriber scbr) const {
                static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

            using output_type = Subscriber;

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
                        composite_exception exception;
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
                                        [state, st](auto&& ct) {
                                                state->out.on_next(std::forward<decltype(ct)>(ct));
                                        },
                                // on_error
                                        [state](rxu::error_ptr e) {
                                                if(--state->pendingCompletions == 0) {
                                                    state->out.on_error(
                                                        rxu::make_error_ptr(std::move(state->exception.add(e))));
                                                } else {
                                                        state->exception.add(e);
                                                }
                                        },
                                //on_completed
                                        [state](){
                                                if (--state->pendingCompletions == 0) {
                                                        if(!state->exception.empty()) {
                                                            state->out.on_error(
                                                                rxu::make_error_ptr(std::move(state->exception)));
                                                        } else {
                                                                state->out.on_completed();
                                                        }
                                                }
                                        }
                                );

                                auto selectedSinkInner = state->coordinator.out(sinkInner);
                                selectedSource.subscribe(std::move(selectedSinkInner));
                        },
                // on_error
                        [state](rxu::error_ptr e) {
                            if(--state->pendingCompletions == 0) {
                                state->out.on_error(
                                    rxu::make_error_ptr(std::move(state->exception.add(e))));
                            } else {
                                state->exception.add(e);
                            }
                        },
                // on_completed
                        [state]() {
                            if (--state->pendingCompletions == 0) {
                                if(!state->exception.empty()) {
                                    state->out.on_error(
                                        rxu::make_error_ptr(std::move(state->exception)));
                                } else {
                                    state->out.on_completed();
                                }
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

/*! @copydoc rx-merge-delay-error.hpp
*/
template<class... AN>
auto merge_delay_error(AN&&... an)
        ->         operator_factory<merge_delay_error_tag, AN...> {
        return operator_factory<merge_delay_error_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<merge_delay_error_tag>
{
        template<class Observable,
                class Enabled = rxu::enable_if_all_true_type_t<
                        is_observable<Observable>>,
                class SourceValue = rxu::value_type_t<Observable>,
                class Merge = rxo::detail::merge_delay_error<SourceValue, rxu::decay_t<Observable>, identity_one_worker>,
                class Value = rxu::value_type_t<SourceValue>,
                class Result = observable<Value, Merge>
        >
        static Result member(Observable&& o) {
                return Result(Merge(std::forward<Observable>(o), identity_current_thread()));
        }

        template<class Observable, class Coordination,
                class Enabled = rxu::enable_if_all_true_type_t<
                        is_observable<Observable>,
                        is_coordination<Coordination>>,
                class SourceValue = rxu::value_type_t<Observable>,
                class Merge = rxo::detail::merge_delay_error<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<Coordination>>,
                class Value = rxu::value_type_t<SourceValue>,
                class Result = observable<Value, Merge>
        >
        static Result member(Observable&& o, Coordination&& cn) {
                return Result(Merge(std::forward<Observable>(o), std::forward<Coordination>(cn)));
        }

        template<class Observable, class Value0, class... ValueN,
                class Enabled = rxu::enable_if_all_true_type_t<
                        all_observables<Observable, Value0, ValueN...>>,
                class EmittedValue = rxu::value_type_t<Observable>,
                class SourceValue = observable<EmittedValue>,
                class ObservableObservable = observable<SourceValue>,
                class Merge = typename rxu::defer_type<rxo::detail::merge_delay_error, SourceValue, ObservableObservable, identity_one_worker>::type,
                class Value = rxu::value_type_t<Merge>,
                class Result = observable<Value, Merge>
        >
        static Result member(Observable&& o, Value0&& v0, ValueN&&... vn) {
                return Result(Merge(rxs::from(o.as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread()));
        }

        template<class Observable, class Coordination, class Value0, class... ValueN,
                class Enabled = rxu::enable_if_all_true_type_t<
                        all_observables<Observable, Value0, ValueN...>,
                        is_coordination<Coordination>>,
                class EmittedValue = rxu::value_type_t<Observable>,
                class SourceValue = observable<EmittedValue>,
                class ObservableObservable = observable<SourceValue>,
                class Merge = typename rxu::defer_type<rxo::detail::merge_delay_error, SourceValue, ObservableObservable, rxu::decay_t<Coordination>>::type,
                class Value = rxu::value_type_t<Merge>,
                class Result = observable<Value, Merge>
        >
        static Result member(Observable&& o, Coordination&& cn, Value0&& v0, ValueN&&... vn) {
                return Result(Merge(rxs::from(o.as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::forward<Coordination>(cn)));
        }

        template<class... AN>
        static operators::detail::merge_invalid_t<AN...> member(AN...) {
                std::terminate();
                return {};
                static_assert(sizeof...(AN) == 10000, "merge_delay_error takes (optional Coordination, optional Value0, optional ValueN...)");
        }
};

}

#endif
