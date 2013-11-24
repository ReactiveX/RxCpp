 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SKIP_HPP)
#define CPPRX_RX_OPERATORS_SKIP_HPP

namespace rxcpp
{

    template <class T, class Integral>
    std::shared_ptr<Observable<T>> Skip(
        const std::shared_ptr<Observable<T>>& source,
        Integral n
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    enum type {
                        Skipping,
                        Forwarding
                    };
                };
                // keep count of remaining OnNext calls to skip and state.
                auto remaining = std::make_shared<std::tuple<std::atomic<Integral>, std::atomic<typename State::type>>>(n, State::Skipping);

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (std::get<1>(*remaining) == State::Forwarding) {
                            observer->OnNext(element);
                        } else {
                            auto local = --std::get<0>(*remaining);

                            if (local == 0) {
                                std::get<1>(*remaining) = State::Forwarding;
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }

    template <class T, class U>
    std::shared_ptr<Observable<T>> SkipUntil(
        const std::shared_ptr<Observable<T>>& source,
        const std::shared_ptr<Observable<U>>& terminus
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct SkipState {
                    enum type {
                        Skipping,
                        Taking
                    };
                };
                struct State {
                    State() : skipState(SkipState::Skipping) {}
                    std::atomic<typename SkipState::type> skipState;
                };
                auto state = std::make_shared<State>();

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    terminus,
                // on next
                    [=](const T& element)
                    {
                        state->skipState = SkipState::Taking;
                    },
                // on completed
                    [=]
                    {
                        state->skipState = SkipState::Taking;
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        state->skipState = SkipState::Taking;
                    }));

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (state->skipState == SkipState::Taking) {
                            observer->OnNext(element);
                        }
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }
}

#endif