// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-combine_latest.hpp

    \brief For each item from all of the observables select a value to emit from the new observable that is returned.

    \tparam AN  types of scheduler (optional), aggregate function (optional), and source observables

    \param  an  scheduler (optional), aggregation function (optional), and source observables

    \return  Observable that emits items that are the result of combining the items emitted by the source observables.

    If scheduler is omitted, identity_current_thread is used.

    If aggregation function is omitted, the resulting observable returns tuples of emitted items.

    \sample

    Neither scheduler nor aggregation function are present:
    \snippet combine_latest.cpp combine_latest sample
    \snippet output.txt combine_latest sample

    Only scheduler is present:
    \snippet combine_latest.cpp Coordination combine_latest sample
    \snippet output.txt Coordination combine_latest sample

    Only aggregation function is present:
    \snippet combine_latest.cpp Selector combine_latest sample
    \snippet output.txt Selector combine_latest sample

    Both scheduler and aggregation function are present:
    \snippet combine_latest.cpp Coordination+Selector combine_latest sample
    \snippet output.txt Coordination+Selector combine_latest sample
*/

#if !defined(RXCPP_OPERATORS_RX_COMBINE_LATEST_HPP)
#define RXCPP_OPERATORS_RX_COMBINE_LATEST_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct combine_latest_invalid_arguments {};

template<class... AN>
struct combine_latest_invalid : public rxo::operator_base<combine_latest_invalid_arguments<AN...>> {
    using type = observable<combine_latest_invalid_arguments<AN...>, combine_latest_invalid<AN...>>;
};
template<class... AN>
using combine_latest_invalid_t = typename combine_latest_invalid<AN...>::type;

template<class Selector, class... ObservableN>
struct is_combine_latest_selector_check {
    using selector_type = rxu::decay_t<Selector>;

    struct tag_not_valid;
    template<class CS, class... CON>
    static auto check(int) -> decltype(std::declval<CS>()((std::declval<typename CON::value_type>())...));
    template<class CS, class... CON>
    static tag_not_valid check(...);

    using type = decltype(check<selector_type, rxu::decay_t<ObservableN>...>(0));

    static const bool value = !rxcpp::is_same_v<type, tag_not_valid>;
};

template<class Selector, class... ObservableN>
struct invalid_combine_latest_selector {
    static const bool value = false;
};

template<class Selector, class... ObservableN>
struct is_combine_latest_selector : public std::conditional_t<
    is_combine_latest_selector_check<Selector, ObservableN...>::value, 
    is_combine_latest_selector_check<Selector, ObservableN...>, 
    invalid_combine_latest_selector<Selector, ObservableN...>> {
};

template<class Selector, class... ON>
using result_combine_latest_selector_t = typename is_combine_latest_selector<Selector, ON...>::type;

template<class Coordination, class Selector, class... ObservableN>
struct combine_latest_traits {

    using tuple_source_type = std::tuple<ObservableN...>;
    using tuple_source_value_type = std::tuple<rxu::detail::maybe < typename ObservableN::value_type>...>;

    using selector_type = rxu::decay_t<Selector>;
    using coordination_type = rxu::decay_t<Coordination>;

    using value_type = typename is_combine_latest_selector<selector_type, ObservableN...>::type;
};

template<class Coordination, class Selector, class... ObservableN>
struct combine_latest : public operator_base<rxu::value_type_t<combine_latest_traits<Coordination, Selector, ObservableN...>>>
{
    using this_type = combine_latest<Coordination, Selector, ObservableN...>;

    using traits = combine_latest_traits<Coordination, Selector, ObservableN...>;

    using tuple_source_type = typename traits::tuple_source_type;
    using tuple_source_value_type = typename traits::tuple_source_value_type;

    using selector_type = typename traits::selector_type;

    using coordination_type = typename traits::coordination_type;
    using coordinator_type = typename coordination_type::coordinator_type;

    struct values
    {
        values(tuple_source_type o, selector_type s, coordination_type sf)
            : source(std::move(o))
            , selector(std::move(s))
            , coordination(std::move(sf))
        {
        }
        tuple_source_type source;
        selector_type selector;
        coordination_type coordination;
    };
    values initial;

    combine_latest(coordination_type sf, selector_type s, tuple_source_type ts)
        : initial(std::move(ts), std::move(s), std::move(sf))
    {
    }

