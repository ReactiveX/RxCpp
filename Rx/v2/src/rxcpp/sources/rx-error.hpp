// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_ERROR_HPP)
#define RXCPP_SOURCES_RX_ERROR_HPP

#include "../rx-includes.hpp"

/*! \file rx-error.hpp

    \brief Returns an observable that sends no items to observer and immediately generates an error, on the specified scheduler.

    \tparam T             the type of (not) emitted items
    \tparam Exception     the type of the error
    \tparam Coordination  the type of the scheduler (optional)

    \param  e   the error to be passed to observers
    \param  cn  the scheduler to use for scheduling the items (optional)

    \return  Observable that sends no items to observer and immediately generates an error.

    \sample
    \snippet error.cpp error sample
    \snippet output.txt error sample

    \sample
    \snippet error.cpp threaded error sample
    \snippet output.txt threaded error sample
*/

namespace rxcpp {

namespace sources {

namespace detail {

template<class T, class Coordination>
struct error : public source_base<T>
{
    using this_type = error<T, Coordination>;

    using coordination_type = rxu::decay_t<Coordination>;

    using coordinator_type = typename coordination_type::coordinator_type;

    struct error_initial_type
    {
        error_initial_type(rxu::error_ptr e, coordination_type cn)
            : exception(e)
            , coordination(std::move(cn))
        {
        }
        rxu::error_ptr exception;
        coordination_type coordination;
    };
    error_initial_type initial;

    error(rxu::error_ptr e, coordination_type cn)
        : initial(e, std::move(cn))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber o) const {

        // creates a worker whose lifetime is the same as this subscription
        auto coordinator = initial.coordination.create_coordinator(o.get_subscription());
        auto controller = coordinator.get_worker();
        auto exception = initial.exception;

        auto producer = [=](const rxsc::schedulable&){
            auto& dest = o;
            if (!dest.is_subscribed()) {
                // terminate loop
                return;
            }

            dest.on_error(exception);
            // o is unsubscribed
        };
        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            o);
        if (selectedProducer.empty()) {
            return;
        }
        controller.schedule(selectedProducer.get());
    }
};

struct throw_ptr_tag{};
struct throw_instance_tag{};

template <class T, class Coordination>
auto make_error(throw_ptr_tag&&, rxu::error_ptr exception, Coordination cn)
    ->      observable<T, error<T, Coordination>> {
    return  observable<T, error<T, Coordination>>(error<T, Coordination>(std::move(exception), std::move(cn)));
}

template <class T, class E, class Coordination>
auto make_error(throw_instance_tag&&, E e, Coordination cn)
    ->      observable<T, error<T, Coordination>> {
    rxu::error_ptr ep = rxu::make_error_ptr(e);
    return  observable<T, error<T, Coordination>>(error<T, Coordination>(std::move(ep), std::move(cn)));
}

}

}

namespace sources {

/*! @copydoc rx-error.hpp
 */
template<class T, class E>
auto error(E e)
    -> decltype(detail::make_error<T>(typename std::conditional_t<rxcpp::is_same_v<rxu::error_ptr, rxu::decay_t<E>>, detail::throw_ptr_tag, detail::throw_instance_tag>(), std::move(e), identity_immediate())) {
    return      detail::make_error<T>(typename std::conditional_t<rxcpp::is_same_v<rxu::error_ptr, rxu::decay_t<E>>, detail::throw_ptr_tag, detail::throw_instance_tag>(), std::move(e), identity_immediate());
}
/*! @copydoc rx-error.hpp
 */
template<class T, class E, class Coordination>
auto error(E e, Coordination cn)
    -> decltype(detail::make_error<T>(typename std::conditional_t<rxcpp::is_same_v<rxu::error_ptr, rxu::decay_t<E>>, detail::throw_ptr_tag, detail::throw_instance_tag>(), std::move(e), std::move(cn))) {
    return      detail::make_error<T>(typename std::conditional_t<rxcpp::is_same_v<rxu::error_ptr, rxu::decay_t<E>>, detail::throw_ptr_tag, detail::throw_instance_tag>(), std::move(e), std::move(cn));
}

}

}

#endif
