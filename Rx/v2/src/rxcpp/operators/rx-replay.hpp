// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_REPLAY_HPP)
#define RXCPP_OPERATORS_RX_REPLAY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<template<class T, class Coordination> class Subject, class Coordination>
class replay_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;

public:
    replay_factory(coordination_type cn)
        : coordination(std::move(cn))
    {
    }

    template<class Observable>
    auto operator()(Observable&& source)
        ->      connectable_observable<rxu::value_type_t<rxu::decay_t<Observable>>,   multicast<rxu::value_type_t<rxu::decay_t<Observable>>, Observable,    Subject<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>>> {
        return  connectable_observable<rxu::value_type_t<rxu::decay_t<Observable>>,   multicast<rxu::value_type_t<rxu::decay_t<Observable>>, Observable,    Subject<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>>>(
                                                                                      multicast<rxu::value_type_t<rxu::decay_t<Observable>>, Observable,    Subject<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>>(
                                                                                          std::forward<Observable>(source),                                 Subject<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>(coordination)));
    }
};

}

template<class Coordination>
inline auto replay(Coordination&& cn)
    ->      detail::replay_factory<rxsub::replay, Coordination> {
    return  detail::replay_factory<rxsub::replay, Coordination>(std::forward<Coordination>(cn));
}

}

}

#endif
