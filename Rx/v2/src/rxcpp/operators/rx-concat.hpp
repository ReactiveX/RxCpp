// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_CONCAT_HPP)
#define RXCPP_OPERATORS_RX_CONCAT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Coordination>
struct concat
    : public operator_base<rxu::value_type_t<rxu::decay_t<T>>>
{
    typedef concat<T, Observable, Coordination> this_type;

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    typedef typename coordination_type::coordinator_type coordinator_type;

    typedef typename source_type::source_operator_type source_operator_type;
    typedef source_value_type collection_type;
    typedef typename collection_type::value_type value_type;

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

    concat(const source_type& o, coordination_type sf)
        : initial(o.source_operator, std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

        struct concat_state_type
            : public std::enable_shared_from_this<concat_state_type>
            , public values
        {
            concat_state_type(values i, coordinator_type coor, output_type oarg)
                : values(i)
                , source(i.source_operator)
                , sourceLifetime(composite_subscription::empty())
                , collectionLifetime(composite_subscription::empty())
                , coordinator(std::move(coor))
                , out(std::move(oarg))
            {
            }

            void subscribe_to(collection_type st)
            {
                auto state = this->shared_from_this();

                collectionLifetime = composite_subscription();

                // when the out observer is unsubscribed all the
                // inner subscriptions are unsubscribed as well
                auto innercstoken = state->out.add(collectionLifetime);

                collectionLifetime.add(make_subscription([state, innercstoken](){
                    state->out.remove(innercstoken);
                }));

                auto selectedSource = on_exception(
                    [&](){return state->coordinator.in(std::move(st));},
                    state->out);
                if (selectedSource.empty()) {
                    return;
                }

                // this subscribe does not share the out subscription
                // so that when it is unsubscribed the out will continue
                auto sinkInner = make_subscriber<value_type>(
                    state->out,
                    collectionLifetime,
                // on_next
                    [state, st](value_type ct) {
                        state->out.on_next(ct);
                    },
                // on_error
                    [state](std::exception_ptr e) {
                        state->out.on_error(e);
                    },
                //on_completed
                    [state](){
                        if (!state->selectedCollections.empty()) {
                            auto value = state->selectedCollections.front();
                            state->selectedCollections.pop_front();
                            state->collectionLifetime.unsubscribe();
                            state->subscribe_to(value);
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
            observable<source_value_type, source_operator_type> source;
            composite_subscription sourceLifetime;
            composite_subscription collectionLifetime;
            std::deque<collection_type> selectedCollections;
            coordinator_type coordinator;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<concat_state_type>(initial, std::move(coordinator), std::move(scbr));

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
        auto sink = make_subscriber<collection_type>(
            state->out,
            state->sourceLifetime,
        // on_next
            [state](collection_type st) {
                if (state->collectionLifetime.is_subscribed()) {
                    state->selectedCollections.push_back(st);
                } else if (state->selectedCollections.empty()) {
                    state->subscribe_to(st);
                }
            },
        // on_error
            [state](std::exception_ptr e) {
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
};

template<class Coordination>
class concat_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;
public:
    concat_factory(coordination_type sf)
        : coordination(std::move(sf))
    {
    }

    template<class Observable>
    auto operator()(Observable source)
        ->      observable<rxu::value_type_t<concat<rxu::value_type_t<Observable>, Observable, Coordination>>,  concat<rxu::value_type_t<Observable>, Observable, Coordination>> {
        return  observable<rxu::value_type_t<concat<rxu::value_type_t<Observable>, Observable, Coordination>>,  concat<rxu::value_type_t<Observable>, Observable, Coordination>>(
                                                                                                                concat<rxu::value_type_t<Observable>, Observable, Coordination>(std::move(source), coordination));
    }
};

}

template<class Coordination>
auto concat(Coordination&& sf)
    ->      detail::concat_factory<Coordination> {
    return  detail::concat_factory<Coordination>(std::forward<Coordination>(sf));
}

}

}

#endif
