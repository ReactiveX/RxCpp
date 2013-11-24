 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_INTERVAL_HPP)
#define CPPRX_RX_OPERATORS_INTERVAL_HPP

namespace rxcpp
{

    inline std::shared_ptr<Observable<size_t>> Interval(
        Scheduler::clock::duration due,
        Scheduler::shared scheduler)
    {
        return CreateObservable<size_t>(
            [=](std::shared_ptr<Observer<size_t>> observer)
            -> Disposable
            {
                struct State : public std::enable_shared_from_this<State> {
                    State(Scheduler::clock::duration due,
                        Scheduler::clock::time_point last,
                        std::shared_ptr<Observer<size_t>> observer) : due(due), last(last), cursor(0), observer(observer) {
                        cd.Add(sd);}

                    Scheduler::clock::duration due;
                    Scheduler::clock::time_point last;
                    size_t cursor;
                    std::shared_ptr<Observer<size_t>> observer;
                    ComposableDisposable cd;
                    SerialDisposable sd;

                    void Tick(Scheduler::shared s){
                        observer->OnNext(cursor); 
                        last += due;
                        ++cursor;
                        auto keepAlive = this->shared_from_this();
                        sd.Set(s->Schedule(
                            last, 
                            [this, keepAlive] (Scheduler::shared s) -> Disposable {
                                Tick(s);
                                return Disposable::Empty();
                            }
                        ));
                    }
                };
                auto state = std::make_shared<State>(due, scheduler->Now(), observer);

                state->last += state->due;
                state->sd.Set(scheduler->Schedule(
                    state->last, 
                    [=] (Scheduler::shared s) -> Disposable {
                        state->Tick(s);
                        return Disposable::Empty();
                    }
                ));
                return state->cd;
            });
    }
}

#endif