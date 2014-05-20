// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_PUBLISH_HPP)
#define RXCPP_OPERATORS_RX_PUBLISH_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<template<class T> class Subject>
class publish_factory
{
public:
    publish_factory() {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      connectable_observable<typename std::decay<Observable>::type::value_type,   multicast<typename std::decay<Observable>::type::value_type, Observable,    Subject<typename std::decay<Observable>::type::value_type>>> {
        return  connectable_observable<typename std::decay<Observable>::type::value_type,   multicast<typename std::decay<Observable>::type::value_type, Observable,    Subject<typename std::decay<Observable>::type::value_type>>>(
                                                                                            multicast<typename std::decay<Observable>::type::value_type, Observable,    Subject<typename std::decay<Observable>::type::value_type>>(
                                                                                                std::forward<Observable>(source),                                       Subject<typename std::decay<Observable>::type::value_type>()));
    }
};

}

inline auto publish()
    ->      detail::publish_factory<rxsub::subject> {
    return  detail::publish_factory<rxsub::subject>();
}

}

}

#endif