    template<int Index, class State>
    void subscribe_one(std::shared_ptr<State> state) const {

        using source_value_type = typename std::tuple_element<Index, tuple_source_type>::type::value_type;

        composite_subscription innercs;

        // when the out observer is unsubscribed all the
        // inner subscriptions are unsubscribed as well
        state->out.add(innercs);

        auto source = on_exception(
            [&](){return state->coordinator.in(std::get<Index>(state->source));},
            state->out);
        if (source.empty()) {
            return;
        }

        // this subscribe does not share the observer subscription
        // so that when it is unsubscribed the observer can be called
        // until the inner subscriptions have finished
        auto sink = make_subscriber<source_value_type>(
            state->out,
            innercs,
        // on_next
            [state](auto&& st) {
                auto& value = std::get<Index>(state->latest);

                if (value.empty()) {
                    ++state->valuesSet;
                }

                value.reset(std::forward<decltype(st)>(st));

                if (state->valuesSet == sizeof... (ObservableN)) {
                    auto values = rxu::surely(state->latest);
                    state->out.on_next(rxu::apply(std::move(values), state->selector));
                }
            },
        // on_error
            [state](rxu::error_ptr e) {
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
    template<class State, int... IndexN>
    void subscribe_all(std::shared_ptr<State> state, rxu::values<int, IndexN...>) const {
        bool subscribed[] = {(subscribe_one<IndexN>(state), true)...};
        subscribed[0] = (*subscribed); // silence warning
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        using output_type = Subscriber;

        struct combine_latest_state_type
            : public std::enable_shared_from_this<combine_latest_state_type>
            , public values
        {
            combine_latest_state_type(values i, coordinator_type coor, output_type oarg)
                : values(std::move(i))
                , pendingCompletions(sizeof... (ObservableN))
                , valuesSet(0)
                , coordinator(std::move(coor))
                , out(std::move(oarg))
            {
            }

            // on_completed on the output must wait until all the
            // subscriptions have received on_completed
            mutable int pendingCompletions;
            mutable int valuesSet;
            mutable tuple_source_value_type latest;
            coordinator_type coordinator;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<combine_latest_state_type>(initial, std::move(coordinator), std::move(scbr));

        subscribe_all(state, typename rxu::values_from<int, sizeof...(ObservableN)>::type());
    }
};

}

/*! @copydoc rx-combine_latest.hpp
*/
template<class... AN>
auto combine_latest(AN&&... an) 
    ->     operator_factory<combine_latest_tag, AN...> {
    return operator_factory<combine_latest_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<> 
struct member_overload<combine_latest_tag>
{
    template<class Observable, class... ObservableN, 
        class Enabled = rxu::enable_if_all_true_type_t<
            all_observables<Observable, ObservableN...>>,
        class combine_latest = rxo::detail::combine_latest<identity_one_worker, rxu::detail::pack, rxu::decay_t<Observable>, rxu::decay_t<ObservableN>...>,
        class Value = rxu::value_type_t<combine_latest>,
        class Result = observable<Value, combine_latest>>
    static Result member(Observable&& o, ObservableN&&... on)
    {
        return Result(combine_latest(identity_current_thread(), rxu::pack(), std::make_tuple(std::forward<Observable>(o), std::forward<ObservableN>(on)...)));
    }

    template<class Observable, class Selector, class... ObservableN,
        class Enabled = rxu::enable_if_all_true_type_t<
            operators::detail::is_combine_latest_selector<Selector, Observable, ObservableN...>,
            all_observables<Observable, ObservableN...>>,
        class ResolvedSelector = rxu::decay_t<Selector>,
        class combine_latest = rxo::detail::combine_latest<identity_one_worker, ResolvedSelector, rxu::decay_t<Observable>, rxu::decay_t<ObservableN>...>,
        class Value = rxu::value_type_t<combine_latest>,
        class Result = observable<Value, combine_latest>>
    static Result member(Observable&& o, Selector&& s, ObservableN&&... on)
    {
        return Result(combine_latest(identity_current_thread(), std::forward<Selector>(s), std::make_tuple(std::forward<Observable>(o), std::forward<ObservableN>(on)...)));
    }

    template<class Coordination, class Observable, class... ObservableN, 
        class Enabled = rxu::enable_if_all_true_type_t<
            is_coordination<Coordination>,
            all_observables<Observable, ObservableN...>>,
        class combine_latest = rxo::detail::combine_latest<Coordination, rxu::detail::pack, rxu::decay_t<Observable>, rxu::decay_t<ObservableN>...>,
        class Value = rxu::value_type_t<combine_latest>,
        class Result = observable<Value, combine_latest>>
    static Result member(Observable&& o, Coordination&& cn, ObservableN&&... on)
    {
        return Result(combine_latest(std::forward<Coordination>(cn), rxu::pack(), std::make_tuple(std::forward<Observable>(o), std::forward<ObservableN>(on)...)));
    }

    template<class Coordination, class Selector, class Observable, class... ObservableN,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_coordination<Coordination>,
            operators::detail::is_combine_latest_selector<Selector, Observable, ObservableN...>,
            all_observables<Observable, ObservableN...>>,
        class ResolvedSelector = rxu::decay_t<Selector>,
        class combine_latest = rxo::detail::combine_latest<Coordination, ResolvedSelector, rxu::decay_t<Observable>, rxu::decay_t<ObservableN>...>,
        class Value = rxu::value_type_t<combine_latest>,
        class Result = observable<Value, combine_latest>>
    static Result member(Observable&& o, Coordination&& cn, Selector&& s, ObservableN&&... on)
    {
        return Result(combine_latest(std::forward<Coordination>(cn), std::forward<Selector>(s), std::make_tuple(std::forward<Observable>(o), std::forward<ObservableN>(on)...)));
    }

    template<class... AN>
    static operators::detail::combine_latest_invalid_t<AN...> member(const AN&...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "combine_latest takes (optional Coordination, optional Selector, required Observable, optional Observable...), Selector takes (Observable::value_type...)");
    } 
};

}

#endif
