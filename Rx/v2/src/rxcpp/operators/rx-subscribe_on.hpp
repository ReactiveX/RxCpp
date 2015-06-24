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
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Coordination> coordination_type;
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
    private:
        subscribe_on_values& operator=(subscribe_on_values o) RXCPP_DELETE;
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
            subscribe_on_state_type(const subscribe_on_values& i, const output_type& oarg)
                : subscribe_on_values(i)
                , out(oarg)
            {
            }
            composite_subscription source_lifetime;
            output_type out;
        private:
            subscribe_on_state_type& operator=(subscribe_on_state_type o) RXCPP_DELETE;
        };

        composite_subscription coordinator_lifetime;

        auto coordinator = initial.coordination.create_coordinator(coordinator_lifetime);

        auto controller = coordinator.get_worker();

        // take a copy of the values for each subscription
        auto state = std::make_shared<subscribe_on_state_type>(initial, std::move(s));

        auto sl = state->source_lifetime;
        auto ol = state->out.get_subscription();

        auto disposer = [=](const rxsc::schedulable&){
            sl.unsubscribe();
            ol.unsubscribe();
            coordinator_lifetime.unsubscribe();
        };
        auto selectedDisposer = on_exception(
            [&](){return coordinator.act(disposer);},
            state->out);
        if (selectedDisposer.empty()) {
            return;
        }
        
        state->source_lifetime.add([=](){
            controller.schedule(selectedDisposer.get());
        });

        state->out.add([=](){
            sl.unsubscribe();
            ol.unsubscribe();
            coordinator_lifetime.unsubscribe();
        });

        auto producer = [=](const rxsc::schedulable&){
            state->source.subscribe(state->source_lifetime, state->out);
        };

        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            state->out);
        if (selectedProducer.empty()) {
            return;
        }

        controller.schedule(selectedProducer.get());
    }
private:
    subscribe_on& operator=(subscribe_on o) RXCPP_DELETE;
};

template<class Coordination>
class subscribe_on_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;
public:
    subscribe_on_factory(coordination_type sf)
        : coordination(std::move(sf))
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   subscribe_on<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Coordination>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   subscribe_on<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Coordination>>(
                                                                          subscribe_on<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Coordination>(std::forward<Observable>(source), coordination));
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
