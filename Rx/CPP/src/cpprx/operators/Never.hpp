 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_NEVER_HPP)
#define CPPRX_RX_OPERATORS_NEVER_HPP

namespace rxcpp
{
    template <class T>
    std::shared_ptr<Observable<T>> Never()
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>>) -> Disposable
        {
            return Disposable::Empty();
        });
    }
}

#endif
