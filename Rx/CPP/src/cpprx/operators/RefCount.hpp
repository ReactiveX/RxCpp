 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_REFCOUNT_HPP)
#define CPPRX_RX_OPERATORS_REFCOUNT_HPP

namespace rxcpp
{

    namespace detail
    {
        template<class T>
        class RefCountObservable : public Producer<RefCountObservable<T>, T>
        {
            std::shared_ptr<ConnectableObservable<T>> source;
            std::mutex lock;
            size_t refcount;
            util::maybe<Disposable> subscription;
            
            class _ : public Sink<_, T>, public Observer<T>
            {
                std::shared_ptr<RefCountObservable<T>> parent;
                
            public:
                typedef Sink<_, T> SinkBase;

                _(std::shared_ptr<RefCountObservable<T>> parent, std::shared_ptr<Observer<T>> observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent)
                {
                }
                
                Disposable Run()
                {
                    SerialDisposable subscription;
                    subscription.Set(parent->source->Subscribe(this->shared_from_this()));

                    std::unique_lock<std::mutex> guard(parent->lock);
                    if (++parent->refcount == 1)
                    {
                        parent->subscription.set(parent->source->Connect());
                    }

                    auto local = parent;

                    return Disposable([subscription, local]()
                    {
                        subscription.Dispose();
                        std::unique_lock<std::mutex> guard(local->lock);
                        if (--local->refcount == 0)
                        {
                            local->subscription->Dispose();
                            local->subscription.reset();
                        }
                    });
                }
                
                virtual void OnNext(const T& t)
                {
                    SinkBase::observer->OnNext(t);
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
            
            typedef Producer<RefCountObservable<T>, T> ProducerBase;
        public:
            
            RefCountObservable(std::shared_ptr<ConnectableObservable<T>> source) :
                ProducerBase([](std::shared_ptr<RefCountObservable<T>> that, std::shared_ptr<Observer<T>> observer, Disposable&& cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<RefCountObservable::_>(new RefCountObservable::_(that, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                source(std::move(source)),
                refcount(0)
            {
                subscription.set(Disposable::Empty());
            }
        };
    }
    template <class T>
    const std::shared_ptr<Observable<T>> RefCount(
        const std::shared_ptr<ConnectableObservable<T>>& source
        )
    {
        return std::make_shared<detail::RefCountObservable<T>>(source);
    }
}

#endif
