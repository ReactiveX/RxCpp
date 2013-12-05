 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_WHERE_HPP)
#define CPPRX_RX_OPERATORS_WHERE_HPP

namespace rxcpp
{
    namespace detail
    {
        template<class T>
        class WhereObservable : public Producer<WhereObservable<T>, T>
        {
            typedef std::shared_ptr<WhereObservable<T>> Parent;
            typedef std::function<bool(T)> Predicate;

            std::shared_ptr<Observable<T>> source;
            Predicate predicate;

            class _ : public Sink<_, T>, public Observer<T>
            {
                Parent parent;

            public:
                typedef Sink<_, T> SinkBase;

                _(Parent parent, std::shared_ptr < Observer < T >> observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent)
                {
                }

                virtual void OnNext(const T& t)
                {
                    util::maybe<bool> shouldRun;
                    try {
                        shouldRun.set(parent->predicate(t));
                    } catch (...) {
                        SinkBase::observer->OnError(std::current_exception());
                        SinkBase::Dispose();
                    }
                    if (!!shouldRun && *shouldRun.get()) {
                        SinkBase::observer->OnNext(t);
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

            typedef Producer<WhereObservable<T>, T> ProducerBase;
        public:

            WhereObservable(const std::shared_ptr<Observable<T>>& source, Predicate predicate) :
                ProducerBase([this](Parent parent, std::shared_ptr < Observer < T >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<WhereObservable::_>(new WhereObservable::_(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return this->source->Subscribe(sink);
                }),
                source(source),
                predicate(std::move(predicate))
            {
            }
        };
    }
    template <class T, class P>
    std::shared_ptr<Observable<T>> Where(
        const std::shared_ptr<Observable<T>>& source,
        P predicate
    )
    {
        return std::make_shared<detail::WhereObservable<T>>(source, std::move(predicate));
    }
}

#endif