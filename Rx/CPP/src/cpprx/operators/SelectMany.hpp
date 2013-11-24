 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SELECTMANY_HPP)
#define CPPRX_RX_OPERATORS_SELECTMANY_HPP

namespace rxcpp
{

    template <class T, class CS, class RS>
    auto SelectMany(
        const std::shared_ptr<Observable<T>>& source,
        CS collectionSelector,
        RS resultSelector)
        -> const std::shared_ptr<Observable<
            typename std::result_of<RS(const T&,
                const typename observable_item<
                    typename std::result_of<CS(const T&)>::type>::type&)>::type>>
    {
        typedef typename std::decay<typename std::result_of<CS(const T&)>::type>::type C;
        typedef typename observable_item<C>::type CI;
        typedef typename std::result_of<RS(const T&, const CI&)>::type U;

        return CreateObservable<U>(
            [=](std::shared_ptr<Observer<U>> observer)
            -> Disposable
            {
                struct State {
                    size_t subscribed;
                    bool cancel;
                    std::mutex lock;
                };
                auto state = std::make_shared<State>();
                state->cancel = false;
                state->subscribed = 0;

                ComposableDisposable cd;

                cd.Add(Disposable([=]{ 
                    std::unique_lock<std::mutex> guard(state->lock);
                    state->cancel = true; })
                );

                ++state->subscribed;
                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& sourceElement)
                    {
                        bool cancel = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            cancel = state->cancel;
                            if (!cancel) ++state->subscribed;
                        }
                        if (!cancel) {
                            util::maybe<C> collection;
                            try {
                                collection.set(collectionSelector(sourceElement));
                            } catch(...) {
                                bool cancel = false;
                                {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    cancel = state->cancel;
                                }
                                if (!cancel) {
                                    observer->OnError(std::current_exception());
                                }
                                cd.Dispose();
                            }
                            if (!!collection) {
                                cd.Add(Subscribe(
                                    *collection.get(),
                                // on next
                                    [=](const CI& collectionElement)
                                    {
                                        bool cancel = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            cancel = state->cancel;
                                        }
                                        if (!cancel) {
                                            util::maybe<U> result;
                                            try {
                                                result.set(resultSelector(sourceElement, collectionElement));
                                            } catch(...) {
                                                observer->OnError(std::current_exception());
                                                cd.Dispose();
                                            }
                                            if (!!result) {
                                                observer->OnNext(std::move(*result.get()));
                                            }
                                        }
                                    },
                                // on completed
                                    [=]
                                    {
                                        bool cancel = false;
                                        bool finished = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            finished = (--state->subscribed) == 0;
                                            cancel = state->cancel;
                                        }
                                        if (!cancel && finished)
                                            observer->OnCompleted(); 
                                    },
                                // on error
                                    [=](const std::exception_ptr& error)
                                    {
                                        bool cancel = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            --state->subscribed;
                                            cancel = state->cancel;
                                        }
                                        if (!cancel)
                                            observer->OnError(error);
                                        cd.Dispose();
                                    }));
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        bool cancel = false;
                        bool finished = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            finished = (--state->subscribed) == 0;
                            cancel = state->cancel;
                        }
                        if (!cancel && finished)
                            observer->OnCompleted(); 
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        bool cancel = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            --state->subscribed;
                            cancel = state->cancel;
                        }
                        if (!cancel)
                            observer->OnError(error);
                    }));
                return cd;
            });
    }
}

#endif