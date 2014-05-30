// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_CONCAT_HPP)
#define RXCPP_OPERATORS_RX_CONCAT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Observable, class SourceFilter>
struct concat
    : public operator_base<typename std::decay<Observable>::type::value_type::value_type>
{
    typedef concat<Observable, SourceFilter> this_type;

    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<SourceFilter>::type source_filter_type;

    typedef typename source_type::value_type collection_type;
    typedef typename collection_type::value_type value_type;

    struct values
    {
        values(source_type o, source_filter_type sf)
            : source(std::move(o))
            , sourceFilter(std::move(sf))
        {
        }
        source_type source;
        source_filter_type sourceFilter;
    };
    values initial;

    concat(source_type o, source_filter_type sf)
        : initial(std::move(o), std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber&& scbr) {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef typename std::decay<Subscriber>::type output_type;

        struct concat_state_type
            : public std::enable_shared_from_this<concat_state_type>
            , public values
        {
            concat_state_type(values i, output_type oarg)
                : values(std::move(i))
                , sourceLifetime(composite_subscription::empty())
                , collectionLifetime(composite_subscription::empty())
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
                    [&](){return state->sourceFilter(st);},
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
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::shared_ptr<concat_state_type>(new concat_state_type(initial, std::forward<Subscriber>(scbr)));

        state->sourceLifetime = composite_subscription();

        // when the out observer is unsubscribed all the
        // inner subscriptions are unsubscribed as well
        state->out.add(state->sourceLifetime);

        auto source = on_exception(
            [&](){return state->sourceFilter(state->source);},
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

template<class SourceFilter>
class concat_factory
{
    typedef typename std::decay<SourceFilter>::type source_filter_type;

    source_filter_type sourceFilter;
public:
    concat_factory(source_filter_type sf)
        : sourceFilter(std::move(sf))
    {
    }

    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename concat<Observable, SourceFilter>::value_type, concat<Observable, SourceFilter>> {
        return  observable<typename concat<Observable, SourceFilter>::value_type, concat<Observable, SourceFilter>>(
                                    concat<Observable, SourceFilter>(std::forward<Observable>(source), sourceFilter));
    }
};

}

template<class SourceFilter>
auto concat(SourceFilter&& sf)
    ->      detail::concat_factory<SourceFilter> {
    return  detail::concat_factory<SourceFilter>(std::forward<SourceFilter>(sf));
}

}

}

#endif
