 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_TAKE_HPP)
#define CPPRX_RX_OPERATORS_TAKE_HPP

namespace rxcpp
{

    template <class T, class Integral>
    std::shared_ptr<Observable<T>> Take(
        const std::shared_ptr<Observable<T>>& source,
        Integral n 
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                // keep count of remaining calls received OnNext and count of OnNext calls issued.
                auto remaining = std::make_shared<std::tuple<std::atomic<Integral>, std::atomic<Integral>>>(n, n);

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        auto local = --std::get<0>(*remaining);
                        RXCPP_UNWIND_AUTO([&](){
                            if (local >= 0){
                                // all elements received
                                if (--std::get<1>(*remaining) == 0) {
                                    // all elements passed on to observer.
                                    observer->OnCompleted();
                                    cd.Dispose();}}});

                        if (local >= 0) {
                            observer->OnNext(element);
                        } 
                    },
                // on completed
                    [=]
                    {
                        if (std::get<1>(*remaining) == 0 && std::get<0>(*remaining) <= 0) {
                            observer->OnCompleted();
                            cd.Dispose();
                        }
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
    std::shared_ptr<Observable<T>> TakeUntil(
        const std::shared_ptr<Observable<T>>& source,
        const std::shared_ptr<Observable<U>>& terminus
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {

                struct TerminusState {
                    enum type {
                        Live,
                        Terminated
                    };
                };
                struct TakeState {
                    enum type {
                        Taking,
                        Completed
                    };
                };
                struct State {
                    State() : terminusState(TerminusState::Live), takeState(TakeState::Taking) {}
                    std::atomic<typename TerminusState::type> terminusState;
                    std::atomic<typename TakeState::type> takeState;
                };
                auto state = std::make_shared<State>();

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    terminus,
                // on next
                    [=](const T& element)
                    {
                        state->terminusState = TerminusState::Terminated;
                    },
                // on completed
                    [=]
                    {
                        state->terminusState = TerminusState::Terminated;
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        state->terminusState = TerminusState::Terminated;
                    }));

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (state->terminusState == TerminusState::Live) {
                            observer->OnNext(element);
                        } else if (state->takeState.exchange(TakeState::Completed) == TakeState::Taking) {
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on completed
                    [=]
                    {
                        if (state->takeState.exchange(TakeState::Completed) == TakeState::Taking) {
                            state->terminusState = TerminusState::Terminated;
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        state->takeState = TakeState::Completed;
                        state->terminusState = TerminusState::Terminated;
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }
}

#endif
