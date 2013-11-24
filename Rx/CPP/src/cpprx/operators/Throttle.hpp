 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_THROTTLE_HPP)
#define CPPRX_RX_OPERATORS_THROTTLE_HPP

namespace rxcpp
{

    template <class T>
    std::shared_ptr<Observable<T>> Throttle(
        const std::shared_ptr<Observable<T>>& source,
        Scheduler::clock::duration due,
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    State() : hasValue(false), id(0) {}
                    std::mutex lock;
                    T value;
                    bool hasValue;
                    size_t id;
                };
                auto state = std::make_shared<State>();

                ComposableDisposable cd;

                SerialDisposable sd;
                cd.Add(sd);

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        size_t current = 0;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            state->hasValue = true;
                            state->value = std::move(element);
                            current = ++state->id;
                        }
                        sd.Set(scheduler->Schedule(
                            due, 
                            [=] (Scheduler::shared) -> Disposable { 
                                {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    if (state->hasValue && state->id == current) {
                                        observer->OnNext(std::move(state->value));
                                    }
                                    state->hasValue = false;
                                }

                                return Disposable::Empty();
                            }
                        ));
                    },
                // on completed
                    [=]
                    {
                        bool sendValue = false;
                        T value;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            sendValue = state->hasValue;
                            if (sendValue) {
                                value = std::move(state->value);}
                            state->hasValue = false;
                            ++state->id;
                        }
                        if (sendValue) {
                            observer->OnNext(std::move(value));}
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            state->hasValue = false;
                            ++state->id;
                        }
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }
}

#endif