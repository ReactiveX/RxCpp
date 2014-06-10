// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_CONCAT_HPP)
#define RXCPP_OPERATORS_RX_CONCAT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Observable, class Coordination>
struct concat
    : public operator_base<typename std::decay<Observable>::type::value_type::value_type>
{
    typedef concat<Observable, Coordination> this_type;

    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Coordination>::type coordination_type;

    typedef typename coordination_type::coordinator_type coordinator_type;

    typedef typename source_type::value_type collection_type;
    typedef typename collection_type::value_type value_type;

    struct values
    {
        values(source_type o, coordination_type sf)
            : source(std::move(o))
            , coordination(std::move(sf))
        {
        }
        source_type source;
        coordination_type coordination;
    };
    values initial;

    concat(source_type o, coordination_type sf)
        : initial(std::move(o), std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef typename coordinator_type::template get<Subscriber>::type output_type;

        struct concat_state_type
            : public std::enable_shared_from_this<concat_state_type>
            , public values
        {
            concat_state_type(values i, coordinator_type coor, output_type oarg)
                : values(std::move(i))
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
                selectedSource->subscribe(
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
            }
            composite_subscription sourceLifetime;
            composite_subscription collectionLifetime;
            std::deque<collection_type> selectedCollections;
            coordinator_type coordinator;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());
        auto selectedDest = on_exception(
            [&](){return coordinator.out(scbr);},
            scbr);
        if (selectedDest.empty()) {
            return;
        }

        // take a copy of the values for each subscription
        auto state = std::shared_ptr<concat_state_type>(new concat_state_type(initial, std::move(coordinator), std::move(selectedDest.get())));

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
        source->subscribe(
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
    }
};

template<class Coordination>
class concat_factory
{
    typedef typename std::decay<Coordination>::type coordination_type;

    coordination_type coordination;
public:
    concat_factory(coordination_type sf)
        : coordination(std::move(sf))
    {
    }

    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename concat<Observable, Coordination>::value_type, concat<Observable, Coordination>> {
        return  observable<typename concat<Observable, Coordination>::value_type, concat<Observable, Coordination>>(
                                    concat<Observable, Coordination>(std::forward<Observable>(source), coordination));
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
