 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_OBSERVEONOBSERVER_HPP)
#define CPPRX_RX_OPERATORS_OBSERVEONOBSERVER_HPP

namespace rxcpp
{

    template <class T>
    std::shared_ptr<Observable<T>> ObserveOnObserver(
        const std::shared_ptr<Observable<T>>& source, 
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observerArg)
            -> Disposable
            {
                std::shared_ptr<ScheduledObserver<T>> observer(
                    new ScheduledObserver<T>(scheduler, std::move(observerArg)));

                ComposableDisposable cd;

                cd.Add(*observer.get());

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        observer->OnNext(std::move(element));
                        observer->EnsureActive();
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                        observer->EnsureActive();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(std::move(error));
                        observer->EnsureActive();
                    }));
                return cd;
            });
    }
}

#endif