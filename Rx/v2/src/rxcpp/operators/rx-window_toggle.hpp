// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_WINDOW_TOGGLE_HPP)
#define RXCPP_OPERATORS_RX_WINDOW_TOGGLE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Openings, class ClosingSelector, class Coordination>
struct window_toggle_traits {

    using source_value_type = rxu::decay_t<T>;
    using coordination_type = rxu::decay_t<Coordination>;
    using openings_type = rxu::decay_t<Openings>;
    using openings_value_type = typename openings_type::value_type;
    using closing_selector_type =  rxu::decay_t<ClosingSelector>;

    static_assert(is_observable<openings_type>::value, "window_toggle Openings must be an observable");

    struct tag_not_valid {};
    template<class CS, class CV>
    static auto check(int) -> decltype((*(CS*)nullptr)((*(CV*)nullptr)));
    template<class CS, class CV>
    static tag_not_valid check(...);

    static_assert(is_observable<decltype(check<closing_selector_type, openings_value_type>(0))>::value, "window_toggle ClosingSelector must be a function with the signature observable<U>(Openings::value_type)");

    using closings_type = rxu::decay_t<decltype(check<closing_selector_type, openings_value_type>(0))>;
    using closings_value_type = typename closings_type::value_type;
};

template<class T, class Openings, class ClosingSelector, class Coordination>
struct window_toggle
{
    typedef window_toggle<T, Openings, ClosingSelector, Coordination> this_type;

    typedef window_toggle_traits<T, Openings, ClosingSelector, Coordination> traits;

    using source_value_type = typename traits::source_value_type;
    using coordination_type = typename traits::coordination_type;
    using coordinator_type = typename coordination_type::coordinator_type;
    using openings_type = typename traits::openings_type;
    using openings_value_type = typename traits::openings_value_type;
    using closing_selector_type = typename traits::closing_selector_type;
    using closings_value_type = typename traits::closings_value_type;

    struct window_toggle_values
    {
        window_toggle_values(openings_type opens, closing_selector_type closes, coordination_type c)
            : openings(opens)
            , closingSelector(closes)
            , coordination(c)
        {
        }
        openings_type openings;
        mutable closing_selector_type closingSelector;
        coordination_type coordination;
    };
    window_toggle_values initial;

    window_toggle(openings_type opens, closing_selector_type closes, coordination_type coordination)
        : initial(opens, closes, coordination)
    {
    }

    template<class Subscriber>
    struct window_toggle_observer
    {
        typedef window_toggle_observer<Subscriber> this_type;
        typedef rxu::decay_t<T> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;

        struct window_toggle_subscriber_values : public window_toggle_values
        {
            window_toggle_subscriber_values(composite_subscription cs, dest_type d, window_toggle_values v, coordinator_type c)
                : window_toggle_values(v)
                , cs(std::move(cs))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(coordinator.get_worker())
            {
            }
            composite_subscription cs;
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable std::list<rxcpp::subjects::subject<T>> subj;
        };
        std::shared_ptr<window_toggle_subscriber_values> state;

