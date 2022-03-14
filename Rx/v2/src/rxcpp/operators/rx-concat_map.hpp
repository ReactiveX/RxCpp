// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-concat_map.hpp

    \brief For each item from this observable use the CollectionSelector to produce an observable and subscribe to that observable.
           For each item from all of the produced observables use the ResultSelector to produce a value to emit from the new observable that is returned.

    \tparam CollectionSelector  the type of the observable producing function. CollectionSelector must be a function with the signature: observable(concat_map::source_value_type)
    \tparam ResultSelector      the type of the aggregation function (optional). ResultSelector must be a function with the signature: concat_map::value_type(concat_map::source_value_type, concat_map::collection_value_type)
    \tparam Coordination        the type of the scheduler (optional).

    \param  s   a function that returns an observable for each item emitted by the source observable.
    \param  rs  a function that combines one item emitted by each of the source and collection observables and returns an item to be emitted by the resulting observable (optional).
    \param  cn  the scheduler to synchronize sources from different contexts. (optional).

    \return  Observable that emits the results of applying a function to a pair of values emitted by the source observable and the collection observable.

    Observables, produced by the CollectionSelector, are concatenated. There is another operator rxcpp::observable<T,SourceType>::flat_map that works similar but merges the observables.

    \sample
    \snippet concat_map.cpp concat_map sample
    \snippet output.txt concat_map sample

    \sample
    \snippet concat_map.cpp threaded concat_map sample
    \snippet output.txt threaded concat_map sample
*/

#if !defined(RXCPP_OPERATORS_RX_CONCATMAP_HPP)
#define RXCPP_OPERATORS_RX_CONCATMAP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct concat_map_invalid_arguments {};

template<class... AN>
struct concat_map_invalid : public rxo::operator_base<concat_map_invalid_arguments<AN...>> {
    using type = observable<concat_map_invalid_arguments<AN...>, concat_map_invalid<AN...>>;
};
template<class... AN>
using concat_map_invalid_t = typename concat_map_invalid<AN...>::type;

template<class Observable, class CollectionSelector, class ResultSelector, class Coordination>
struct concat_traits {
    using source_type = rxu::decay_t<Observable>;
    using collection_selector_type = rxu::decay_t<CollectionSelector>;
    using result_selector_type = rxu::decay_t<ResultSelector>;
    using coordination_type = rxu::decay_t<Coordination>;

    using source_value_type = typename source_type::value_type;

    struct tag_not_valid {};
    template<class CV, class CCS>
    static auto collection_check(int) -> decltype(std::declval<CCS>()(std::declval<CV>()));
    template<class CV, class CCS>
    static tag_not_valid collection_check(...);

    static_assert(!rxcpp::is_same_v<decltype(collection_check<source_value_type, collection_selector_type>(0)), tag_not_valid>, "concat_map CollectionSelector must be a function with the signature observable(concat_map::source_value_type)");

    using collection_type = decltype(std::declval<collection_selector_type>()((*(source_value_type *) nullptr)));

//#if _MSC_VER >= 1900
    static_assert(is_observable<collection_type>::value, "concat_map CollectionSelector must return an observable");
//#endif

    using collection_value_type = typename collection_type::value_type;

    template<class CV, class CCV, class CRS>
    static auto result_check(int) -> decltype((std::declval<CRS>())(std::declval<CV>(), std::declval<CCV>()));
    template<class CV, class CCV, class CRS>
    static tag_not_valid result_check(...);

    static_assert(!rxcpp::is_same_v<decltype(result_check<source_value_type, collection_value_type, result_selector_type>(0)), tag_not_valid>, "concat_map ResultSelector must be a function with the signature concat_map::value_type(concat_map::source_value_type, concat_map::collection_value_type)");

    using value_type = rxu::decay_t<decltype(std::declval<result_selector_type>()(std::declval<source_value_type>(), std::declval<collection_value_type>()))> ;
};

