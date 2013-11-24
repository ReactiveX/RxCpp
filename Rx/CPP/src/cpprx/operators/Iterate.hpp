 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_ITERATE_HPP)
#define CPPRX_RX_OPERATORS_ITERATE_HPP

namespace rxcpp
{

    using std::begin;
    using std::end;
    template <class Range>
    auto Iterate(
        Range r,
        Scheduler::shared scheduler = nullptr)
        -> std::shared_ptr<Observable<typename std::decay<decltype(*begin(r))>::type>>
    {
        typedef decltype(begin(r)) It;
        typedef typename std::decay<decltype(*begin(r))>::type T;

        if (!scheduler) {scheduler = std::make_shared<CurrentThreadScheduler>();}
        auto range = std::make_shared<Range>(std::move(r));

        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer) -> Disposable
        {
            struct State 
            {
                explicit State(std::shared_ptr<Range> rangeArg) : cancel(false) {
                    this->range = std::move(rangeArg);
                    this->r_cursor = begin(*this->range);
                    this->r_end = end(*this->range); 
                }
                bool cancel;
                std::shared_ptr<Range> range;
                It r_cursor;
                It r_end;
            };
            auto state = std::make_shared<State>(range);

            ComposableDisposable cd;

            cd.Add(Disposable([=]{
                state->cancel = true;
            }));

            cd.Add(scheduler->Schedule(
                fix0([=](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
            {
                if (state->cancel)
                    return Disposable::Empty();

                if (state->r_cursor == state->r_end)
                {
                    observer->OnCompleted();
                }
                else
                {
                    observer->OnNext(*state->r_cursor);
                    ++state->r_cursor; 
                    return s->Schedule(std::move(self));
                }
                return Disposable::Empty();             
            })));

            return cd;
        });
    }
}

#endif