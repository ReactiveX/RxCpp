 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_EMPTY_HPP)
#define CPPRX_RX_OPERATORS_EMPTY_HPP

namespace rxcpp
{

    namespace detail
    {
        template<class T>
        class EmptyObservable : public Producer<EmptyObservable<T>, T>
        {
            typedef std::shared_ptr<EmptyObservable<T>> Parent;
            Scheduler::shared scheduler;

            class _ : public Sink<_, T>
            {
                Parent parent;

            public:
                typedef Sink<_, T> SinkBase;

                _(Parent parent, std::shared_ptr < Observer < T >> observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent)
                {
                }

                Disposable Run()
                {
                    auto local = parent;
                    auto that = this->shared_from_this();
                    return parent->scheduler->Schedule(
                        [=](Scheduler::shared) -> Disposable {
                            that->SinkBase::observer->OnCompleted();
                            that->SinkBase::Dispose();
                            return Disposable::Empty();
                    });
                }
            };

            typedef Producer<EmptyObservable<T>, T> ProducerBase;
        public:

            EmptyObservable(Scheduler::shared scheduler) :
                ProducerBase([](Parent parent, std::shared_ptr < Observer < T >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                scheduler(scheduler)
            {
                if (!scheduler) 
                { 
                    this->scheduler = std::make_shared<CurrentThreadScheduler>(); 
                }
            }
        };
    }
    template <class T>
    std::shared_ptr<Observable<T>> Empty(
            Scheduler::shared scheduler = nullptr
        )
    {
        return std::make_shared<detail::EmptyObservable<T>>(std::move(scheduler));
    }
}

#endif
