// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SUBSCRIBE_HPP)
#define RXCPP_OPERATORS_RX_SUBSCRIBE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Subscriber>
class subscribe_factory;

template<class T, class I>
class subscribe_factory<subscriber<T, I>>
{
    subscriber<T, I> scrbr;
public:
    subscribe_factory(subscriber<T, I> s)
        : scrbr(std::move(s))
    {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(std::forward<Observable>(source).subscribe(std::move(scrbr))) {
        return      std::forward<Observable>(source).subscribe(std::move(scrbr));
    }
};

}

template<class T, class Arg0>
auto subscribe(Arg0&& a0)
    ->      detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0)))> {
    return  detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0)))>
                                        (make_subscriber<T>(std::forward<Arg0>(a0)));
}
template<class T, class Arg0, class Arg1>
auto subscribe(Arg0&& a0, Arg1&& a1)
    ->      detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1)))> {
    return  detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1)))>
                                        (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
}
template<class T, class Arg0, class Arg1, class Arg2>
auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2)
    ->      detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))> {
    return  detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))>
                                        (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3>
auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    ->      detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))> {
    return  detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))>
                                        (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
    ->      detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))> {
    return  detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))>
                                        (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
    ->      detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))> {
    return  detail::subscribe_factory<decltype  (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))>
                                        (make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)));
}

namespace detail {

class dynamic_factory
{
public:
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>>(std::forward<Observable>(source));
    }
};

}

inline auto as_dynamic()
    ->      detail::dynamic_factory {
    return  detail::dynamic_factory();
}

}

}

#endif
