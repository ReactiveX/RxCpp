 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_OPERATORS_HPP)
#define CPPRX_RX_OPERATORS_HPP

namespace rxcpp
{

    //////////////////////////////////////////////////////////////////////
    // 
    // constructors
    template <class T>
    struct CreatedAutoDetachObserver : public Observer<T>
    {
        std::shared_ptr<Observer<T>> observer;
        SerialDisposable disposable;
        
        virtual ~CreatedAutoDetachObserver() {
            clear();
        }

        virtual void OnNext(const T& element)
        {
            if (observer) {
                RXCPP_UNWIND(disposer, [&](){
                    disposable.Dispose();
                });
                observer->OnNext(element);
                disposer.dismiss();
            }
        }
        virtual void OnCompleted() 
        {
            if (observer) {
                RXCPP_UNWIND(disposer, [&](){
                    disposable.Dispose();
                });
                std::shared_ptr<Observer<T>> final;
                using std::swap;
                swap(final, observer);
                final->OnCompleted();
                disposer.dismiss();
            }
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            if (observer) {
                RXCPP_UNWIND(disposer, [&](){
                    disposable.Dispose();
                });
                std::shared_ptr<Observer<T>> final;
                using std::swap;
                swap(final, observer);
                final->OnError(error);
                disposer.dismiss();
            }
        }
        void clear() 
        {
            observer = nullptr;
        }
    };

    template <class T>
    std::shared_ptr<CreatedAutoDetachObserver<T>> CreateAutoDetachObserver(
        std::shared_ptr<Observer<T>> observer
        )
    {
        auto p = std::make_shared<CreatedAutoDetachObserver<T>>();
        p->observer = std::move(observer);
        
        return p;
    }

    template <class T, class S>
    class CreatedObservable : public Observable<T>
    {
        S subscribe;

    public:
        CreatedObservable(S subscribe)
            : subscribe(std::move(subscribe))
        {
        }
        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            auto autoDetachObserver = CreateAutoDetachObserver(std::move(observer));

