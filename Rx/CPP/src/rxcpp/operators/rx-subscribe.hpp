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
template<class OnNext, class OnError, class OnCompleted>
class subscribe_factory_chained
{
    composite_subscription cs;
    OnNext onnext;
    OnError onerror;
    OnCompleted oncompleted;
public:
    subscribe_factory_chained(OnNext n, OnError e, OnCompleted c)
        : cs(std::move(cs))
        , onnext(std::move(n))
        , onerror(std::move(e))
        , oncompleted(std::move(c))
    {}
    template<class Observable>
    auto operator()(Observable source)
        -> decltype(source.subscribe(make_observer<typename Observable::value_type>(std::move(cs), std::move(onnext), std::move(onerror), std::move(oncompleted)))) {
        return      source.subscribe(make_observer<typename Observable::value_type>(std::move(cs), std::move(onnext), std::move(onerror), std::move(oncompleted)));
    }
};

template<class Observer>
auto subscribe(Observer o, tag_observer&&)
    ->      subscribe_to_observer_factory<Observer> {
    return  subscribe_to_observer_factory<Observer>(std::move(o));
}

struct tag_function {};
template<class OnNext>
auto subscribe(OnNext n, tag_function&&)
    ->      subscribe_factory<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty> {
    return  subscribe_factory<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty>(std::move(n), rxcpp::detail::OnErrorEmpty(), rxcpp::detail::OnCompletedEmpty());
}

template<class OnNext, class OnError>
auto subscribe(OnNext n, OnError e, tag_function&&)
    ->      subscribe_factory<OnNext, OnError, rxcpp::detail::OnCompletedEmpty> {
    return  subscribe_factory<OnNext, OnError, rxcpp::detail::OnCompletedEmpty>(std::move(n), std::move(e), rxcpp::detail::OnCompletedEmpty());
}

template<class OnNext, class OnError, class OnCompleted>
auto subscribe(OnNext n, OnError e, OnCompleted c, tag_function&&)
    ->      subscribe_factory<OnNext, OnError, OnCompleted> {
    return  subscribe_factory<OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c));
}

template<class OnNext>
auto subscribe(composite_subscription cs, OnNext n, tag_subscription&&)
    ->      subscribe_factory_chained<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty> {
    return  subscribe_factory_chained<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty>(std::move(cs), std::move(n), rxcpp::detail::OnErrorEmpty(), rxcpp::detail::OnCompletedEmpty());
}

template<class OnNext, class OnError>
auto subscribe(composite_subscription cs, OnNext n, OnError e, tag_subscription&&)
    ->      subscribe_factory_chained<OnNext, OnError, rxcpp::detail::OnCompletedEmpty> {
    return  subscribe_factory_chained<OnNext, OnError, rxcpp::detail::OnCompletedEmpty>(std::move(cs), std::move(n), std::move(e), rxcpp::detail::OnCompletedEmpty());
}

}

template<class Arg>
auto subscribe(Arg a)
    -> decltype(detail::subscribe(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, detail::tag_function>::type())) {
    return      detail::subscribe(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, detail::tag_function>::type());
}

template<class Arg1, class Arg2>
auto subscribe(Arg1 a1, Arg2 a2)
    -> decltype(detail::subscribe(std::move(a1), std::move(a2), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type())) {
    return      detail::subscribe(std::move(a1), std::move(a2), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type());
}

template<class Arg1, class Arg2, class Arg3>
auto subscribe(Arg1 a1, Arg2 a2, Arg3 a3)
    -> decltype(detail::subscribe(std::move(a1), std::move(a2), std::move(a3), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type())) {
    return      detail::subscribe(std::move(a1), std::move(a2), std::move(a3), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, detail::tag_function>::type());
}

template<class OnNext, class OnError, class OnCompleted>
auto subscribe(composite_subscription cs, OnNext n, OnError e, OnCompleted c)
    ->      detail::subscribe_factory<OnNext, OnError, OnCompleted> {
    return  detail::subscribe_factory<OnNext, OnError, OnCompleted>(std::move(cs), std::move(n), std::move(e), std::move(c));
}

}

}

#endif
