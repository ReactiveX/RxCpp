 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SELECT_HPP)
#define CPPRX_RX_OPERATORS_SELECT_HPP

namespace rxcpp
{

    namespace detail
    {
        template<class Tin, class Tout>
        class SelectObservable : public Producer<SelectObservable<Tin, Tout>, Tout>
        {
            typedef SelectObservable<Tin, Tout> Self;
            typedef std::shared_ptr<Self> Parent;
            typedef std::function<Tout(Tin)> Selector;

            std::shared_ptr<Observable<Tin>> source;
            Selector selector;

            class _ : public Sink<_, Tout>, public Observer<Tin>
            {
                Parent parent;

            public:
                typedef Sink<_, Tout> SinkBase;

                _(Parent parent, std::shared_ptr < Observer < Tout >> observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent)
                {
                }

                virtual void OnNext(const Tin& t)
                {
                    util::maybe<Tout> result;
                    try {
                        result.set(parent->selector(t));
                    } catch (...) {
                        SinkBase::observer->OnError(std::current_exception());
                        SinkBase::Dispose();
                    }
                    if (!!result) {
                        SinkBase::observer->OnNext(*result.get());
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

            typedef Producer<Self, Tout> ProducerBase;
        public:

            SelectObservable(const std::shared_ptr<Observable<Tin>>& source, Selector selector) :
                ProducerBase([this](Parent parent, std::shared_ptr < Observer < Tout >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<Self::_>(new Self::_(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return this->source->Subscribe(sink);
                }),
                source(source),
                selector(std::move(selector))
            {
            }
        };
    }
    template <class Tin, class S>
    auto Select(
        const std::shared_ptr<Observable<Tin>>& source,
        S selector
    ) -> const std::shared_ptr<Observable<typename std::result_of<S(const Tin&)>::type>>
    {
        typedef typename std::result_of<S(const Tin&)>::type Tout;
        return std::make_shared<detail::SelectObservable<Tin, Tout>>(source, std::move(selector));
    }
}

#endif