            if (CurrentThreadScheduler::IsScheduleRequired()) {
                auto scheduler = std::make_shared<CurrentThreadScheduler>();
                scheduler->Schedule(
                   [=](Scheduler::shared) -> Disposable {
                        try {
                            autoDetachObserver->disposable.Set(subscribe(autoDetachObserver));
                        } catch (...) {
                            autoDetachObserver->OnError(std::current_exception());
                        }   
                        return Disposable::Empty();
                   }
                );
                return autoDetachObserver->disposable;
            }
            try {
                autoDetachObserver->disposable.Set(subscribe(autoDetachObserver));
                return autoDetachObserver->disposable;
            } catch (...) {
                autoDetachObserver->OnError(std::current_exception());
            }   
            return Disposable::Empty();
        }
    };

    template <class T, class S>
    std::shared_ptr<Observable<T>> CreateObservable(S subscribe)
    {
        return std::make_shared<CreatedObservable<T,S>>(std::move(subscribe));
    }

    template <class T>
    struct CreatedObserver : public Observer<T>
    {
        std::function<void(const T&)>   onNext;
        std::function<void()>           onCompleted;
        std::function<void(const std::exception_ptr&)> onError;
        
        virtual ~CreatedObserver() {
            clear();
        }

        virtual void OnNext(const T& element)
        {
            if(onNext)
            {
                onNext(element);
            }
        }
        virtual void OnCompleted() 
        {
            if(onCompleted)
            {
                std::function<void()> final;
                using std::swap;
                swap(final, onCompleted);
                clear();
                final();
            }
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            if(onError)
            {
                std::function<void(const std::exception_ptr&)> final;
                using std::swap;
                swap(final, onError);
                clear();
                final(error);
            }
        }
        void clear() 
        {
            onNext = nullptr;
            onCompleted = nullptr;
            onError = nullptr;
        }
    };

    template <class T>
    std::shared_ptr<Observer<T>> CreateObserver(
        std::function<void(const T&)> onNext,
        std::function<void()> onCompleted = nullptr,
        std::function<void(const std::exception_ptr&)> onError = nullptr
        )
    {
        auto p = std::make_shared<CreatedObserver<T>>();
        p->onNext = std::move(onNext);
        p->onCompleted = std::move(onCompleted);
        p->onError = std::move(onError);
        
        return p;
    }


    namespace detail
    {
        template<class Derived, class T>
        class Sink : public std::enable_shared_from_this<Derived>
        {
            typedef std::shared_ptr<Observer<T>> SinkObserver;
            mutable std::mutex lock;
            mutable util::maybe<Disposable> cancel;

        protected:
            mutable SinkObserver observer;
            
        public:
            Sink(SinkObserver observerArg, Disposable cancelArg) :
                observer(std::move(observerArg))
            {
                cancel.set(std::move(cancelArg));
                if (!observer)
                {
                    observer = std::make_shared<Observer<T>>();
                }
            }
            
            void Dispose() const
            {
                std::unique_lock<std::mutex> guard(lock);
                observer = std::make_shared<Observer<T>>();
                if (cancel)
                {
                    cancel->Dispose();
                    cancel.reset();
                }
            }
            
            Disposable GetDisposable() const
            {
                // make sure to capture state and not 'this'.
                // usage means that 'this' will usualy be destructed
                // immediately
                auto local = this->shared_from_this();
                return Disposable([local]{
                    local->Dispose();
                });
            }
            
            class _ : public Observer<T>
            {
                std::shared_ptr<Derived> that;
            public:
                _(std::shared_ptr<Derived> that) : that(that)
                {}
                
                virtual void OnNext(const T& t)
                {
                    std::unique_lock<std::mutex> guard(that->lock);
                    that->observer->OnNext(t);
                }
                virtual void OnCompleted()
                {
                    std::unique_lock<std::mutex> guard(that->lock);
                    that->observer->OnCompleted();
                    if (that->cancel)
                    {
                        that->cancel->Dispose();
                        that->cancel.reset();
                    }
                }
                virtual void OnError(const std::exception_ptr& e)
                {
                    std::unique_lock<std::mutex> guard(that->lock);
                    that->observer->OnError(e);
                    if (that->cancel)
                    {
                        that->cancel->Dispose();
                        that->cancel.reset();
                    }
                }
            };
        };
        
        template<class Derived, class T>
        class Producer : public std::enable_shared_from_this<Derived>, public Observable<T>
        {
        public:
            typedef std::function<void(Disposable)> SetSink;
            typedef std::function<Disposable(std::shared_ptr<Derived>, std::shared_ptr<Observer<T>>, Disposable, SetSink)> Run;
        private:
            Run run;
            struct State
            {
                SerialDisposable sink;
                SerialDisposable subscription;
            };
        public:
            Producer(Run run) : 
                run(std::move(run))
            {
            }

            virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
            {
                auto state = std::make_shared<State>();
                auto that = this->shared_from_this();
                if (CurrentThreadScheduler::IsScheduleRequired()) {
                    auto scheduler = std::make_shared<CurrentThreadScheduler>();
                    scheduler->Schedule([=](Scheduler::shared) -> Disposable
                    {
                            state->subscription.Set(
                                run(that, observer, state->subscription, [=](Disposable d)
                                {
                                    state->sink.Set(std::move(d));
                                }));
                            return Disposable::Empty();
                    });
                }
                else
                {
                    state->subscription.Set(
                        run(that, observer, state->subscription, [=](Disposable d)
                        {
                            state->sink.Set(std::move(d));
                        }));
                }
                return Disposable([=]()
                {
                    state->sink.Dispose();
                    state->subscription.Dispose();
                });
            }
        };
        
    }

    
    struct SubjectState {
        enum type {
            Invalid,
            Forwarding,
            Completed,
            Error
        };
    };

    template <class T, class Base, class Subject>
    class ObservableSubject : 
        public Base,
        public std::enable_shared_from_this<Subject>
    {
    protected:
        std::mutex lock;
        SubjectState::type state;
        std::exception_ptr error;
        std::vector<std::shared_ptr<Observer<T>>> observers;
        
        virtual ~ObservableSubject() {
            // putting this first means that the observers
            // will be destructed outside the lock
            std::vector<std::shared_ptr<Observer<T>>> empty;

            std::unique_lock<decltype(lock)> guard(lock);
            using std::swap;
            swap(observers, empty);
        }

        ObservableSubject() : state(SubjectState::Forwarding) {
        }

        void RemoveObserver(std::shared_ptr<Observer<T>> toRemove)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto it = std::find(begin(observers), end(observers), toRemove);
            if (it != end(observers))
                *it = nullptr;
        }
    public:

        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            std::weak_ptr<Observer<T>> wptr = observer;
            std::weak_ptr<Subject> wself = this->shared_from_this();

            Disposable d([wptr, wself]{
                if (auto self = wself.lock())
                {
                    self->RemoveObserver(wptr.lock());
                }
            });

            {
                std::unique_lock<decltype(lock)> guard(lock);
                if (state == SubjectState::Completed) {
                    observer->OnCompleted();
                    return Disposable::Empty();
                } else if (state == SubjectState::Error) {
                    observer->OnError(error);
                    return Disposable::Empty();
                } else {
                    for(auto& o : observers)
                    {
                        if (!o){
                            o = std::move(observer);
                            return d;
                        }
                    }
                    observers.push_back(std::move(observer));
                    return d;
                }
            }
        }
    };

    template <class T, class Base>
    class ObserverSubject : 
        public Observer<T>,
        public Base
    {
    public:
        ObserverSubject() {}

        template<class A>
        explicit ObserverSubject(A&& a) : Base(std::forward<A>(a)) {}

        virtual void OnNext(const T& element)
        {
            std::unique_lock<decltype(Base::lock)> guard(Base::lock);
            auto local = Base::observers;
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnNext(element);
                }
            }
        }
        virtual void OnCompleted() 
        {
            std::unique_lock<decltype(Base::lock)> guard(Base::lock);
            Base::state = SubjectState::Completed;
            auto local = std::move(Base::observers);
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnCompleted();
                }
            }
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            std::unique_lock<decltype(Base::lock)> guard(Base::lock);
            Base::state = SubjectState::Error;
            Base::error = error;
            auto local = std::move(Base::observers);
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnError(error);
                }
            }
        }
    };

    template <class T>
    class Subject : 
        public ObserverSubject<T, ObservableSubject<T, Observable<T>, Subject<T>>>
    {
    };

    template <class T>
    std::shared_ptr<Subject<T>> CreateSubject()
    {
        return std::make_shared<Subject<T>>();
    }

    template <class K, class T, class Base>
    class GroupedObservableSubject : 
        public Base
    {
        K key;
    public:
        GroupedObservableSubject(K key) : key(std::move(key)) {}

        virtual K Key() {return key;}
    };

    template <class K, class T>
    class GroupedSubject : 
        public ObserverSubject<T, GroupedObservableSubject<K, T, ObservableSubject<T, GroupedObservable<K, T>, GroupedSubject<K, T>>>>
    {
        typedef ObserverSubject<T, GroupedObservableSubject<K, T, ObservableSubject<T, GroupedObservable<K, T>, GroupedSubject<K, T>>>> base;
    public:
        GroupedSubject(K key) : base(std::move(key)) {}
    };

    template <class T, class K>
    std::shared_ptr<GroupedSubject<K, T>> CreateGroupedSubject(K key)
    {
        return std::make_shared<GroupedSubject<K, T>>(std::move(key));
    }

    template <class T>
    class BehaviorSubject : 
        public std::enable_shared_from_this<BehaviorSubject<T>>,
        public Observable<T>,
        public Observer<T>
    {
        std::mutex lock;
        size_t slotCount;
        T value;
        SubjectState::type state;
        util::maybe<std::exception_ptr> error;
        std::vector<std::shared_ptr<Observer<T>>> observers;

        void RemoveObserver(std::shared_ptr<Observer<T>> toRemove)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto it = std::find(begin(observers), end(observers), toRemove);
            if (it != end(observers))
            {
                *it = nullptr;
                ++slotCount;
            }
        }

        BehaviorSubject();
    public:

        typedef std::shared_ptr<BehaviorSubject<T>> shared;

        explicit BehaviorSubject(T t) : slotCount(0), value(std::move(t)), state(SubjectState::Forwarding) {}
        
        virtual ~BehaviorSubject() {
            // putting this first means that the observers
            // will be destructed outside the lock
            std::vector<std::shared_ptr<Observer<T>>> empty;

            std::unique_lock<decltype(lock)> guard(lock);
            using std::swap;
            swap(observers, empty);
        }

        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            std::weak_ptr<Observer<T>> wptr = observer;
            std::weak_ptr<BehaviorSubject> wself = this->shared_from_this();

            Disposable d([wptr, wself]{
                if (auto self = wself.lock())
                {
                    self->RemoveObserver(wptr.lock());
                }
            });

            SubjectState::type localState = SubjectState::Invalid;
            util::maybe<T> localValue;
            util::maybe<std::exception_ptr> localError;
            {
                std::unique_lock<decltype(lock)> guard(lock);

                localState = state;

                if (state == SubjectState::Forwarding || localState == SubjectState::Completed) 
                {
                    localValue.set(value);
                }
                else if (localState == SubjectState::Error)
                {
                    localError = error;
                }

                if (state == SubjectState::Forwarding)
                {
                    if (slotCount > 0)
                    {
                        for(auto& o : observers)
                        {
                            if (!o)
                            {
                                o = observer;
                                --slotCount;
                                break;
                            }
                        }
                    }
                    else
                    {
                        observers.push_back(observer);
                    }
                }
            }

            if (localState == SubjectState::Completed) {
                observer->OnNext(*localValue.get());
                observer->OnCompleted();
                return Disposable::Empty();
            }
            else if (localState == SubjectState::Error) {
                observer->OnError(*localError.get());
                return Disposable::Empty();
            }
            else if (localState == SubjectState::Forwarding) {
                observer->OnNext(*localValue.get());
            }

            return d;
        }

        virtual void OnNext(const T& element)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto local = observers;
            value = element;
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnNext(element);
                }
            }
        }
        virtual void OnCompleted() 
        {
            std::unique_lock<decltype(lock)> guard(lock);
            state = SubjectState::Completed;
            auto local = std::move(observers);
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnCompleted();
                }
            }
        }
        virtual void OnError(const std::exception_ptr& errorArg) 
        {
            std::unique_lock<decltype(lock)> guard(lock);
            state = SubjectState::Error;
            error.set(errorArg);
            auto local = std::move(observers);
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnError(errorArg);
                }
            }
        }
    };

    template <class T, class Arg>
    std::shared_ptr<BehaviorSubject<T>> CreateBehaviorSubject(Arg a)
    {
        return std::make_shared<BehaviorSubject<T>>(std::move(a));
    }

    template <class T>
    class AsyncSubject :
        public std::enable_shared_from_this<AsyncSubject<T>>,
        public Observable<T>,
        public Observer<T>
    {
        std::mutex lock;
        size_t slotCount;
        util::maybe<T> value;
        SubjectState::type state;
        util::maybe<std::exception_ptr> error;
        std::vector < std::shared_ptr < Observer<T >> > observers;

        void RemoveObserver(std::shared_ptr < Observer < T >> toRemove)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto it = std::find(begin(observers), end(observers), toRemove);
            if (it != end(observers))
            {
                *it = nullptr;
                ++slotCount;
            }
        }

    public:

        typedef std::shared_ptr<AsyncSubject<T>> shared;

        AsyncSubject() : slotCount(0), value(), state(SubjectState::Forwarding) {}

        virtual ~AsyncSubject() {
            // putting this first means that the observers
            // will be destructed outside the lock
            std::vector < std::shared_ptr < Observer<T >> > empty;

            std::unique_lock<decltype(lock)> guard(lock);
            using std::swap;
            swap(observers, empty);
        }

        virtual Disposable Subscribe(std::shared_ptr < Observer < T >> observer)
        {
            std::weak_ptr<Observer<T>> wptr = observer;
            std::weak_ptr<AsyncSubject> wself = this->shared_from_this();

            Disposable d([wptr, wself]{
                if (auto self = wself.lock())
                {
                    self->RemoveObserver(wptr.lock());
                }
            });

            SubjectState::type localState = SubjectState::Invalid;
            util::maybe<T> localValue;
            util::maybe<std::exception_ptr> localError;
            {
                std::unique_lock<decltype(lock)> guard(lock);

                localState = state;

                if (localState == SubjectState::Completed) 
                {
                    localValue = value;
                }
                else if (localState == SubjectState::Error)
                {
                    localError = error;
                }
                else if (state == SubjectState::Forwarding)
                {
                    if (slotCount > 0)
                    {
                        for (auto& o : observers)
                        {
                            if (!o)
                            {
                                o = observer;
                                --slotCount;
                                break;
                            }
                        }
                    }
                    else
                    {
                        observers.push_back(observer);
                    }
                }
            }

            if (localState == SubjectState::Completed) {
                if (localValue) {
                    observer->OnNext(*localValue.get());
                }
                observer->OnCompleted();
                return Disposable::Empty();
            }
            else if (localState == SubjectState::Error) {
                observer->OnError(*localError.get());
                return Disposable::Empty();
            }

            return d;
        }

        virtual void OnNext(const T& element)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            if (state == SubjectState::Forwarding) {
                value = element;
            }
        }
        virtual void OnCompleted()
        {
            std::unique_lock<decltype(lock)> guard(lock);
            state = SubjectState::Completed;
            auto local = std::move(observers);
            auto localValue = value;
            guard.unlock();
            for (auto& o : local)
            {
                if (o) {
                    if (localValue) {
                        o->OnNext(*localValue.get());
                    }
                    o->OnCompleted();
                }
            }
        }
        virtual void OnError(const std::exception_ptr& errorArg)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            state = SubjectState::Error;
            error.set(errorArg);
            auto local = std::move(observers);
            guard.unlock();
            for (auto& o : local)
            {
                if (o) {
                    o->OnError(errorArg);
                }
            }
        }
    };

    template <class T>
    std::shared_ptr<AsyncSubject<T>> CreateAsyncSubject()
    {
        return std::make_shared<AsyncSubject<T>>();
    }

