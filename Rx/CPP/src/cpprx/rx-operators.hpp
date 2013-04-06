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
            if (CurrentThreadScheduler::IsScheduleRequired()) {
                auto scheduler = std::make_shared<CurrentThreadScheduler>();
                return scheduler->Schedule(
                   [=](Scheduler::shared) -> Disposable {
                        try {
                            return subscribe(observer);
                        } catch (...) {
                            observer->OnError(std::current_exception());
                        }   
                        return Disposable::Empty();
                   }
                );
            }
            try {
                return subscribe(observer);
            } catch (...) {
                observer->OnError(std::current_exception());
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
            try 
            {
                if(onNext)
                {
                    onNext(element);
                }
            }
            catch (...)
            {
                OnError(std::current_exception());
            }
        }
        virtual void OnCompleted() 
        {
            if(onCompleted)
            {
                onCompleted();
                clear();
            }         
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            if(onError)
            {
                onError(error);
                clear();
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


    template <class T, class Base, class Subject>
    class ObservableSubject : 
        public Base,
        public std::enable_shared_from_this<Subject>
    {
    protected:
        std::mutex lock;
        std::vector<std::shared_ptr<Observer<T>>> observers;
        
        virtual ~ObservableSubject() {
            // putting this first means that the observers
            // will be destructed outside the lock
            std::vector<std::shared_ptr<Observer<T>>> empty;

            std::unique_lock<decltype(lock)> guard(lock);
            using std::swap;
            swap(observers, empty);
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
                for(auto& o : observers)
                {
                    if (!o){
                        o = std::move(observer);
                        return d;
                    }
                }
                observers.push_back(std::move(observer));
            }
            return d;
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
                try 
                {
                    if (o) {
                        o->OnNext(element);
                    }
                }
                catch (...)
                {
                    o->OnError(std::current_exception());
                    Base::RemoveObserver(o);
                }
            }
        }
        virtual void OnCompleted() 
        {
            std::unique_lock<decltype(Base::lock)> guard(Base::lock);
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
        T value;
        std::vector<std::shared_ptr<Observer<T>>> observers;

        void RemoveObserver(std::shared_ptr<Observer<T>> toRemove)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto it = std::find(begin(observers), end(observers), toRemove);
            if (it != end(observers))
                *it = nullptr;
        }

        BehaviorSubject();
    public:

        explicit BehaviorSubject(T t) : value(std::move(t)) {}
        
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

            try 
            {
                {
                    std::unique_lock<decltype(lock)> guard(lock);
                    for(auto& o : observers)
                    {
                        if (!o){
                            o = std::move(observer);
                            return d;
                        }
                    }
                    observers.push_back(observer);
                }
                observer->OnNext(value);
            }
            catch (...)
            {
                observer->OnError(std::current_exception());
                RemoveObserver(observer);
            }

            return d;
        }

        virtual void OnNext(T element)
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto local = observers;
            value = std::move(element);
            guard.unlock();
            for(auto& o : local)
            {
                try 
                {
                    if (o) {
                        o->OnNext(value);
                    }
                }
                catch (...)
                {
                    o->OnError(std::current_exception());
                    RemoveObserver(o);
                }
            }
        }
        virtual void OnCompleted() 
        {
            std::unique_lock<decltype(lock)> guard(lock);
            auto local = std::move(observers);
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
            std::unique_lock<decltype(lock)> guard(lock);
            auto local = std::move(observers);
            guard.unlock();
            for(auto& o : local)
            {
                if (o) {
                    o->OnError(error);
                }
            }
        }
    };

    template <class T, class Arg>
    std::shared_ptr<BehaviorSubject<T>> CreateBehaviorSubject(Arg a)
    {
        return std::make_shared<BehaviorSubject<T>>(std::move(a));
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
                try {
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
                } catch (...) {
                    observer->OnError(std::current_exception());
                }   
                return Disposable::Empty();             
            })));

            return cd;
        });
    }

    std::shared_ptr<Observable<size_t>> Interval(
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
                    SharedDisposable sd;

                    void Tick(Scheduler::shared s){
                        observer->OnNext(cursor); 
                        last += due;
                        ++cursor;
                        auto keepAlive = this->shared_from_this();
                        sd.Set(s->Schedule(
                            last, 
                            [this, keepAlive] (Scheduler::shared s){
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
                    [=] (Scheduler::shared s){
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
                try {
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
                } catch (...) {
                    observer->OnError(std::current_exception());
                }   
                return Disposable::Empty();             
            })));

            return cd;
        });
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
        auto observer = CreateObserver<T>(std::move(onNext), [&]{
            std::unique_lock<std::mutex> guard(lock);
            done = true;
            wake.notify_one();
        }, [&](const std::exception_ptr& e){
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
                        auto result = selector(element);
                        observer->OnNext(std::move(result));
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
        typedef typename std::result_of<CS(const T&)>::type C;
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
                            try {
                                auto collection = collectionSelector(sourceElement);
                                cd.Add(Subscribe(
                                    collection,
                                // on next
                                    [=](const CI& collectionElement)
                                    {
                                        bool cancel = false;
                                        {
                                            std::unique_lock<std::mutex> guard(state->lock);
                                            cancel = state->cancel;
                                        }
                                        try {
                                            if (!cancel) {
                                                auto result = resultSelector(sourceElement, collectionElement);
                                                observer->OnNext(std::move(result)); 
                                            }
                                        } catch (...) {
                                            observer->OnError(std::current_exception());
                                            cd.Dispose();
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
                            } catch (...) {
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

                SharedDisposable sd;
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
                                try {
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
                                                try {
                                                    if (!cancel) {
                                                        observer->OnNext(std::move(t)); 
                                                    }
                                                } catch (...) {
                                                    observer->OnError(std::current_exception());
                                                    cd.Dispose();
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
                                } catch (...) {
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
                        static bool pending = true; 
                        auto local = state;
                        std::unique_lock<std::mutex> guard(local->lock);
                        std::get<Index>(local->latest) = element;
                        if (!std::get<Index>(local->latestValid)) {
                            std::get<Index>(local->latestValid) = true;
                            --local->pendingFirst;
                        }
                        if (local->pendingFirst == 0) {
                            Latest args = local->latest;
                            auto result = util::tuple_dispatch(local->selector, args);
                            ++state->pendingIssue;
                            {
                                RXCPP_UNWIND_AUTO([&](){guard.lock();});
                                guard.unlock();
                                observer->OnNext(std::move(result));
                            }
                            --state->pendingIssue;
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
                        
                        ++state->pending;
                        auto result = util::tuple_dispatch(state->selector, args);
                        {
                            RXCPP_UNWIND_AUTO([&](){guard.lock();});
                            guard.unlock();
                            observer->OnNext(std::move(result));
                        }
                        --state->pending;
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
                        
                        ++state->pending;
                        auto result = util::tuple_dispatch(state->selector, args);
                        {
                            RXCPP_UNWIND_AUTO([&](){guard.lock();});
                            guard.unlock();
                            observer->OnNext(std::move(result));
                        }
                        --state->pending;
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
                        auto result = predicate(element);
                        if (result)
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
            [=](std::shared_ptr<Observer<LocalGroupObservable>> observer)
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
                        auto key = keySelector(element);
                        auto keySubject = CreateGroupedSubject<Value>(key);

                        typename Groups::iterator groupIt;
                        bool newGroup = false;

                        {
                            std::unique_lock<std::mutex> guard(state->lock);
                            std::tie(groupIt, newGroup) = state->groups.insert(
                                std::make_pair(key,keySubject)
                            );
                        }

                        if (newGroup)
                        {
                            LocalGroupObservable nextGroup(std::move(keySubject));
                            observer->OnNext(nextGroup);
                        }
                        auto result = valueSelector(element);
                        groupIt->second->OnNext(std::move(result));
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
            [=](std::shared_ptr<Observer<StdCollection>> observer)
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

                cd.Add(Disposable([=]{ *cancel = true; }));

                SharedDisposable sd;
                cd.Add(sd);

                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        sd.Set(scheduler->Schedule(
                            due, 
                            [=] (Scheduler::shared){ 
                                if (!*cancel)
                                    observer->OnNext(element); 
                                return Disposable::Empty();
                            }
                        ));
                    },
                // on completed
                    [=]
                    {
                        sd.Set(scheduler->Schedule(
                            due, 
                            [=](Scheduler::shared){ 
                                if (!*cancel)
                                    observer->OnCompleted(); 
                                return Disposable::Empty();
                            }
                        ));
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

                SharedDisposable sd;
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
                            [=] (Scheduler::shared){ 
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

                SharedDisposable sd;
                cd.Add(sd);

                cd.Add(scheduler->Schedule([=](Scheduler::shared){
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
