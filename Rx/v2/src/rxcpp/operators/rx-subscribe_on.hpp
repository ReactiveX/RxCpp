// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SUBSCRIBE_ON_HPP)
#define RXCPP_OPERATORS_RX_SUBSCRIBE_ON_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Coordination>
struct subscribe_on : public operator_base<T>
{
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    struct subscribe_on_values
    {
        ~subscribe_on_values()
        {
        }
        subscribe_on_values(source_type s, coordination_type sf)
            : source(std::move(s))
            , coordination(std::move(sf))
        {
        }
        source_type source;
        coordination_type coordination;
    };
    const subscribe_on_values initial;

    ~subscribe_on()
    {
    }
    subscribe_on(source_type s, coordination_type sf)
        : initial(std::move(s), std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber s) const {

        typedef Subscriber output_type;
        struct subscribe_on_state_type
            : public std::enable_shared_from_this<subscribe_on_state_type>
            , public subscribe_on_values
        {
            subscribe_on_state_type(const subscribe_on_values& i, coordinator_type coor, const output_type& oarg)
                : subscribe_on_values(i)
                , coordinator(std::move(coor))
                , out(oarg)
            {
            }
            composite_subscription source_lifetime;
            coordinator_type coordinator;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(s.get_subscription());

        auto controller = coordinator.get_worker();

        // take a copy of the values for each subscription
        auto state = std::shared_ptr<subscribe_on_state_type>(new subscribe_on_state_type(initial, std::move(coordinator), std::move(s)));

        auto disposer = [=](const rxsc::schedulable&){
            state->source_lifetime.unsubscribe();
            state->out.unsubscribe();
        };
        auto selectedDisposer = on_exception(
            [&](){return state->coordinator.act(disposer);},
            state->out);
        if (selectedDisposer.empty()) {
            return;
        }

        state->out.add([=](){
            controller.schedule(selectedDisposer.get());
        });
        state->source_lifetime.add([=](){
            controller.schedule(selectedDisposer.get());
        });

        auto producer = [=](const rxsc::schedulable&){
            state->source.subscribe(state->source_lifetime, state->out);
        };

        auto selectedProducer = on_exception(
            [&](){return state->coordinator.act(producer);},
            state->out);
        if (selectedProducer.empty()) {
            return;
        }

        controller.schedule(selectedProducer.get());
    }
};

template<class Coordination>
class subscribe_on_factory
{
    typedef typename std::decay<Coordination>::type coordination_type;

    coordination_type coordination;
public:
    subscribe_on_factory(coordination_type sf)
        : coordination(std::move(sf))
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   subscribe_on<typename std::decay<Observable>::type::value_type, Observable, Coordination>> {
        return  observable<typename std::decay<Observable>::type::value_type,   subscribe_on<typename std::decay<Observable>::type::value_type, Observable, Coordination>>(
                                                                                subscribe_on<typename std::decay<Observable>::type::value_type, Observable, Coordination>(std::forward<Observable>(source), coordination));
    }
};

}

template<class Coordination>
auto subscribe_on(Coordination sf)
    ->      detail::subscribe_on_factory<Coordination> {
    return  detail::subscribe_on_factory<Coordination>(std::move(sf));
}

}

}

#endif