#if RXCPP_USE_VARIADIC_TEMPLATES
    template<class... A, class F>
    auto ToAsync(F f, Scheduler::shared scheduler = nullptr)
        ->std::function < std::shared_ptr < Observable< decltype(f((*(A*)nullptr)...)) >> (const A&...)>
    {
        typedef decltype(f((*(A*) nullptr)...)) R;
        if (!scheduler)
        {
            scheduler = std::make_shared<EventLoopScheduler>();
        }
        return [=](const A&... a) -> std::shared_ptr < Observable<R >>
        {
            auto args = std::make_tuple(a...);
            auto result = CreateAsyncSubject<R>();
            scheduler->Schedule([=](Scheduler::shared) -> Disposable
            {
                util::maybe<R> value;
                try
                {
                    value.set(util::tuple_dispatch(f, args));
                }
                catch (...)
                {
                    result->OnError(std::current_exception());
                    return Disposable::Empty();
                }
                result->OnNext(*value.get());
                result->OnCompleted();
                return Disposable::Empty();
            });
            return result;
        };
    }
#endif

    template <class Source, class Subject>
    class ConnectableSubject : 
            public std::enable_shared_from_this<ConnectableSubject<Source, Subject>>,
            public ConnectableObservable<typename subject_item<Subject>::type>
    {
    private:
        ConnectableSubject();

        Source source;
        Subject subject;
        util::maybe<Disposable> subscription;
        std::mutex lock;

    public:
        virtual ~ConnectableSubject() {}

        ConnectableSubject(Source source, Subject subject) : source(source), subject(subject)
        {
        }

        virtual Disposable Connect()
        {
            std::unique_lock<std::mutex> guard(lock);
            if (!subscription)
            {
                subscription.set(source->Subscribe(observer(subject)));
            }
            auto that = this->shared_from_this();
            return Disposable([that]()
            {
                std::unique_lock<std::mutex> guard(that->lock);
                if (that->subscription)
                {
                    that->subscription->Dispose();
                    that->subscription.reset();
                }
            });
        }

        virtual Disposable Subscribe(std::shared_ptr < Observer < typename subject_item<Subject>::type >> observer)
        {
            return subject->Subscribe(observer);
        }
    };

    template <class F>
    struct fix0_thunk {
        F f;
        fix0_thunk(F&& f) : f(std::move(f))
        {
        }
        Disposable operator()(Scheduler::shared s) const 
        {
            return f(s, *this);
        }
    };
    template <class F>
    fix0_thunk<F> fix0(F f)
    {
        return fix0_thunk<F>(std::move(f));
    }

    //////////////////////////////////////////////////////////////////////
    // 
    // imperative functions

    template <class S>
    Disposable Subscribe(
        const S& source,
        typename util::identity<std::function<void(const typename observable_item<S>::type&)>>::type onNext,
        std::function<void()> onCompleted = nullptr,
        std::function<void(const std::exception_ptr&)> onError = nullptr
        )
    {
        auto observer = CreateObserver<typename observable_item<S>::type>(
            std::move(onNext), std::move(onCompleted), std::move(onError));
        
        return source->Subscribe(observer);
    }

    template <class T>
    void ForEach(
        const std::shared_ptr<Observable<T>>& source,
        typename util::identity<std::function<void(const T&)>>::type onNext
        )
    {
        std::mutex lock;
        std::condition_variable wake;
        bool done = false;
        std::exception_ptr error;
        auto observer = CreateObserver<T>(std::move(onNext), 
        //on completed
            [&]{
                std::unique_lock<std::mutex> guard(lock);
                done = true;
                wake.notify_one();
            }, 
        //on error
            [&](const std::exception_ptr& e){
                std::unique_lock<std::mutex> guard(lock);
                done = true;
                error = std::move(e);
                wake.notify_one();
            });
        
        source->Subscribe(observer);

        {
            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&]{return done;});
        }

        if (error != std::exception_ptr()) {
            std::rethrow_exception(error);}
    }
}

