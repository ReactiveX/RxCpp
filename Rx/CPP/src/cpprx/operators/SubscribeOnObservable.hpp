 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SUBSCRIBEONOBSERVABLE_HPP)
#define CPPRX_RX_OPERATORS_SUBSCRIBEONOBSERVABLE_HPP

namespace rxcpp
{

    template <class T>
    std::shared_ptr<Observable<T>> SubscribeOnObservable(
        const std::shared_ptr<Observable<T>>& source, 
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                ComposableDisposable cd;

                SerialDisposable sd;
                cd.Add(sd);

                cd.Add(scheduler->Schedule([=](Scheduler::shared) -> Disposable {
                    sd.Set(ScheduledDisposable(scheduler, source->Subscribe(observer)));
                    return Disposable::Empty();
                }));
                return cd;
            });
    }
}

#endif