 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_CONCAT_HPP)
#define CPPRX_RX_OPERATORS_CONCAT_HPP

namespace rxcpp
{

    template <class ObservableT>
    ObservableT Concat(
        const std::shared_ptr<Observable<ObservableT>>& source)
    {
        typedef typename observable_item<ObservableT>::type T;
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    bool completed;
                    bool subscribed;
                    bool cancel;
                    std::queue<ObservableT> queue;
                    Scheduler::shared scheduler;
                    std::mutex lock;
                };
                auto state = std::make_shared<State>();
                state->cancel = false;
                state->subscribed = 0;
                state->scheduler = std::make_shared<CurrentThreadScheduler>();

                ComposableDisposable cd;

                SerialDisposable sd;
                cd.Add(sd);

                cd.Add(Disposable([=]{ 
                    std::unique_lock<std::mutex> guard(state->lock);
                    state->cancel = true; })
                );

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const ObservableT& sourceElement)
                    {
                        bool cancel = false;
                        bool subscribed = false;
                        Scheduler::shared sched;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            cancel = state->cancel;
                            sched = state->scheduler;
                            if (!cancel) {
                                subscribed = state->subscribed;
                                state->queue.push(sourceElement);
                                state->subscribed = true;
                                sched = state->scheduler;
                            }
                        }
                        if (!cancel && !subscribed) {
                            sd.Set(sched->Schedule(
                                fix0([state, cd, sd, observer](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
                            {
                                bool cancel = false;
                                bool finished = false;
                                ObservableT next;
                                {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    finished = state->queue.empty();
                                    cancel = state->cancel;
                                    if (!cancel && !finished) {next = state->queue.front(); state->queue.pop();}
                                }
                                if (!cancel && !finished) {
                                    sd.Set(Subscribe(
                                        next,
                                    // on next
                                        [=](const T& t)
                                        {
                                            bool cancel = false;
                                            {
                                                std::unique_lock<std::mutex> guard(state->lock);
                                                cancel = state->cancel;
                                            }
                                            if (!cancel) {
                                                observer->OnNext(std::move(t)); 
                                            }
                                        },
                                    // on completed
                                        [=]
                                        {
                                            bool cancel = false;
                                            bool finished = false;
                                            bool subscribe = false;
                                            {
                                                std::unique_lock<std::mutex> guard(state->lock);
                                                finished = state->queue.empty() && state->completed;
                                                subscribe = !state->queue.empty();
                                                state->subscribed = subscribe;
                                                cancel = state->cancel;
                                            }
                                            if (!cancel) {
                                                if (subscribe) {sd.Set(s->Schedule(std::move(self)));}
                                                else if (finished) {observer->OnCompleted(); cd.Dispose();}
                                            }
                                        },
                                    // on error
                                        [=](const std::exception_ptr& error)
                                        {
                                            bool cancel = false;
                                            {
                                                std::unique_lock<std::mutex> guard(state->lock);
                                                cancel = state->cancel;
                                            }
                                            if (!cancel) {
                                                observer->OnError(std::current_exception());
                                            }
                                            cd.Dispose();
                                        }));
                                }
                                return Disposable::Empty();             
                            })));
                        }
                    },
                // on completed
                    [=]
                    {
                        bool cancel = false;
                        bool finished = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            state->completed = true;
                            finished = state->queue.empty() && !state->subscribed;
                            cancel = state->cancel;
                        }
                        if (!cancel && finished) {
                            observer->OnCompleted(); 
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        bool cancel = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            cancel = state->cancel;
                        }
                        if (!cancel) {
                            observer->OnError(std::current_exception());
                        }
                        cd.Dispose();
                    }));
                return cd;
            });
    }
}

#endif