        window_toggle_observer(composite_subscription cs, dest_type d, window_toggle_values v, coordinator_type c)
            : state(std::make_shared<window_toggle_subscriber_values>(window_toggle_subscriber_values(std::move(cs), std::move(d), v, std::move(c))))
        {
            auto localState = state;

            composite_subscription innercs;

            // when the out observer is unsubscribed all the
            // inner subscriptions are unsubscribed as well
            auto innerscope = localState->dest.add(innercs);

            innercs.add([=](){
                localState->dest.remove(innerscope);
            });

            auto source = on_exception(
                [&](){return localState->coordinator.in(localState->openings);},
                localState->dest);
            if (source.empty()) {
                return;
            }

            // this subscribe does not share the observer subscription
            // so that when it is unsubscribed the observer can be called
            // until the inner subscriptions have finished
            auto sink = make_subscriber<openings_value_type>(
                localState->dest,
                innercs,
            // on_next
                [localState](const openings_value_type& ov) {
                    auto closer = localState->closingSelector(ov);

                    auto it = localState->subj.insert(localState->subj.end(), rxcpp::subjects::subject<T>());
                    localState->dest.on_next(it->get_observable().as_dynamic());

                    composite_subscription innercs;

                    // when the out observer is unsubscribed all the
                    // inner subscriptions are unsubscribed as well
                    auto innerscope = localState->dest.add(innercs);

                    innercs.add([=](){
                        localState->dest.remove(innerscope);
                    });

                    auto source = localState->coordinator.in(closer);

                    auto sit = std::make_shared<decltype(it)>(it);
                    auto close = [localState, sit]() {
                        auto it = *sit;
                        *sit = localState->subj.end();
                        if (it != localState->subj.end()) {
                            it->get_subscriber().on_completed();
                            localState->subj.erase(it);
                        }
                    };

                    // this subscribe does not share the observer subscription
                    // so that when it is unsubscribed the observer can be called
                    // until the inner subscriptions have finished
                    auto sink = make_subscriber<closings_value_type>(
                        localState->dest,
                        innercs,
                    // on_next
                        [close, innercs](closings_value_type) {
                            close();
                            innercs.unsubscribe();
                        },
                    // on_error
                        [localState](std::exception_ptr e) {
                            localState->dest.on_error(e);
                        },
                    // on_completed
                        close
                    );
                    auto selectedSink = localState->coordinator.out(sink);
                    source.subscribe(std::move(selectedSink));
                },
            // on_error
                [localState](std::exception_ptr e) {
                    localState->dest.on_error(e);
                },
            // on_completed
                []() {
                }
            );
            auto selectedSink = on_exception(
                [&](){return localState->coordinator.out(sink);},
                localState->dest);
            if (selectedSink.empty()) {
                return;
            }
            source->subscribe(std::move(selectedSink.get()));
        }

        void on_next(T v) const {
            auto localState = state;
            auto work = [v, localState](const rxsc::schedulable&){
                for (auto s : localState->subj) {
                    s.get_subscriber().on_next(v);
                }
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        void on_error(std::exception_ptr e) const {
            auto localState = state;
            auto work = [e, localState](const rxsc::schedulable&){
                for (auto s : localState->subj) {
                    s.get_subscriber().on_error(e);
                }
                localState->dest.on_error(e);
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        void on_completed() const {
            auto localState = state;
            auto work = [localState](const rxsc::schedulable&){
                for (auto s : localState->subj) {
                    s.get_subscriber().on_completed();
                }
                localState->dest.on_completed();
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        static subscriber<T, observer_type> make(dest_type d, window_toggle_values v) {
            auto cs = composite_subscription();
            auto coordinator = v.coordination.create_coordinator();

            return make_subscriber<T>(cs, observer_type(this_type(cs, std::move(d), std::move(v), std::move(coordinator))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(window_toggle_observer<Subscriber>::make(std::move(dest), initial)) {
        return      window_toggle_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Openings, class ClosingSelector, class Coordination>
class window_toggle_factory
{
    typedef rxu::decay_t<Openings> openings_type;
    typedef rxu::decay_t<ClosingSelector> closing_selector_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    openings_type openings;
    closing_selector_type closingSelector;
    coordination_type coordination;
public:
    window_toggle_factory(openings_type opens, closing_selector_type closes, coordination_type c) : openings(opens), closingSelector(closes), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<observable<rxu::value_type_t<rxu::decay_t<Observable>>>>(window_toggle<rxu::value_type_t<rxu::decay_t<Observable>>, Openings, ClosingSelector, Coordination>(openings, closingSelector, coordination))) {
        return      source.template lift<observable<rxu::value_type_t<rxu::decay_t<Observable>>>>(window_toggle<rxu::value_type_t<rxu::decay_t<Observable>>, Openings, ClosingSelector, Coordination>(openings, closingSelector, coordination));
    }
};

}

template<class Openings, class ClosingSelector, class Coordination>
inline auto window_toggle(Openings openings, ClosingSelector closingSelector, Coordination coordination)
    ->      detail::window_toggle_factory<Openings, ClosingSelector, Coordination> {
    return  detail::window_toggle_factory<Openings, ClosingSelector, Coordination>(openings, closingSelector, coordination);
}

template<class Openings, class ClosingSelector>
inline auto window_toggle(Openings openings, ClosingSelector closingSelector)
    ->      detail::window_toggle_factory<Openings, ClosingSelector, identity_one_worker> {
    return  detail::window_toggle_factory<Openings, ClosingSelector, identity_one_worker>(openings, closingSelector, identity_immediate());
}

}

}

#endif
