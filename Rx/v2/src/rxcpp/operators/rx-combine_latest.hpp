// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_COMBINE_LATEST_HPP)
#define RXCPP_OPERATORS_RX_COMBINE_LATEST_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Coordination, class Selector, class... ObservableN>
struct combine_latest_traits {

    typedef std::tuple<ObservableN...> tuple_source_type;
    typedef std::tuple<rxu::detail::maybe<typename ObservableN::value_type>...> tuple_source_value_type;

    typedef rxu::decay_t<Selector> selector_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    struct tag_not_valid {};
    template<class CS, class... CVN>
    static auto check(int) -> decltype((*(CS*)nullptr)((*(CVN*)nullptr)...));
    template<class CS, class... CVN>
    static tag_not_valid check(...);

    static_assert(!std::is_same<decltype(check<selector_type, typename ObservableN::value_type...>(0)), tag_not_valid>::value, "combine_latest Selector must be a function with the signature value_type(Observable::value_type...)");

    typedef decltype(check<selector_type, typename ObservableN::value_type...>(0)) value_type;
};

template<class Coordination, class Selector, class... ObservableN>
struct combine_latest : public operator_base<rxu::value_type_t<combine_latest_traits<Coordination, Selector, ObservableN...>>>
{
    typedef combine_latest<Coordination, Selector, ObservableN...> this_type;

    typedef combine_latest_traits<Coordination, Selector, ObservableN...> traits;

    typedef typename traits::tuple_source_type tuple_source_type;
    typedef typename traits::tuple_source_value_type tuple_source_value_type;

    typedef typename traits::selector_type selector_type;

    typedef typename traits::coordination_type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

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

        typedef typename std::tuple_element<Index, tuple_source_type>::type::value_type source_value_type;

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
            [state](source_value_type st) {
                auto& value = std::get<Index>(state->latest);

                if (value.empty()) {
                    ++state->valuesSet;
                }

                value.reset(st);

                if (state->valuesSet == sizeof... (ObservableN)) {
                    auto values = rxu::surely(state->latest);
                    auto selectedResult = rxu::apply(values, state->selector);
                    state->out.on_next(selectedResult);
                }
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
    template<class State, int... IndexN>
    void subscribe_all(std::shared_ptr<State> state, rxu::values<int, IndexN...>) const {
        bool subscribed[] = {(subscribe_one<IndexN>(state), true)...};
        subscribed[0] = (*subscribed); // silence warning
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

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

template<class Coordination, class Selector, class... ObservableN>
class combine_latest_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef rxu::decay_t<Selector> selector_type;
    typedef std::tuple<ObservableN...> tuple_source_type;

    coordination_type coordination;
    selector_type selector;
    tuple_source_type sourcen;

    template<class... YObservableN>
    auto make(std::tuple<YObservableN...> source)
        ->      observable<rxu::value_type_t<combine_latest<Coordination, Selector, YObservableN...>>, combine_latest<Coordination, Selector, YObservableN...>> {
        return  observable<rxu::value_type_t<combine_latest<Coordination, Selector, YObservableN...>>, combine_latest<Coordination, Selector, YObservableN...>>(
                                             combine_latest<Coordination, Selector, YObservableN...>(coordination, selector, std::move(source)));
    }
public:
    combine_latest_factory(coordination_type sf, selector_type s, ObservableN... on)
        : coordination(std::move(sf))
        , selector(std::move(s))
        , sourcen(std::make_tuple(std::move(on)...))
    {
    }

    template<class Observable>
    auto operator()(Observable source)
        -> decltype(make(std::tuple_cat(std::make_tuple(source), *(tuple_source_type*)nullptr))) {
        return      make(std::tuple_cat(std::make_tuple(source), sourcen));
    }
};

}

template<class Coordination, class Selector, class... ObservableN>
auto combine_latest(Coordination sf, Selector s, ObservableN... on)
    ->      detail::combine_latest_factory<Coordination, Selector, ObservableN...> {
    return  detail::combine_latest_factory<Coordination, Selector, ObservableN...>(std::move(sf), std::move(s), std::move(on)...);
}

}

}

#endif
