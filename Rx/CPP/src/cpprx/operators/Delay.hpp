 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_DELAY_HPP)
#define CPPRX_RX_OPERATORS_DELAY_HPP

namespace rxcpp
{

    template <class T>
    std::shared_ptr<Observable<T>> Delay(
        const std::shared_ptr<Observable<T>>& source,
        Scheduler::clock::duration due,
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                auto cancel = std::make_shared<bool>(false);

                ComposableDisposable cd;

                cd.Add(Disposable([=]{ 
                    *cancel = true; }));

                SerialDisposable sd;
                auto wsd = cd.Add(sd);

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        auto sched_disposable = scheduler->Schedule(
                            due, 
                            [=] (Scheduler::shared) -> Disposable { 
                                if (!*cancel)
                                    observer->OnNext(element); 
                                return Disposable::Empty();
                            }
                        );
                        auto ssd = wsd.lock();
                        if (ssd)
                        {
                            *ssd.get() = std::move(sched_disposable);
                        }
                    },
                // on completed
                    [=]
                    {
                        auto sched_disposable = scheduler->Schedule(
                            due, 
                            [=](Scheduler::shared) -> Disposable { 
                                if (!*cancel)
                                    observer->OnCompleted(); 
                                return Disposable::Empty();
                            }
                        );
                        auto ssd = wsd.lock();
                        if (ssd)
                        {
                            *ssd.get() = std::move(sched_disposable);
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        if (!*cancel)
                            observer->OnError(error);
                    }));
                return cd;
            });
    }
}

#endif