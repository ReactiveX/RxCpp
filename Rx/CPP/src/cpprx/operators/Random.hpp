 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_RANDOM_HPP)
#define CPPRX_RX_OPERATORS_RANDOM_HPP

namespace rxcpp
{

    //
    //    std::mt19937 twister;
    //    std::uniform_int_distribution<int>  xDistribution(0, xExtent - 1);
    //    auto xSource = Random(twister, xDistribution, [](std::mt19937& e){
    //        e.seed(std::random_device()());
    //    });
    //
    template<typename Engine, typename Distribution, typename Seeder>
    std::shared_ptr<Observable<typename Distribution::result_type>> Random(
        Engine e, 
        Distribution d, 
        Seeder s,
        Scheduler::shared scheduler = nullptr)
    {
        typedef typename Distribution::result_type T;
        if (!scheduler) {scheduler = std::make_shared<CurrentThreadScheduler>();}
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer) -> Disposable
            {
                struct State 
                {
                    bool cancel;
                    Engine e;
                    Distribution d;
                    T step()
                    {
                        return d(e);
                    }
                };
                auto state = std::make_shared<State>();
                state->cancel = false;
                state->e = e;
                state->d = d;

                // allow the seed to be set
                s(state->e);

                ComposableDisposable cd;

                cd.Add(Disposable([=]{
                    state->cancel = true;
                }));

                cd.Add(scheduler->Schedule(
                    fix0([=](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
                {
                    if (state->cancel)
                        return Disposable::Empty();

                    observer->OnNext(state->step());
                    return s->Schedule(std::move(self));
                })));

                return cd;
            });
    }
}

#endif