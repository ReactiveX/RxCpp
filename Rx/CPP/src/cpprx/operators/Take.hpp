 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_TAKE_HPP)
#define CPPRX_RX_OPERATORS_TAKE_HPP

namespace rxcpp
{

    namespace detail
    {
        template<class T, class Integral>
        class TakeObservable : public Producer<TakeObservable<T, Integral>, T>
        {
            typedef std::shared_ptr<TakeObservable<T, Integral>> Parent;

            std::shared_ptr<Observable<T>> source;
            Integral count;

            class _ : public Sink<_, T>, public Observer<T>
            {
                Parent parent;
                Integral remaining;

            public:
                typedef Sink<_, T> SinkBase;

                _(Parent parent, std::shared_ptr < Observer < T >> observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent),
                    remaining(parent->count)
                {
                }

                virtual void OnNext(const T& t)
                {
                    if (remaining > 0)
                    {
                        --remaining;
                        SinkBase::observer->OnNext(t);

                        if (remaining == 0)
                        {
                            SinkBase::observer->OnCompleted();
                            SinkBase::Dispose();
                        }
                    }
                }
                virtual void OnCompleted()
                {
                    SinkBase::observer->OnCompleted();
                    SinkBase::Dispose();
                }
                virtual void OnError(const std::exception_ptr& e)
                {
                    SinkBase::observer->OnError(e);
                    SinkBase::Dispose();
                }
            };

            typedef Producer<TakeObservable<T, Integral>, T> ProducerBase;
        public:

            TakeObservable(const std::shared_ptr<Observable<T>>& source, Integral count) :
                ProducerBase([this](Parent parent, std::shared_ptr < Observer < T >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return this->source->Subscribe(sink);
                }),
                source(source),
                count(count)
            {
            }
        };
    }
    template <class T, class Integral>
    std::shared_ptr<Observable<T>> Take(
        const std::shared_ptr<Observable<T>>& source,
        Integral n 
    )
    {
        return std::make_shared<detail::TakeObservable<T, Integral>>(source, n);
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
                    [=](const U& element)
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
