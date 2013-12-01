 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_DEMATERIALIZE_HPP)
#define CPPRX_RX_OPERATORS_DEMATERIALIZE_HPP

namespace rxcpp
{

namespace detail
{

template<class T>
class DematerializeObservable : public Producer<DematerializeObservable<T>, T>
{
    typedef DematerializeObservable<T> This;
    typedef std::shared_ptr<This> Parent;
    typedef std::shared_ptr<Observable<std::shared_ptr<Notification<T>>>> Source;
    typedef std::shared_ptr<Observer<T>> Destination;

private:

    Source source;

    class _ : public Sink<_, T>, public Observer<std::shared_ptr<Notification<T>>>
    {
        Parent parent;

    public:
        typedef Sink<_, T> SinkBase;

        _(Parent parent, Destination observer, Disposable cancel) :
            SinkBase(std::move(observer), std::move(cancel)),
            parent(parent)
        {
        }

        virtual void OnNext(const std::shared_ptr<Notification<T>>& n)
        {
        	n->Accept([this](const T& t){
	            this->SinkBase::observer->OnNext(t);
        	}, [this](){
	            this->SinkBase::observer->OnCompleted();
	            this->SinkBase::Dispose();
        	}, [this](const std::exception_ptr& e){
	            this->SinkBase::observer->OnError(e);
	            this->SinkBase::Dispose();
        	});
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

    typedef Producer<This, T> ProducerBase;
public:

    DematerializeObservable(Source source) :
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
std::shared_ptr<Observable<T>> Dematerialize(
	const std::shared_ptr<Observable<std::shared_ptr<Notification<T>>>>& source
)
{
	return std::make_shared<detail::DematerializeObservable<T>>(
	    std::move(source)
	);
}

}

#endif