template<class Observable, class CollectionSelector, class ResultSelector, class Coordination>
struct concat_map
    : public operator_base<rxu::value_type_t<concat_traits<Observable, CollectionSelector, ResultSelector, Coordination>>>
{
    using this_type = concat_map<Observable, CollectionSelector, ResultSelector, Coordination>;
    using traits = concat_traits<Observable, CollectionSelector, ResultSelector, Coordination>;

    using source_type = typename traits::source_type;
    using collection_selector_type = typename traits::collection_selector_type;
    using result_selector_type = typename traits::result_selector_type;

    using source_value_type = typename traits::source_value_type;
    using collection_type = typename traits::collection_type;
    using collection_value_type = typename traits::collection_value_type;

    using coordination_type = typename traits::coordination_type;
    using coordinator_type = typename coordination_type::coordinator_type;

    struct values
    {
        values(source_type o, collection_selector_type s, result_selector_type rs, coordination_type sf)
            : source(std::move(o))
            , selectCollection(std::move(s))
            , selectResult(std::move(rs))
            , coordination(std::move(sf))
        {
        }
        source_type source;
        collection_selector_type selectCollection;
        result_selector_type selectResult;
        coordination_type coordination;
    private:
        values& operator=(const values&) RXCPP_DELETE;
    };
    values initial;

    concat_map(source_type o, collection_selector_type s, result_selector_type rs, coordination_type sf)
        : initial(std::move(o), std::move(s), std::move(rs), std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        using output_type = Subscriber;

        struct concat_map_state_type
            : public std::enable_shared_from_this<concat_map_state_type>
            , public values
        {
            concat_map_state_type(values i, coordinator_type coor, output_type oarg)
                : values(std::move(i))
                , sourceLifetime(composite_subscription::empty())
                , collectionLifetime(composite_subscription::empty())
                , coordinator(std::move(coor))
                , out(std::move(oarg))
            {
            }

            void subscribe_to(const source_value_type& st)
            {
                subscribe_to(std::make_shared<source_value_type>(st));
            }

            void subscribe_to(source_value_type&& st)
            {
                subscribe_to(std::make_shared<source_value_type>(std::move(st)));
            }

            void subscribe_to(std::shared_ptr<source_value_type>&& st)
            {
                auto state = this->shared_from_this();

                auto selectedCollection = on_exception(
                    [&](){return state->selectCollection(*st);},
                    state->out);
                if (selectedCollection.empty()) {
                    return;
                }

                collectionLifetime = composite_subscription();

                // when the out observer is unsubscribed all the
                // inner subscriptions are unsubscribed as well
                auto innercstoken = state->out.add(collectionLifetime);

                collectionLifetime.add(make_subscription([state, innercstoken](){
                    state->out.remove(innercstoken);
                }));

                auto selectedSource = on_exception(
                    [&](){return state->coordinator.in(selectedCollection.get());},
                    state->out);
                if (selectedSource.empty()) {
                    return;
                }

                // this subscribe does not share the source subscription
                // so that when it is unsubscribed the source will continue
                auto sinkInner = make_subscriber<collection_value_type>(
                    state->out,
                    collectionLifetime,
                // on_next
                    [state, st](collection_value_type ct) {
                        auto selectedResult = state->selectResult(*st, std::move(ct));
                        state->out.on_next(std::move(selectedResult));
                    },
                // on_error
                    [state](rxu::error_ptr e) {
                        state->out.on_error(e);
                    },
                //on_completed
                    [state](){
                        if (!state->selectedCollections.empty()) {
                            auto value = state->selectedCollections.front();
                            state->selectedCollections.pop_front();
                            state->collectionLifetime.unsubscribe();
                            state->subscribe_to(std::move(value));
                        } else if (!state->sourceLifetime.is_subscribed()) {
                            state->out.on_completed();
                        }
                    }
                );
                auto selectedSinkInner = on_exception(
                    [&](){return state->coordinator.out(sinkInner);},
                    state->out);
                if (selectedSinkInner.empty()) {
                    return;
                }
                selectedSource->subscribe(std::move(selectedSinkInner.get()));
            }
            composite_subscription sourceLifetime;
            composite_subscription collectionLifetime;
            std::deque<source_value_type> selectedCollections;
            coordinator_type coordinator;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<concat_map_state_type>(initial, std::move(coordinator), std::move(scbr));

        state->sourceLifetime = composite_subscription();

        // when the out observer is unsubscribed all the
        // inner subscriptions are unsubscribed as well
        state->out.add(state->sourceLifetime);

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
            state->sourceLifetime,
        // on_next
            [state](auto&& st) {
                if (state->collectionLifetime.is_subscribed()) {
                    state->selectedCollections.push_back(std::forward<decltype(st)>(st));
                } else if (state->selectedCollections.empty()) {
                    state->subscribe_to(std::forward<decltype(st)>(st));
                }
            },
        // on_error
            [state](rxu::error_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (!state->collectionLifetime.is_subscribed() && state->selectedCollections.empty()) {
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
private:
    concat_map& operator=(const concat_map&) RXCPP_DELETE;
};

}

/*! @copydoc rx-concat_map.hpp
*/
template<class... AN>
auto concat_map(AN&&... an)
->     operator_factory<concat_map_tag, AN...> {
    return operator_factory<concat_map_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

/*! @copydoc rx-concat_map.hpp
*/
template<class... AN>
auto concat_transform(AN&&... an)
->     operator_factory<concat_map_tag, AN...> {
    return operator_factory<concat_map_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<concat_map_tag>
{
    template<class Observable, class CollectionSelector,
        class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
        class SourceValue = rxu::value_type_t<Observable>,
        class CollectionType = rxu::callable_result_t<CollectionSelectorType, SourceValue>,
        class ResultSelectorType = rxu::detail::take_at<1>,
        class Enabled = rxu::enable_if_all_true_type_t<
            all_observables<Observable, CollectionType>>,
        class ConcatMap = rxo::detail::concat_map<rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, ResultSelectorType, identity_one_worker>,
        class CollectionValueType = rxu::value_type_t<CollectionType>,
        class Value = rxu::callable_result_t<ResultSelectorType, SourceValue, CollectionValueType>,
        class Result = observable<Value, ConcatMap>
    >
    static Result member(Observable&& o, CollectionSelector&& s) {
        return Result(ConcatMap(std::forward<Observable>(o), std::forward<CollectionSelector>(s), ResultSelectorType(), identity_current_thread()));
    }

    template<class Observable, class CollectionSelector, class Coordination,
        class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
        class SourceValue = rxu::value_type_t<Observable>,
        class CollectionType = rxu::callable_result_t<CollectionSelectorType, SourceValue>,
        class ResultSelectorType = rxu::detail::take_at<1>,
        class Enabled = rxu::enable_if_all_true_type_t<
            all_observables<Observable, CollectionType>,
            is_coordination<Coordination>>,
        class ConcatMap = rxo::detail::concat_map<rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, ResultSelectorType, rxu::decay_t<Coordination>>,
        class CollectionValueType = rxu::value_type_t<CollectionType>,
        class Value = rxu::callable_result_t<ResultSelectorType, SourceValue, CollectionValueType>,
        class Result = observable<Value, ConcatMap>
    >
    static Result member(Observable&& o, CollectionSelector&& s, Coordination&& cn) {
        return Result(ConcatMap(std::forward<Observable>(o), std::forward<CollectionSelector>(s), ResultSelectorType(), std::forward<Coordination>(cn)));
    }

    template<class Observable, class CollectionSelector, class ResultSelector,
        class IsCoordination = is_coordination<ResultSelector>,
        class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
        class SourceValue = rxu::value_type_t<Observable>,
        class CollectionType = rxu::callable_result_t<CollectionSelectorType, SourceValue>,
        class Enabled = rxu::enable_if_all_true_type_t<
            all_observables<Observable, CollectionType>,
            rxu::negation<IsCoordination>>,
        class ConcatMap = rxo::detail::concat_map<rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, rxu::decay_t<ResultSelector>, identity_one_worker>,
        class CollectionValueType = rxu::value_type_t<CollectionType>,
        class ResultSelectorType = rxu::decay_t<ResultSelector>,
        class Value = rxu::callable_result_t<ResultSelectorType, SourceValue, CollectionValueType>,
        class Result = observable<Value, ConcatMap>
    >
    static Result member(Observable&& o, CollectionSelector&& s, ResultSelector&& rs) {
        return Result(ConcatMap(std::forward<Observable>(o), std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_current_thread()));
    }

    template<class Observable, class CollectionSelector, class ResultSelector, class Coordination,
        class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
        class SourceValue = rxu::value_type_t<Observable>,
        class CollectionType = rxu::callable_result_t<CollectionSelectorType, SourceValue>,
        class Enabled = rxu::enable_if_all_true_type_t<
            all_observables<Observable, CollectionType>,
            is_coordination<Coordination>>,
        class ConcatMap = rxo::detail::concat_map<rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, rxu::decay_t<ResultSelector>, rxu::decay_t<Coordination>>,
        class CollectionValueType = rxu::value_type_t<CollectionType>,
        class ResultSelectorType = rxu::decay_t<ResultSelector>,
        class Value = rxu::callable_result_t<ResultSelectorType, SourceValue, CollectionValueType>,
        class Result = observable<Value, ConcatMap>
    >
    static Result member(Observable&& o, CollectionSelector&& s, ResultSelector&& rs, Coordination&& cn) {
        return Result(ConcatMap(std::forward<Observable>(o), std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), std::forward<Coordination>(cn)));
    }

    template<class... AN>
    static operators::detail::concat_map_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "concat_map takes (CollectionSelector, optional ResultSelector, optional Coordination)");
    }
};

}

#endif
