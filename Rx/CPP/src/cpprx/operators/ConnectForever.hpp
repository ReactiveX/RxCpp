 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_CONNECTFOREVER_HPP)
#define CPPRX_RX_OPERATORS_CONNECTFOREVER_HPP

namespace rxcpp
{

    template <class T>
    const std::shared_ptr<Observable<T>> ConnectForever(
        const std::shared_ptr<ConnectableObservable<T>>& source
        )
    {
        source->Connect();
        return observable(source);
    }
}

#endif