#include "operators/Empty.hpp"
#include "operators/Return.hpp"
#include "operators/Throw.hpp"
#include "operators/Range.hpp"
#include "operators/Random.hpp"
#include "operators/Interval.hpp"
#include "operators/Iterate.hpp"
#include "operators/Using.hpp"

//////////////////////////////////////////////////////////////////////
// 
// standard query operators

#include "operators/Select.hpp"
#include "operators/SelectMany.hpp"
#include "operators/Concat.hpp"
#include "operators/CombineLatest.hpp"
#include "operators/Zip.hpp"
#include "operators/Merge.hpp"
#include "operators/Where.hpp"
#include "operators/GroupBy.hpp"
#include "operators/Multicast.hpp"
#include "operators/Publish.hpp"
#include "operators/RefCount.hpp"
#include "operators/ConnectForever.hpp"
#include "operators/Scan.hpp"
#include "operators/Take.hpp"
#include "operators/Skip.hpp"
#include "operators/DistinctUntilChanged.hpp"

//////////////////////////////////////////////////////////////////////
// 
// time

#include "operators/Delay.hpp"
#include "operators/Throttle.hpp"

namespace rxcpp
{

    template <class StdCollection>
    std::shared_ptr<Observable<StdCollection>> ToStdCollection(
        const std::shared_ptr<Observable<typename StdCollection::value_type>>& source
        )
    {
        typedef typename StdCollection::value_type Value;
        return CreateObservable<StdCollection>(
            [=](std::shared_ptr<Observer<StdCollection>> observer) -> Disposable
            {
                auto stdCollection = std::make_shared<StdCollection>();
                return Subscribe(
                    source,
                // on next
                    [=](const Value& element)
                    {
                        stdCollection->insert(stdCollection->end(), element);
                    },
                // on completed
                    [=]
                    {
                        observer->OnNext(std::move(*stdCollection.get()));
                        observer->OnCompleted();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                    });
            });
    }


