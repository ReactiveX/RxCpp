 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_USING_HPP)
#define CPPRX_RX_OPERATORS_USING_HPP

namespace rxcpp
{

    namespace detail
    {
        template<class T, class R, class ScopedObservable>
        class UsingObservable : public Producer<UsingObservable<T, R, ScopedObservable>, T>
        {
            typedef UsingObservable<T, R, ScopedObservable> This;
            typedef std::shared_ptr<This> Parent;
            typedef typename std::decay<ScopedObservable>::type Source;

        public:
            typedef std::function<R()> ResourceFactory;
            typedef std::function<ScopedObservable(R)> ObservableFactory;

        private:
            ResourceFactory resourceFactory;
            ObservableFactory observableFactory;

            class _ : public Sink<_, T>, public Observer<T>
            {
                Parent parent;
                Source source;

            public:
                typedef Sink<_, T> SinkBase;

                _(Parent parent, std::shared_ptr < Observer < T >> observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent)
                {
                }

                Disposable Run()
                {
                    ComposableDisposable cd;
                    auto disposable = Disposable::Empty();

                    try
                    {
                        auto resource = parent->resourceFactory();
                        disposable = resource;
                        source = parent->observableFactory(resource);
                    }
                    catch (...)
                    {
                        cd.Add(Throw<T>(std::current_exception())->Subscribe(this->shared_from_this()));
                        cd.Add(std::move(disposable));
                        return cd;
                    }

                    cd.Add(source->Subscribe(this->shared_from_this()));
                    cd.Add(std::move(disposable));
                    return cd;
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

            typedef Producer<This, T> ProducerBase;
        public:

            UsingObservable(ResourceFactory resourceFactory, ObservableFactory observableFactory) :
                ProducerBase([](Parent parent, std::shared_ptr < Observer < T >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<UsingObservable::_>(new UsingObservable::_(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                resourceFactory(resourceFactory),
                observableFactory(observableFactory)
            {
            }
        };
    }
    template <class RF, class OF>
    auto Using(
        RF resourceFactory,
        OF observableFactory
        )
        -> decltype(observableFactory(resourceFactory()))
    {
        typedef decltype(observableFactory(resourceFactory())) ScopedObservable;
        typedef typename observable_item<ScopedObservable>::type T;
        typedef decltype(resourceFactory()) R;
        return std::make_shared<detail::UsingObservable<T, R, ScopedObservable>>(std::move(resourceFactory), std::move(observableFactory));
    }
}

#endif