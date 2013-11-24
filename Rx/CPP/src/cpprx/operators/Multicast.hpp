 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_MULTICAST_HPP)
#define CPPRX_RX_OPERATORS_MULTICAST_HPP

namespace rxcpp
{

    template <class T, class MulticastSubject>
    std::shared_ptr<ConnectableObservable<T>> Multicast(const std::shared_ptr < Observable < T >> &source, const std::shared_ptr<MulticastSubject>& multicastSubject)
    {
        return std::static_pointer_cast<ConnectableObservable<T>>(
            std::make_shared < ConnectableSubject < std::shared_ptr < Observable < T >> , std::shared_ptr<MulticastSubject> >> (source, multicastSubject));
    }
}

#endif