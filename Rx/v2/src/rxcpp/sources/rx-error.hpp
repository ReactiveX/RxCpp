// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_ERROR_HPP)
#define RXCPP_SOURCES_RX_ERROR_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class T>
struct error : public source_base<T>
{
    typedef error<T> this_type;

    struct error_initial_type
    {
        error_initial_type(std::exception_ptr e, rxsc::scheduler sc)
            : exception(e)
            , factory(sc)
        {
        }
        std::exception_ptr exception;
        rxsc::scheduler factory;
    };
    error_initial_type initial;

    error(std::exception_ptr e, rxsc::scheduler sc)
        : initial(e, sc)
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber o) {

        // creates a worker whose lifetime is the same as this subscription
        auto controller = initial.factory.create_worker(o.get_subscription());
        auto exception = initial.exception;

        controller.schedule(
            [=](const rxsc::schedulable&){
                if (!o.is_subscribed()) {
                    // terminate loop
                    return;
                }

                o.on_error(exception);
                // o is unsubscribed
            });
    }
};

struct throw_ptr_tag{};
struct throw_instance_tag{};

template <class T>
auto make_error(throw_ptr_tag&&, std::exception_ptr exception, rxsc::scheduler scheduler)
    ->      observable<T, error<T>> {
    return  observable<T, error<T>>(error<T>(std::move(exception), std::move(scheduler)));
}

template <class T, class E>
auto make_error(throw_instance_tag&&, E e, rxsc::scheduler scheduler)
    ->      observable<T, error<T>> {
    std::exception_ptr exception;
    try {throw e;} catch(...) {exception = std::current_exception();}
    return  observable<T, error<T>>(error<T>(std::move(exception), std::move(scheduler)));
}

}

template<class T, class E>
auto error(E e, rxsc::scheduler sc = rxsc::make_current_thread())
    -> decltype(detail::make_error<T>(typename std::conditional<std::is_same<std::exception_ptr, typename std::decay<E>::type>::value, detail::throw_ptr_tag, detail::throw_instance_tag>::type(), std::move(e), std::move(sc))) {
    return      detail::make_error<T>(typename std::conditional<std::is_same<std::exception_ptr, typename std::decay<E>::type>::value, detail::throw_ptr_tag, detail::throw_instance_tag>::type(), std::move(e), std::move(sc));
}

}

}

#endif
