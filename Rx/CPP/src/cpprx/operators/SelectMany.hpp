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
                    std::atomic<int> count;
                    std::mutex lock;
                    ComposableDisposable group;
                };
                auto state = std::make_shared<State>();
                state->count = 0;

                SerialDisposable sourceDisposable;
                state->group.Add(sourceDisposable);

                ++state->count;
                sourceDisposable.Set(Subscribe(
                    source,
                // on next
                    [=](const T& sourceElement)
                    {
                        util::maybe<C> collection;
                        try {
                            collection.set(collectionSelector(sourceElement));
                        } catch(...) {
                            std::unique_lock<std::mutex> guard(state->lock);
                            observer->OnError(std::current_exception());
                            state->group.Dispose();
                            return;
                        }

                        SerialDisposable inner;
                        state->group.Add(inner);

                        ++state->count;
                        inner.Set(Subscribe(
                            *collection.get(),
                        // on next
                            [=](const CI& collectionElement)
                            {
                                util::maybe<U> result;
                                try {
                                    result.set(resultSelector(sourceElement, collectionElement));
                                } catch(...) {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    observer->OnError(std::current_exception());
                                    state->group.Dispose();
                                }
                                std::unique_lock<std::mutex> guard(state->lock);
                                observer->OnNext(std::move(*result.get()));
                            },
                        // on completed
                            [=]
                            {
                                inner.Dispose();
                                if (0 == --state->count) {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    observer->OnCompleted();
                                    state->group.Dispose();
                                }
                            },
                        // on error
                            [=](const std::exception_ptr& error)
                            {
                                std::unique_lock<std::mutex> guard(state->lock);
                                observer->OnError(error);
                                state->group.Dispose();
                            }));
                    },
                // on completed
                    [=]
                    {
                        if (0 == --state->count) {
                            std::unique_lock<std::mutex> guard(state->lock);
                            observer->OnCompleted();
                            state->group.Dispose();
                        } else {
                            sourceDisposable.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        observer->OnError(error);
                        state->group.Dispose();
                    }));
                return state->group;
            });
    }
}

#endif
