 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_RETURN_HPP)
#define CPPRX_RX_OPERATORS_RETURN_HPP

namespace rxcpp
{

    namespace detail
    {
        template<class T>
        class ReturnObservable : public Producer<ReturnObservable<T>, T>
        {
            typedef std::shared_ptr<ReturnObservable<T>> Parent;
            T value;
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
                            that->SinkBase::observer->OnNext(local->value);
                            that->SinkBase::observer->OnCompleted();
                            that->SinkBase::Dispose();
                            return Disposable::Empty();
                    });
                }
            };

            typedef Producer<ReturnObservable<T>, T> ProducerBase;
        public:

            ReturnObservable(T value, Scheduler::shared scheduler) :
                ProducerBase([](Parent parent, std::shared_ptr < Observer < T >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<ReturnObservable::_>(new ReturnObservable::_(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                value(value),
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
    std::shared_ptr<Observable<T>> Return(
            T value,
            Scheduler::shared scheduler = nullptr
        )
    {
        return std::make_shared<detail::ReturnObservable<T>>(std::move(value), std::move(scheduler));
    }
}

#endif