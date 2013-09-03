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
                    auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                value(value)
            {
                if (!scheduler) 
                { 
                    scheduler = std::make_shared<CurrentThreadScheduler>(); 
                }
            }
        };
    }
    template <class T>
    const std::shared_ptr<Observable<T>> Return(
            T value,
            Scheduler::shared scheduler = nullptr
        )
    {
        return std::make_shared<detail::ReturnObservable<T>>(std::move(value), std::move(scheduler));
    }

    namespace detail
    {
        template<class T>
        class ThrowObservable : public Producer<ThrowObservable<T>, T>
        {
            typedef ThrowObservable<T> This;
            typedef std::shared_ptr<This> Parent;

            std::exception_ptr exception;
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
                            that->SinkBase::observer->OnError(local->exception);
                            that->SinkBase::Dispose();
                            return Disposable::Empty();
                    });
                }
            };

            typedef Producer<This, T> ProducerBase;
        public:

            ThrowObservable(std::exception_ptr exception, Scheduler::shared scheduler) :
                ProducerBase([](Parent parent, std::shared_ptr < Observer < T >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                exception(exception)
            {
                if (!scheduler)
                {
                    scheduler = std::make_shared<CurrentThreadScheduler>();
                }
            }
        };
    }
    template <class T>
    const std::shared_ptr<Observable<T>> Throw(
        std::exception_ptr exception,
        Scheduler::shared scheduler = nullptr
        )
    {
        return std::make_shared<detail::ThrowObservable<T>>(std::move(exception), std::move(scheduler));
    }

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

    template <class Integral>
    auto Range(
        Integral start, Integral end = std::numeric_limits<Integral>::max(), Integral step = 1,
        Scheduler::shared scheduler = nullptr)
        -> std::shared_ptr<Observable<Integral>>
    {
        if (!scheduler) {scheduler = std::make_shared<CurrentThreadScheduler>();}
        return CreateObservable<Integral>(
            [=](std::shared_ptr<Observer<Integral>> observer) -> Disposable
        {
            struct State 
            {
                bool cancel;
                Integral i;
                Integral rem;
            };
            auto state = std::make_shared<State>();
            state->cancel = false;
            state->i = start;
            state->rem = ((end - start) + step) / step;

            ComposableDisposable cd;

            cd.Add(Disposable([=]{
                state->cancel = true;
            }));

            cd.Add(scheduler->Schedule(
                fix0([=](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
            {
                if (state->cancel)
                    return Disposable::Empty();

                if (!state->rem)
                {
                    observer->OnCompleted();
                }
                else
                {
                    observer->OnNext(state->i);
                    --state->rem; 
                    state->i += step;
                    return s->Schedule(std::move(self));
                }
                return Disposable::Empty();             
            })));

            return cd;
        });
    }

    inline std::shared_ptr<Observable<size_t>> Interval(
        Scheduler::clock::duration due,
        Scheduler::shared scheduler)
    {
        return CreateObservable<size_t>(
            [=](std::shared_ptr<Observer<size_t>> observer)
            -> Disposable
            {
                struct State : public std::enable_shared_from_this<State> {
                    State(Scheduler::clock::duration due,
                        Scheduler::clock::time_point last,
                        std::shared_ptr<Observer<size_t>> observer) : due(due), last(last), cursor(0), observer(observer) {
                        cd.Add(sd);}

                    Scheduler::clock::duration due;
                    Scheduler::clock::time_point last;
                    size_t cursor;
                    std::shared_ptr<Observer<size_t>> observer;
                    ComposableDisposable cd;
                    SerialDisposable sd;

                    void Tick(Scheduler::shared s){
                        observer->OnNext(cursor); 
                        last += due;
                        ++cursor;
                        auto keepAlive = this->shared_from_this();
                        sd.Set(s->Schedule(
                            last, 
                            [this, keepAlive] (Scheduler::shared s) -> Disposable {
                                Tick(s);
                                return Disposable::Empty();
                            }
                        ));
                    }
                };
                auto state = std::make_shared<State>(due, scheduler->Now(), observer);

                state->last += state->due;
                state->sd.Set(scheduler->Schedule(
                    state->last, 
                    [=] (Scheduler::shared s) -> Disposable {
                        state->Tick(s);
                        return Disposable::Empty();
                    }
                ));
                return state->cd;
            });
    }

    using std::begin;
    using std::end;
    template <class Range>
    auto Iterate(
        Range r,
        Scheduler::shared scheduler = nullptr)
        -> std::shared_ptr<Observable<typename std::decay<decltype(*begin(r))>::type>>
    {
        typedef decltype(begin(r)) It;
        typedef typename std::decay<decltype(*begin(r))>::type T;

        if (!scheduler) {scheduler = std::make_shared<CurrentThreadScheduler>();}
        auto range = std::make_shared<Range>(std::move(r));

        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer) -> Disposable
        {
            struct State 
            {
                explicit State(std::shared_ptr<Range> rangeArg) : cancel(false) {
                    this->range = std::move(rangeArg);
                    this->r_cursor = begin(*this->range);
                    this->r_end = end(*this->range); 
                }
                bool cancel;
                std::shared_ptr<Range> range;
                It r_cursor;
                It r_end;
            };
            auto state = std::make_shared<State>(range);

            ComposableDisposable cd;

            cd.Add(Disposable([=]{
                state->cancel = true;
            }));

            cd.Add(scheduler->Schedule(
                fix0([=](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
            {
                if (state->cancel)
                    return Disposable::Empty();

                if (state->r_cursor == state->r_end)
                {
                    observer->OnCompleted();
                }
                else
                {
                    observer->OnNext(*state->r_cursor);
                    ++state->r_cursor; 
                    return s->Schedule(std::move(self));
                }
                return Disposable::Empty();             
            })));

            return cd;
        });
    }

    namespace detail
    {
        template<class T, class R>
        class UsingObservable : public Producer<UsingObservable<T, R>, T>
        {
            typedef UsingObservable<T, R> This;
            typedef std::shared_ptr<This> Parent;
            typedef std::shared_ptr<Observable < T >> Source;

        public:
            typedef std::function<R()> ResourceFactory;
            typedef std::function<Source(R)> ObservableFactory;

        private:
            ResourceFactory resourceFactory;
            ObservableFactory observableFactory;

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

                Disposable Run()
                {
                    ComposableDisposable cd;
                    Source source;
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
                    auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
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
        typedef typename observable_item<decltype(observableFactory(resourceFactory()))>::type T;
        typedef decltype(resourceFactory()) R;
        return std::make_shared<detail::UsingObservable<T, R>>(std::move(resourceFactory), std::move(observableFactory));
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

    //////////////////////////////////////////////////////////////////////
    // 
    // standard query operators

    template <class T, class S>
    auto Select(
        const std::shared_ptr<Observable<T>>& source,
        S selector
        )
    -> const std::shared_ptr<Observable<typename std::result_of<S(const T&)>::type>>
    {
        typedef typename std::result_of<S(const T&)>::type U;
        return CreateObservable<U>(
            [=](std::shared_ptr<Observer<U>> observer)
            {
                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        util::maybe<U> result;
                        try {
                            result.set(selector(element));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }
                        if (!!result) {
                            observer->OnNext(std::move(*result.get()));
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

    template <class T, class CS, class RS>
    auto SelectMany(
        const std::shared_ptr<Observable<T>>& source,
        CS collectionSelector,
        RS resultSelector)
        -> const std::shared_ptr<Observable<
            typename std::result_of<RS(const T&,
                const typename observable_item<
                    typename std::result_of<CS(const T&)>::type>::type&)>::type>>
    {
        typedef typename std::decay<typename std::result_of<CS(const T&)>::type>::type C;
        typedef typename observable_item<C>::type CI;
        typedef typename std::result_of<RS(const T&, const CI&)>::type U;

        return CreateObservable<U>(
            [=](std::shared_ptr<Observer<U>> observer)
            -> Disposable
            {
                struct State {
                    size_t subscribed;
                    bool cancel;
                    std::mutex lock;
                };
                auto state = std::make_shared<State>();
                state->cancel = false;
                state->subscribed = 0;

                ComposableDisposable cd;

                cd.Add(Disposable([=]{ 
                    std::unique_lock<std::mutex> guard(state->lock);
                    state->cancel = true; })
                );

                ++state->subscribed;
                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& sourceElement)
                    {
                        bool cancel = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            cancel = state->cancel;
                            if (!cancel) ++state->subscribed;
                        }
                        if (!cancel) {
                            util::maybe<C> collection;
                            try {
                                collection.set(collectionSelector(sourceElement));
                            } catch(...) {
                                bool cancel = false;
                                {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    cancel = state->cancel;
                                }
                                if (!cancel) {
                                    observer->OnError(std::current_exception());
                                }
                                cd.Dispose();
                            }
                            if (!!collection) {
                                cd.Add(Subscribe(
                                    *collection.get(),
                                // on next
                                    [=](const CI& collectionElement)
                                    {
                                        bool cancel = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            cancel = state->cancel;
                                        }
                                        if (!cancel) {
                                            util::maybe<U> result;
                                            try {
                                                result.set(resultSelector(sourceElement, collectionElement));
                                            } catch(...) {
                                                observer->OnError(std::current_exception());
                                                cd.Dispose();
                                            }
                                            if (!!result) {
                                                observer->OnNext(std::move(*result.get()));
                                            }
                                        }
                                    },
                                // on completed
                                    [=]
                                    {
                                        bool cancel = false;
                                        bool finished = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            finished = (--state->subscribed) == 0;
                                            cancel = state->cancel;
                                        }
                                        if (!cancel && finished)
                                            observer->OnCompleted(); 
                                    },
                                // on error
                                    [=](const std::exception_ptr& error)
                                    {
                                        bool cancel = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            --state->subscribed;
                                            cancel = state->cancel;
                                        }
                                        if (!cancel)
                                            observer->OnError(error);
                                        cd.Dispose();
                                    }));
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        bool cancel = false;
                        bool finished = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            finished = (--state->subscribed) == 0;
                            cancel = state->cancel;
                        }
                        if (!cancel && finished)
                            observer->OnCompleted(); 
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        bool cancel = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            --state->subscribed;
                            cancel = state->cancel;
                        }
                        if (!cancel)
                            observer->OnError(error);
                    }));
                return cd;
            });
    }
    
    template <class ObservableT>
    ObservableT Concat(
        const std::shared_ptr<Observable<ObservableT>>& source)
    {
        typedef typename observable_item<ObservableT>::type T;
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    bool completed;
                    bool subscribed;
                    bool cancel;
                    std::queue<ObservableT> queue;
                    Scheduler::shared scheduler;
                    std::mutex lock;
                };
                auto state = std::make_shared<State>();
                state->cancel = false;
                state->subscribed = 0;
                state->scheduler = std::make_shared<CurrentThreadScheduler>();

                ComposableDisposable cd;

                SerialDisposable sd;
                cd.Add(sd);

                cd.Add(Disposable([=]{ 
                    std::unique_lock<std::mutex> guard(state->lock);
                    state->cancel = true; })
                );

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const ObservableT& sourceElement)
                    {
                        bool cancel = false;
                        bool subscribed = false;
                        Scheduler::shared sched;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            cancel = state->cancel;
                            sched = state->scheduler;
                            if (!cancel) {
                                subscribed = state->subscribed;
                                state->queue.push(sourceElement);
                                state->subscribed = true;
                                sched = state->scheduler;
                            }
                        }
                        if (!cancel && !subscribed) {
                            sd.Set(sched->Schedule(
                                fix0([state, cd, sd, observer](Scheduler::shared s, std::function<Disposable(Scheduler::shared)> self) -> Disposable
                            {
                                bool cancel = false;
                                bool finished = false;
                                ObservableT next;
                                {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    finished = state->queue.empty();
                                    cancel = state->cancel;
                                    if (!cancel && !finished) {next = state->queue.front(); state->queue.pop();}
                                }
                                if (!cancel && !finished) {
                                    sd.Set(Subscribe(
                                        next,
                                    // on next
                                        [=](const T& t)
                                        {
                                            bool cancel = false;
                                            {
                                                std::unique_lock<std::mutex> guard(state->lock);
                                                cancel = state->cancel;
                                            }
                                            if (!cancel) {
                                                observer->OnNext(std::move(t)); 
                                            }
                                        },
                                    // on completed
                                        [=]
                                        {
                                            bool cancel = false;
                                            bool finished = false;
                                            bool subscribe = false;
                                            {
                                                std::unique_lock<std::mutex> guard(state->lock);
                                                finished = state->queue.empty() && state->completed;
                                                subscribe = !state->queue.empty();
                                                state->subscribed = subscribe;
                                                cancel = state->cancel;
                                            }
                                            if (!cancel) {
                                                if (subscribe) {sd.Set(s->Schedule(std::move(self)));}
                                                else if (finished) {observer->OnCompleted(); cd.Dispose();}
                                            }
                                        },
                                    // on error
                                        [=](const std::exception_ptr& error)
                                        {
                                            bool cancel = false;
                                            {
                                                std::unique_lock<std::mutex> guard(state->lock);
                                                cancel = state->cancel;
                                            }
                                            if (!cancel) {
                                                observer->OnError(std::current_exception());
                                            }
                                            cd.Dispose();
                                        }));
                                }
                                return Disposable::Empty();             
                            })));
                        }
                    },
                // on completed
                    [=]
                    {
                        bool cancel = false;
                        bool finished = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            state->completed = true;
                            finished = state->queue.empty() && !state->subscribed;
                            cancel = state->cancel;
                        }
                        if (!cancel && finished) {
                            observer->OnCompleted(); 
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        bool cancel = false;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            cancel = state->cancel;
                        }
                        if (!cancel) {
                            observer->OnError(std::current_exception());
                        }
                        cd.Dispose();
                    }));
                return cd;
            });
    }

    namespace detail{
        template<size_t Index, size_t SourcesSize, class SubscribeState>
        struct CombineLatestSubscriber {
            typedef typename SubscribeState::Latest Latest;
            static void subscribe(
                ComposableDisposable& cd, 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& observer, 
                const std::shared_ptr<SubscribeState>& state, 
                const typename SubscribeState::Sources& sources) {
                cd.Add(Subscribe(
                    std::get<Index>(sources),
                // on next
                    [=](const typename std::tuple_element<Index, Latest>::type& element)
                    {
                        auto local = state;
                        std::unique_lock<std::mutex> guard(local->lock);
                        std::get<Index>(local->latest) = element;
                        if (!std::get<Index>(local->latestValid)) {
                            std::get<Index>(local->latestValid) = true;
                            --local->pendingFirst;
                        }
                        if (local->pendingFirst == 0) {
                            Latest args = local->latest;
                            typedef decltype(util::tuple_dispatch(local->selector, args)) U;
                            util::maybe<U> result;
                            try {
                                result.set(util::tuple_dispatch(local->selector, args));
                            } catch(...) {
                                observer->OnError(std::current_exception());
                            }
                            if (!!result) {
                                ++state->pendingIssue;
                                {
                                    RXCPP_UNWIND_AUTO([&](){guard.lock();});
                                    guard.unlock();
                                    observer->OnNext(std::move(*result.get()));
                                }
                                --state->pendingIssue;
                            }
                            if (state->done && state->pendingIssue == 0) {
                                guard.unlock();
                                observer->OnCompleted();
                                cd.Dispose();
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        --state->pendingComplete;
                        if (state->pendingComplete == 0) {
                            state->done = true;
                            if (state->pendingIssue == 0) {
                                guard.unlock();
                                observer->OnCompleted();
                                cd.Dispose();
                            }
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                CombineLatestSubscriber<Index + 1, SourcesSize, SubscribeState>::
                    subscribe(cd, observer, state, sources);
            }
        };
        template<size_t SourcesSize, class SubscribeState>
        struct CombineLatestSubscriber<SourcesSize, SourcesSize, SubscribeState> {
            static void subscribe(
                ComposableDisposable& , 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& , 
                const std::shared_ptr<SubscribeState>& , 
                const typename SubscribeState::Sources& ) {}
        };
    }

#if RXCPP_USE_VARIADIC_TEMPLATES
    template <class... CombineLSource, class S>
    auto CombineLatest(
        S selector,
        const std::shared_ptr<Observable<CombineLSource>>&... source
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const CombineLSource&...)>::type>>
    {
        typedef typename std::result_of<S(const CombineLSource&...)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<CombineLSource>>...> Sources;
        typedef std::tuple<CombineLSource...> Latest;
        typedef decltype(std::make_tuple((source, true)...)) LatestValid;
        struct State {
            typedef Latest Latest;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : latestValid()
                , pendingFirst(SourcesSize::value)
                , pendingIssue(0)
                , pendingComplete(SourcesSize::value)
                , done(false)
                , selector(std::move(selector))
            {}
            std::mutex lock;
            LatestValid latestValid;
            size_t pendingFirst;
            size_t pendingIssue;
            size_t pendingComplete;
            bool done;
            S selector;
            Latest latest;
        };
        Sources sources(source...);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(selector));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::CombineLatestSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#else
    template <class CombineLSource1, class CombineLSource2, class S>
    auto CombineLatest(
        S selector,
        const std::shared_ptr<Observable<CombineLSource1>>& source1,
        const std::shared_ptr<Observable<CombineLSource2>>& source2
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const CombineLSource1&, const CombineLSource2&)>::type>>
    {
        typedef typename std::result_of<S(const CombineLSource1&, const CombineLSource2&)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<CombineLSource1>>, std::shared_ptr<Observable<CombineLSource2>>> Sources;
        typedef std::tuple<CombineLSource1, CombineLSource2> Latest;
        typedef std::tuple<bool, bool> LatestValid;
        struct State {
            typedef Latest Latest;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : latestValid()
                , pendingFirst(SourcesSize::value)
                , pendingIssue(0)
                , pendingComplete(SourcesSize::value)
                , done(false)
                , selector(std::move(selector))
            {}
            std::mutex lock;
            LatestValid latestValid;
            size_t pendingFirst;
            size_t pendingIssue;
            size_t pendingComplete;
            bool done;
            S selector;
            Latest latest;
        };
        Sources sources(source1, source2);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(std::move(selector)));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::CombineLatestSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#endif

    namespace detail{
        template<size_t Index, size_t SourcesSize, class SubscribeState>
        struct ZipSubscriber {
            typedef typename std::tuple_element<Index, typename SubscribeState::Queues>::type::value_type Item;
            typedef std::shared_ptr<Observer<typename SubscribeState::result_type>> ResultObserver;
            struct Next
            {
                std::unique_lock<std::mutex>& guard;
                std::shared_ptr<SubscribeState> state;
                const ResultObserver& observer;
                const ComposableDisposable& cd;
                explicit Next(
                    std::unique_lock<std::mutex>& guard,
                    std::shared_ptr<SubscribeState> state,
                    const ResultObserver& observer,
                    const ComposableDisposable& cd)
                    : guard(guard)
                    , state(std::move(state))
                    , observer(observer)
                    , cd(cd) {
                }
#if RXCPP_USE_VARIADIC_TEMPLATES
                template<class... ZipQueue>
                void operator()(ZipQueue&... queue) {
                    // build array of bool that we can iterate to detect empty queues
                    bool empties[] = {queue.empty()...};
                    if (std::find(std::begin(empties), std::end(empties), true) == std::end(empties)) {
                        // all queues have an item.
                        //
                        // copy front of each queue
                        auto args = std::make_tuple(queue.front()...);
                        
                        // cause side-effect of pop on each queue
                        std::make_tuple((queue.pop(), true)...);
                        
                        typedef decltype(util::tuple_dispatch(state->selector, args)) U;
                        util::maybe<U> result;
                        try {
                            result.set(util::tuple_dispatch(state->selector, args));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }
                        if (!!result) {
                            ++state->pending;
                            {
                                RXCPP_UNWIND_AUTO([&](){guard.lock();});
                                guard.unlock();
                                observer->OnNext(std::move(*result.get()));
                            }
                            --state->pending;
                        }
                    }
                    // build new array to check for any empty queue
                    bool post_empties[] = {queue.empty()...};
                    if (state->completed && state->pending == 0 &&
                        std::find(std::begin(post_empties), std::end(post_empties), true) != std::end(post_empties)) {
                        // at least one queue is empty and at least one of the sources has completed.
                        // it is time to stop.
                        
                        RXCPP_UNWIND_AUTO([&](){guard.lock();});
                        guard.unlock();
                        observer->OnCompleted();
                        cd.Dispose();
                    }
                }
#else
                template<class ZipQueue1, class ZipQueue2>
                void operator()(ZipQueue1& queue1, ZipQueue2& queue2) {
                    // build array of bool that we can iterate to detect empty queues
                    bool empties[] = {queue1.empty(), queue2.empty()};
                    if (std::find(std::begin(empties), std::end(empties), true) == std::end(empties)) {
                        // all queues have an item.
                        //
                        // copy front of each queue
                        auto args = std::make_tuple(queue1.front(), queue2.front());
                        
                        queue1.pop();
                        queue2.pop();
                        
                        typedef decltype(util::tuple_dispatch(state->selector, args)) U;
                        util::maybe<U> result;
                        try {
                            result.set(util::tuple_dispatch(state->selector, args));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }
                        if (!!result) {
                            ++state->pending;
                            {
                                RXCPP_UNWIND_AUTO([&](){guard.lock();});
                                guard.unlock();
                                observer->OnNext(std::move(*result.get()));
                            }
                            --state->pending;
                        }
                    }
                    // build new array to check for any empty queue
                    bool post_empties[] = {queue1.empty(), queue2.empty()};
                    if (state->completed && state->pending == 0 &&
                        std::find(std::begin(post_empties), std::end(post_empties), true) != std::end(post_empties)) {
                        // at least one queue is empty and at least one of the sources has completed.
                        // it is time to stop.
                        
                        RXCPP_UNWIND_AUTO([&](){guard.lock();});
                        guard.unlock();
                        observer->OnCompleted();
                        cd.Dispose();
                    }
                }
#endif //RXCPP_USE_VARIADIC_TEMPLATES
            };
            static void subscribe(
                ComposableDisposable& cd, 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& observer, 
                const std::shared_ptr<SubscribeState>& state, 
                const typename SubscribeState::Sources& sources) {
                cd.Add(Subscribe(
                    std::get<Index>(sources),
                // on next
                    [=](const Item& element)
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        std::get<Index>(state->queues).push(element);
                        Next next(guard, state, observer, cd);
                        util::tuple_dispatch(next, state->queues);
                    },
                // on completed
                    [=]
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        state->completed = true;
                        Next next(guard, state, observer, cd);
                        util::tuple_dispatch(next, state->queues);
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                ZipSubscriber<Index + 1, SourcesSize, SubscribeState>::
                    subscribe(cd, observer, state, sources);
            }
        };
        template<size_t SourcesSize, class SubscribeState>
        struct ZipSubscriber<SourcesSize, SourcesSize, SubscribeState> {
            static void subscribe(
                ComposableDisposable& ,
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& , 
                const std::shared_ptr<SubscribeState>& , 
                const typename SubscribeState::Sources& ) {}
        };
    }
#if RXCPP_USE_VARIADIC_TEMPLATES
    template <class... ZipSource, class S>
    auto Zip(
        S selector,
        const std::shared_ptr<Observable<ZipSource>>&... source
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const ZipSource&...)>::type>>
    {
        typedef typename std::result_of<S(const ZipSource&...)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<ZipSource>>...> Sources;
        typedef std::tuple<std::queue<ZipSource>...> Queues;
        struct State {
            typedef Queues Queues;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : selector(std::move(selector))
                , pending(0)
                , completed(false)
            {}
            std::mutex lock;
            S selector;
            size_t pending;
            bool completed;
            Queues queues;
        };
        Sources sources(source...);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(std::move(selector)));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::ZipSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#else
    template <class ZipSource1, class ZipSource2, class S>
    auto Zip(
        S selector,
        const std::shared_ptr<Observable<ZipSource1>>& source1,
        const std::shared_ptr<Observable<ZipSource2>>& source2
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const ZipSource1&, const ZipSource2&)>::type>>
    {
        typedef typename std::result_of<S(const ZipSource1&, const ZipSource2&)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<ZipSource1>>, std::shared_ptr<Observable<ZipSource2>>> Sources;
        typedef std::tuple<std::queue<ZipSource1>, std::queue<ZipSource2>> Queues;
        struct State {
            typedef Queues Queues;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : selector(std::move(selector))
                , pending(0)
                , completed(false)
            {}
            std::mutex lock;
            S selector;
            size_t pending;
            bool completed;
            Queues queues;
        };
        Sources sources(source1, source2);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(std::move(selector)));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::ZipSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#endif //RXCPP_USE_VARIADIC_TEMPLATES

    namespace detail{
        template<size_t Index, size_t SourcesSize, class SubscribeState>
        struct MergeSubscriber {
            typedef typename SubscribeState::result_type Item;
            typedef std::shared_ptr<Observer<typename SubscribeState::result_type>> ResultObserver;
            static void subscribe(
                ComposableDisposable& cd, 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& observer, 
                const std::shared_ptr<SubscribeState>& state,
                const typename SubscribeState::Sources& sources) {
                cd.Add(Subscribe(
                    std::get<Index>(sources),
                // on next
                    [=](const Item& element)
                    {
                        observer->OnNext(element);
                    },
                // on completed
                    [=]
                    {
                        if (--state->pendingComplete == 0) {
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                MergeSubscriber<Index + 1, SourcesSize, SubscribeState>::
                    subscribe(cd, observer, state, sources);
            }
        };
        template<size_t SourcesSize, class SubscribeState>
        struct MergeSubscriber<SourcesSize, SourcesSize, SubscribeState> {
            static void subscribe(
                ComposableDisposable& , 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& , 
                const std::shared_ptr<SubscribeState>& ,
                const typename SubscribeState::Sources& ) {}
        };
    }
#if RXCPP_USE_VARIADIC_TEMPLATES
    template <class MergeSource, class... MergeSourceNext>
    std::shared_ptr<Observable<MergeSource>> Merge(
        const std::shared_ptr<Observable<MergeSource>>& firstSource,
        const std::shared_ptr<Observable<MergeSourceNext>>&... otherSource
        )
    {
        typedef MergeSource result_type;
        typedef decltype(std::make_tuple(firstSource, otherSource...)) Sources;
        struct State {
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            State()
                : pendingComplete(SourcesSize::value)
            {}
            std::atomic<size_t> pendingComplete;
        };
        Sources sources(firstSource, otherSource...);
        // bug on osx prevents using make_shared
        std::shared_ptr<State> state(new State());
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::MergeSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#else
    template <class MergeSource, class MergeSourceNext>
    std::shared_ptr<Observable<MergeSource>> Merge(
        const std::shared_ptr<Observable<MergeSource>>& firstSource,
        const std::shared_ptr<Observable<MergeSourceNext>>& otherSource
        )
    {
        typedef MergeSource result_type;
        typedef decltype(std::make_tuple(firstSource, otherSource)) Sources;
        struct State {
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            State()
                : pendingComplete(SourcesSize::value)
            {}
            std::atomic<size_t> pendingComplete;
        };
        Sources sources(firstSource, otherSource);
        // bug on osx prevents using make_shared
        std::shared_ptr<State> state(new State());
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::MergeSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#endif //RXCPP_USE_VARIADIC_TEMPLATES

    template <class T, class P>
    const std::shared_ptr<Observable<T>> Where(
        const std::shared_ptr<Observable<T>>& source,
        P predicate
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            {
                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        typedef decltype(predicate(element)) U;
                        util::maybe<U> result;
                        try {
                            result.set(predicate(element));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }
                        if (!!result && *result.get())
                        {
                            observer->OnNext(element);
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

    template <class T, class KS, class VS, class L>
    auto GroupBy(
        const std::shared_ptr<Observable<T>>& source,
        KS keySelector,
        VS valueSelector,
        L less) 
        -> std::shared_ptr<Observable<std::shared_ptr<GroupedObservable<
            typename std::decay<decltype(keySelector((*(T*)0)))>::type, 
            typename std::decay<decltype(valueSelector((*(T*)0)))>::type>>>>
    {
        typedef typename std::decay<decltype(keySelector((*(T*)0)))>::type Key;
        typedef typename std::decay<decltype(valueSelector((*(T*)0)))>::type Value;

        typedef std::shared_ptr<GroupedObservable<Key, Value>> LocalGroupObservable;

        return CreateObservable<LocalGroupObservable>(
            [=](std::shared_ptr<Observer<LocalGroupObservable>> observer) -> Disposable
            {
                typedef std::map<Key, std::shared_ptr<GroupedSubject<Key, Value>>, L> Groups;

                struct State
                {
                    explicit State(L less) : groups(std::move(less)) {}
                    std::mutex lock;
                    Groups groups;
                };
                auto state = std::make_shared<State>(std::move(less));

                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        util::maybe<Key> key;
                        try {
                            key.set(keySelector(element));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }

                        if (!!key) {
                            auto keySubject = CreateGroupedSubject<Value>(*key.get());

                            typename Groups::iterator groupIt;
                            bool newGroup = false;

                            {
                                std::unique_lock<std::mutex> guard(state->lock);
                                std::tie(groupIt, newGroup) = state->groups.insert(
                                    std::make_pair(*key.get(), keySubject)
                                );
                            }

                            if (newGroup)
                            {
                                LocalGroupObservable nextGroup(std::move(keySubject));
                                observer->OnNext(nextGroup);
                            }

                            util::maybe<Value> result;
                            try {
                                result.set(valueSelector(element));
                            } catch(...) {
                                observer->OnError(std::current_exception());
                            }
                            if (!!result) {
                                groupIt->second->OnNext(std::move(*result.get()));
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        for(auto& group : state->groups) {
                            group.second->OnCompleted();
                        }
                        observer->OnCompleted();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        for(auto& group : state->groups) {
                            group.second->OnError(error);
                        }
                        observer->OnError(error);
                    });
            });
    }


    template <class T, class MulticastSubject>
    std::shared_ptr<ConnectableObservable<T>> Multicast(const std::shared_ptr < Observable < T >> &source, const std::shared_ptr<MulticastSubject>& multicastSubject)
    {
        return std::static_pointer_cast<ConnectableObservable<T>>(
            std::make_shared < ConnectableSubject < std::shared_ptr < Observable < T >> , std::shared_ptr<MulticastSubject> >> (source, multicastSubject));
    }

    template <class T>
    std::shared_ptr<ConnectableObservable<T>> Publish(const std::shared_ptr < Observable < T >> &source)
    {
        auto multicastSubject = std::make_shared<Subject<T>>();
        return Multicast(source, multicastSubject);
    }

    template <class T, class V>
    std::shared_ptr<ConnectableObservable<T>> Publish(const std::shared_ptr < Observable < T >> &source, V value)
    {
        auto multicastSubject = std::make_shared<BehaviorSubject<T>>(value);
        return Multicast(source, multicastSubject);
    }

    template <class T>
    std::shared_ptr<ConnectableObservable<T>> PublishLast(const std::shared_ptr < Observable < T >> &source)
    {
        auto multicastSubject = std::make_shared<AsyncSubject<T>>();
        return Multicast(source, multicastSubject);
    }

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
                    auto sink = std::shared_ptr<_>(new _(that, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return sink->Run();
                }),
                refcount(0),
                source(std::move(source))
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

    template <class T>
    const std::shared_ptr<Observable<T>> ConnectForever(
        const std::shared_ptr<ConnectableObservable<T>>& source
        )
    {
        source->Connect();
        return observable(source);
    }

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

        private:

            Source source;
            A seed;
            Accumulator accumulator;

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
                            accumulation.set(parent->accumulator(parent->seed, t));
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

            ScanObservable(Source source, A seed, Accumulator accumulator) :
                ProducerBase([this](Parent parent, std::shared_ptr < Observer < A >> observer, Disposable && cancel, typename ProducerBase::SetSink setSink) -> Disposable
                {
                    auto sink = std::shared_ptr<_>(new _(parent, observer, std::move(cancel)));
                    setSink(sink->GetDisposable());
                    return this->source->Subscribe(sink);
                }),
                source(std::move(source)),
                seed(std::move(seed)),
                accumulator(std::move(accumulator))
            {
            }
        };
    }
    template <class T, class A>
    const std::shared_ptr<Observable<A>> Scan(
        const std::shared_ptr<Observable<T>>& source,
        A seed,
        typename detail::ScanObservable<T, A>::Accumulator accumulator
        )
    {
        return std::make_shared<detail::ScanObservable<T, A>>(std::move(source), std::move(seed), std::move(accumulator));
    }

    template <class T, class Integral>
    std::shared_ptr<Observable<T>> Take(
        const std::shared_ptr<Observable<T>>& source,
        Integral n 
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                // keep count of remaining calls received OnNext and count of OnNext calls issued.
                auto remaining = std::make_shared<std::tuple<std::atomic<Integral>, std::atomic<Integral>>>(n, n);

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        auto local = --std::get<0>(*remaining);
                        RXCPP_UNWIND_AUTO([&](){
                            if (local >= 0){
                                // all elements received
                                if (--std::get<1>(*remaining) == 0) {
                                    // all elements passed on to observer.
                                    observer->OnCompleted();
                                    cd.Dispose();}}});

                        if (local >= 0) {
                            observer->OnNext(element);
                        } 
                    },
                // on completed
                    [=]
                    {
                        if (std::get<1>(*remaining) == 0 && std::get<0>(*remaining) <= 0) {
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }

    template <class T, class U>
    std::shared_ptr<Observable<T>> TakeUntil(
        const std::shared_ptr<Observable<T>>& source,
        const std::shared_ptr<Observable<U>>& terminus
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {

                struct TerminusState {
                    enum type {
                        Live,
                        Terminated
                    };
                };
                struct TakeState {
                    enum type {
                        Taking,
                        Completed
                    };
                };
                struct State {
                    State() : terminusState(TerminusState::Live), takeState(TakeState::Taking) {}
                    std::atomic<typename TerminusState::type> terminusState;
                    std::atomic<typename TakeState::type> takeState;
                };
                auto state = std::make_shared<State>();

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    terminus,
                // on next
                    [=](const T& element)
                    {
                        state->terminusState = TerminusState::Terminated;
                    },
                // on completed
                    [=]
                    {
                        state->terminusState = TerminusState::Terminated;
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        state->terminusState = TerminusState::Terminated;
                    }));

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (state->terminusState == TerminusState::Live) {
                            observer->OnNext(element);
                        } else if (state->takeState.exchange(TakeState::Completed) == TakeState::Taking) {
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on completed
                    [=]
                    {
                        if (state->takeState.exchange(TakeState::Completed) == TakeState::Taking) {
                            state->terminusState = TerminusState::Terminated;
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        state->takeState = TakeState::Completed;
                        state->terminusState = TerminusState::Terminated;
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }

    template <class T, class Integral>
    std::shared_ptr<Observable<T>> Skip(
        const std::shared_ptr<Observable<T>>& source,
        Integral n
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    enum type {
                        Skipping,
                        Forwarding
                    };
                };
                // keep count of remaining OnNext calls to skip and state.
                auto remaining = std::make_shared<std::tuple<std::atomic<Integral>, std::atomic<typename State::type>>>(n, State::Skipping);

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (std::get<1>(*remaining) == State::Forwarding) {
                            observer->OnNext(element);
                        } else {
                            auto local = --std::get<0>(*remaining);

                            if (local == 0) {
                                std::get<1>(*remaining) = State::Forwarding;
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }

    template <class T, class U>
    std::shared_ptr<Observable<T>> SkipUntil(
        const std::shared_ptr<Observable<T>>& source,
        const std::shared_ptr<Observable<U>>& terminus
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct SkipState {
                    enum type {
                        Skipping,
                        Taking
                    };
                };
                struct State {
                    State() : skipState(SkipState::Skipping) {}
                    std::atomic<typename SkipState::type> skipState;
                };
                auto state = std::make_shared<State>();

                ComposableDisposable cd;

                cd.Add(Subscribe(
                    terminus,
                // on next
                    [=](const T& element)
                    {
                        state->skipState = SkipState::Taking;
                    },
                // on completed
                    [=]
                    {
                        state->skipState = SkipState::Taking;
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        state->skipState = SkipState::Taking;
                    }));

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (state->skipState == SkipState::Taking) {
                            observer->OnNext(element);
                        }
                    },
                // on completed
                    [=]
                    {
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
            });
    }


    template <class StdCollection>
    const std::shared_ptr<Observable<StdCollection>> ToStdCollection(
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

    //////////////////////////////////////////////////////////////////////
    // 
    // time

    template <class T>
    std::shared_ptr<Observable<T>> Delay(
        const std::shared_ptr<Observable<T>>& source,
        Scheduler::clock::duration due,
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                auto cancel = std::make_shared<bool>(false);

                ComposableDisposable cd;

                cd.Add(Disposable([=]{ 
                    *cancel = true; }));

                SerialDisposable sd;
                auto wsd = cd.Add(sd);

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        auto sched_disposable = scheduler->Schedule(
                            due, 
                            [=] (Scheduler::shared) -> Disposable { 
                                if (!*cancel)
                                    observer->OnNext(element); 
                                return Disposable::Empty();
                            }
                        );
                        auto ssd = wsd.lock();
                        if (ssd)
                        {
                            *ssd.get() = std::move(sched_disposable);
                        }
                    },
                // on completed
                    [=]
                    {
                        auto sched_disposable = scheduler->Schedule(
                            due, 
                            [=](Scheduler::shared) -> Disposable { 
                                if (!*cancel)
                                    observer->OnCompleted(); 
                                return Disposable::Empty();
                            }
                        );
                        auto ssd = wsd.lock();
                        if (ssd)
                        {
                            *ssd.get() = std::move(sched_disposable);
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        if (!*cancel)
                            observer->OnError(error);
                    }));
                return cd;
            });
    }

    template <class T>
    std::shared_ptr<Observable<T>> Throttle(
        const std::shared_ptr<Observable<T>>& source,
        Scheduler::clock::duration due,
        Scheduler::shared scheduler)
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    State() : hasValue(false), id(0) {}
                    std::mutex lock;
                    T value;
                    bool hasValue;
                    size_t id;
                };
                auto state = std::make_shared<State>();

                ComposableDisposable cd;

                SerialDisposable sd;
                cd.Add(sd);

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        size_t current = 0;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            state->hasValue = true;
                            state->value = std::move(element);
                            current = ++state->id;
                        }
                        sd.Set(scheduler->Schedule(
                            due, 
                            [=] (Scheduler::shared) -> Disposable { 
                                {
                                    std::unique_lock<std::mutex> guard(state->lock);
                                    if (state->hasValue && state->id == current) {
                                        observer->OnNext(std::move(state->value));
                                    }
                                    state->hasValue = false;
                                }

                                return Disposable::Empty();
                            }
                        ));
                    },
                // on completed
                    [=]
                    {
                        bool sendValue = false;
                        T value;
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            sendValue = state->hasValue;
                            if (sendValue) {
                                value = std::move(state->value);}
                            state->hasValue = false;
                            ++state->id;
                        }
                        if (sendValue) {
                            observer->OnNext(std::move(value));}
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            state->hasValue = false;
                            ++state->id;
                        }
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                return cd;
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

    // removes duplicate-sequenced values. e.g. 1,2,2,3,1 ==> 1,2,3,1
    template <class T>
    std::shared_ptr<Observable<T>> DistinctUntilChanged(
        const std::shared_ptr<Observable<T>>& source)
    {   
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                struct State {
                    State() : last(), hasValue(false) {}
                    T last; bool hasValue;
                };
        
                auto state = std::make_shared<State>();
                state->hasValue = false;

                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (!state->hasValue || !(state->last == element))
                        {
                            observer->OnNext(element);
                            state->last = element;
                            state->hasValue = true;
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
