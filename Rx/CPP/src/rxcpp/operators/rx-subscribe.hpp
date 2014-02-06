// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SUBSCRIBE_HPP)
#define RXCPP_OPERATORS_RX_SUBSCRIBE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Observer>
class subscribe_to_observer_factory
{
    Observer observer;
public:
    subscribe_to_observer_factory(Observer o) : observer(std::move(o)) {}
    template<class Observable>
    auto operator()(Observable source)
        -> decltype(source.subscribe(std::move(observer))) {
        return      source.subscribe(std::move(observer));
    }
};
template<class OnNext, class OnError, class OnCompleted>
class subscribe_factory
{
    OnNext onnext;
    OnError onerror;
    OnCompleted oncompleted;
public:
    subscribe_factory(OnNext n, OnError e, OnCompleted c)
        : onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {}
    template<class Observable>
    auto operator()(Observable source)
        -> decltype(source.subscribe(make_observer<typename Observable::value_type>(std::move(onnext), std::move(onerror), std::move(oncompleted)))) {
        return      source.subscribe(make_observer<typename Observable::value_type>(std::move(onnext), std::move(onerror), std::move(oncompleted)));
    }
};

}


template<class Observer>
auto subscribe(Observer o)
    -> typename std::enable_if<is_observer<Observer>::value,
            detail::subscribe_to_observer_factory<Observer>>::type {
    return  detail::subscribe_to_observer_factory<Observer>(std::move(o));
}

template<class OnNext>
auto subscribe(OnNext n)
    -> typename std::enable_if<!is_observer<OnNext>::value,
            detail::subscribe_factory<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty>>::type {
    return  detail::subscribe_factory<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty>(std::move(n), rxcpp::detail::OnErrorEmpty(), rxcpp::detail::OnCompletedEmpty());
}

template<class OnNext, class OnError>
auto subscribe(OnNext n, OnError e)
    ->      detail::subscribe_factory<OnNext, OnError, rxcpp::detail::OnCompletedEmpty> {
    return  detail::subscribe_factory<OnNext, OnError, rxcpp::detail::OnCompletedEmpty>(std::move(n), std::move(e), rxcpp::detail::OnCompletedEmpty());
}

template<class OnNext, class OnError, class OnCompleted>
auto subscribe(OnNext n, OnError e, OnCompleted c)
    ->      detail::subscribe_factory<OnNext, OnError, OnCompleted> {
    return  detail::subscribe_factory<OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c));
}


}

}

#endif
