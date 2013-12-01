 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_MATERIALIZE_HPP)
#define CPPRX_RX_OPERATORS_MATERIALIZE_HPP

namespace rxcpp
{

namespace detail
{

template<class T>
class MaterializeObservable : public Producer<MaterializeObservable<T>, std::shared_ptr<Notification<T>>>
{
    typedef MaterializeObservable<T> This;
    typedef std::shared_ptr<This> Parent;
    typedef std::shared_ptr<Observable<T>> Source;
    typedef std::shared_ptr<Observer<std::shared_ptr<Notification<T>>>> Destination;

private:

    Source source;

    class _ : public Sink<_, std::shared_ptr<Notification<T>>>, public Observer<T>
    {
        Parent parent;

    public:
        typedef Sink<_, std::shared_ptr<Notification<T>>> SinkBase;

        _(Parent parent, Destination observer, Disposable cancel) :
            SinkBase(std::move(observer), std::move(cancel)),
            parent(parent)
        {
        }

        virtual void OnNext(const T& t)
        {
            SinkBase::observer->OnNext(Notification<T>::CreateOnNext(t));
        }
        virtual void OnCompleted()
        {
            SinkBase::observer->OnNext(Notification<T>::CreateOnCompleted());
            SinkBase::observer->OnCompleted();
            SinkBase::Dispose();
        }
        virtual void OnError(const std::exception_ptr& e)
        {
            SinkBase::observer->OnNext(Notification<T>::CreateOnError(e));
            SinkBase::observer->OnCompleted();
            SinkBase::Dispose();
        }
    };

    typedef Producer<This, std::shared_ptr<Notification<T>>> ProducerBase;
public:

    MaterializeObservable(Source source) :
        ProducerBase([this](Parent parent, Destination observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
        {
            auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
            setSink(sink->GetDisposable());
            return this->source->Subscribe(sink);
        }),
        source(std::move(source))
    {
    }
};

}

template <class T>
std::shared_ptr<Observable<std::shared_ptr<Notification<T>>>> Materialize(
    const std::shared_ptr<Observable<T>>& source
)
{
    return std::make_shared<detail::MaterializeObservable<T>>(
        std::move(source)
    );
}

}

#endif