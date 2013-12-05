 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SCAN_HPP)
#define CPPRX_RX_OPERATORS_SCAN_HPP

namespace rxcpp
{
    namespace detail
    {
        template<class T, class A>
        class ScanObservable : public Producer<ScanObservable<T, A>, A>
        {
            typedef ScanObservable<T, A> This;
            typedef std::shared_ptr<This> Parent;
            typedef std::shared_ptr<Observable<T>> Source;
            typedef std::shared_ptr<Observer<A>> Destination;

        public:
            typedef std::function<A(A, T)> Accumulator;
            typedef std::function<util::maybe<A>(T)> Seeder;

        private:

            Source source;
            util::maybe<A> seed;
            Accumulator accumulator;
            Seeder seeder;

            class _ : public Sink<_, A>, public Observer<T>
            {
                Parent parent;
                util::maybe<A> accumulation;

            public:
                typedef Sink<_, A> SinkBase;

                _(Parent parent, Destination observer, Disposable cancel) :
                    SinkBase(std::move(observer), std::move(cancel)),
                    parent(parent)
                {
                }

                virtual void OnNext(const T& t)
                {
                    try
                    {
                        if (accumulation)
                        {
                            accumulation.set(parent->accumulator(*accumulation.get(), t));
                        }
                        else
                        {
                            accumulation.set(!!parent->seed ? parent->accumulator(*parent->seed.get(), t) : *parent->seeder(t).get());
                        }
                    }
                    catch (...)
                    {
                        SinkBase::observer->OnError(std::current_exception());
                        SinkBase::Dispose();
                        return;
                    }
                    SinkBase::observer->OnNext(*accumulation.get());
                }
                virtual void OnCompleted()
                {
                    if (!accumulation && !!parent->seed) {
                        SinkBase::observer->OnNext(*parent->seed.get());
                    }
                    SinkBase::observer->OnCompleted();
                    SinkBase::Dispose();
                }
                virtual void OnError(const std::exception_ptr& e)
                {
                    SinkBase::observer->OnError(e);
                    SinkBase::Dispose();
                }
            };

            typedef Producer<This, A> ProducerBase;
        public:

            ScanObservable(Source source, util::maybe<A> seed, Accumulator accumulator, Seeder seeder) :
                ProducerBase([this](Parent parent, std::shared_ptr < Observer < A >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<ScanObservable::_>(new ScanObservable::_(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return this->source->Subscribe(sink);
                }),
                source(std::move(source)),
                seed(std::move(seed)),
                accumulator(std::move(accumulator)),
                seeder(std::move(seeder))
            {
            }
        };
    }
    template <class T, class A>
    std::shared_ptr<Observable<A>> Scan(
        const std::shared_ptr<Observable<T>>& source,
        A seed,
        typename detail::ScanObservable<T, A>::Accumulator accumulator
        )
    {
        return std::make_shared<detail::ScanObservable<T, A>>(
            std::move(source), 
            util::maybe<A>(std::move(seed)), 
            std::move(accumulator),
            [](T) -> util::maybe<A> {abort(); return util::maybe<A>();});
    }
    template <class T>
    std::shared_ptr<Observable<T>> Scan(
        const std::shared_ptr<Observable<T>>& source,
        typename detail::ScanObservable<T, T>::Accumulator accumulator
        )
    {
        return std::make_shared<detail::ScanObservable<T, T>>(
            std::move(source), 
            util::maybe<T>(), 
            std::move(accumulator),
            [](T t) -> util::maybe<T> {return util::maybe<T>(t);});
    }
}

#endif