    // no more than one event ever 'milliseconds'
    // TODO: oops, this is not the right definition for throttle.
    template <class T>
    std::shared_ptr<Observable<T>> LimitWindow(
        const std::shared_ptr<Observable<T>>& source,
        int milliseconds)
    {
        if (milliseconds == 0)
            return source;
        
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    std::chrono::steady_clock::time_point dueTime;
                };
        
                auto state = std::make_shared<State>();

                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        auto now = std::chrono::steady_clock::now();

                        if (now >= state->dueTime)
                        {
                            observer->OnNext(element);
                            state->dueTime = now + std::chrono::duration<int, std::milli>(milliseconds);
                        }
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                    });
            });
    }



    template <class T>
    std::shared_ptr<Observable<T>> SubscribeOnObservable(
        const std::shared_ptr<Observable<T>>& source, 
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                ComposableDisposable cd;

                SerialDisposable sd;
                cd.Add(sd);

                cd.Add(scheduler->Schedule([=](Scheduler::shared) -> Disposable {
                    sd.Set(ScheduledDisposable(scheduler, source->Subscribe(observer)));
                    return Disposable::Empty();
                }));
                return cd;
            });
    }

    template <class T>
    std::shared_ptr<Observable<T>> ObserveOnObserver(
        const std::shared_ptr<Observable<T>>& source, 
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observerArg)
            -> Disposable
            {
                std::shared_ptr<ScheduledObserver<T>> observer(
                    new ScheduledObserver<T>(scheduler, std::move(observerArg)));

                ComposableDisposable cd;

                cd.Add(*observer.get());

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        observer->OnNext(std::move(element));
                        observer->EnsureActive();
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                        observer->EnsureActive();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(std::move(error));
                        observer->EnsureActive();
                    }));
                return cd;
            });
    }

    class StdQueueDispatcher
    {
        mutable std::queue<std::function<void()>> pending;
        mutable std::condition_variable wake;
        mutable std::mutex pendingLock;

        std::function<void()> get() const
        {
            std::function<void()> fn;
            fn = std::move(pending.front());
            pending.pop();
            return std::move(fn);
        }

        void dispatch(std::function<void()> fn) const
        {
            if (fn)
            {
                try {
                    fn();
                }
                catch(...) {
                    std::unexpected();
                }
            }
        }

    public:
        template <class Fn>
        void post(Fn fn) const
        {
            {
                std::unique_lock<std::mutex> guard(pendingLock);
                pending.push(std::move(fn));
            }
            wake.notify_one();
        }

        void try_dispatch() const
        {
            std::function<void()> fn;
            {
                std::unique_lock<std::mutex> guard(pendingLock);
                if (!pending.empty())
                {
                    fn = get();
                }
            }
            dispatch(std::move(fn));
        }

        bool dispatch_one() const
        {
            std::function<void()> fn;
            {
                std::unique_lock<std::mutex> guard(pendingLock);
                wake.wait(guard, [this]{ return !pending.empty();});
                fn = get();
            }
            bool result = !!fn;
            dispatch(std::move(fn));
            return result;
        }
    };
#if defined(OBSERVE_ON_DISPATCHER_OP)
    typedef OBSERVE_ON_DISPATCHER_OP ObserveOnDispatcherOp;
#else
    typedef StdQueueDispatcher ObserveOnDispatcherOp;
#endif 

    template <class T>
    std::shared_ptr<Observable<T>> ObserveOnDispatcher(
        const std::shared_ptr<Observable<T>>& source)
    {
        auto dispatcher = std::make_shared<ObserveOnDispatcherOp>();

        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                auto cancel = std::make_shared<bool>(false);

                ComposableDisposable cd;

                cd.Add(Disposable([=]{ 
                    *cancel = true; 
                }));
                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        dispatcher->post([=]{
                            if (!*cancel)
                                observer->OnNext(element); 
                        });
                    },
                // on completed
                    [=]
                    {
                        dispatcher->post([=]{
                            if(!*cancel)
                                observer->OnCompleted(); 
                        });
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        dispatcher->post([=]{
                            if (!*cancel)
                                observer->OnError(error); 
                        });
                    }));
                return cd;
            });
    }

}

#endif
