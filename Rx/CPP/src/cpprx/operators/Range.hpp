 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_RANGE_HPP)
#define CPPRX_RX_OPERATORS_RANGE_HPP

namespace rxcpp
{


    template <class Integral>
    auto Range(
        Integral start, Integral end = std::numeric_limits<Integral>::max(), Integral step = 1,
        Scheduler::shared scheduler = nullptr)
        -> std::shared_ptr<Observable<Integral>>
    {
        if (!scheduler) {scheduler = std::make_shared<CurrentThreadScheduler>();}
        return CreateObservable<Integral>(
            [=](std::shared_ptr<Observer<Integral>> observer) -> Disposable
        {
            struct State 
            {
                bool cancel;
                Integral i;
                Integral rem;
            };
            auto state = std::make_shared<State>();
            state->cancel = false;
            state->i = start;
            state->rem = ((end - start) + step) / step;

            ComposableDisposable cd;

            cd.Add(Disposable([=]{
                state->cancel = true;
            }));

            cd.Add(scheduler->Schedule(
                fix0([=](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
            {
                if (state->cancel)
                    return Disposable::Empty();

                if (!state->rem)
                {
                    observer->OnCompleted();
                }
                else
                {
                    observer->OnNext(state->i);
                    --state->rem; 
                    state->i += step;
                    return s->Schedule(std::move(self));
                }
                return Disposable::Empty();             
            })));

            return cd;
        });
    }
}

#endif
