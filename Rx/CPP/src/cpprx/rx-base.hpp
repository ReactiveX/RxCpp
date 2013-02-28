// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPRX_RX_BASE_HPP)
#define CPPRX_RX_BASE_HPP
#pragma once

namespace rxcpp
{
    //////////////////////////////////////////////////////////////////////
    // 
    // Abstract interfaces

    template <class T>
    struct Observer
    {
        virtual void OnNext(const T&) {};
        virtual void OnCompleted() {};
        virtual void OnError(const std::exception_ptr&) {};

        virtual ~Observer() {}
    };

    class Disposable
    {
        std::function<void()> dispose;
    public:
        Disposable(std::function<void()> dispose) : dispose(std::move(dispose)) 
        {
        }
        void Dispose()
        {
            if (dispose) dispose();
        }
    };

    template <class T>
    struct Observable
    {
        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer) = 0;
        virtual ~Observable() {}
    };



    //////////////////////////////////////////////////////////////////////
    // 
    // constructors

    template <class T, class S>
    class CreatedObservable : public Observable<T>
    {
        S subscribe;

    public:
        CreatedObservable(S subscribe) : subscribe(std::move(subscribe))
        {
        }
        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            return subscribe(std::move(observer));
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

    template <class T>
    class Subject : 
        public Observable<T>, 
        public Observer<T>, 
        public std::enable_shared_from_this<Subject<T>>
    {
        std::vector<std::shared_ptr<Observer<T>>> observers;
    public:
        virtual void OnNext(const T& element)
        {
            for(auto& o : observers)
            {
                try 
                {
                    if (o)
                        o->OnNext(element);
                }
                catch (...)
                {
                    auto o_ = std::move(o);
                    o_->OnError(std::current_exception());
                }
            }
        }
        virtual void OnCompleted() 
        {
            for(auto& o : observers)
            {
                if (o) {
                    o->OnCompleted();
                    o = nullptr;
                }
            }
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            for(auto& o : observers)
            {
                if (o) {
                    o->OnError(error);
                    o = nullptr;
                }
            }
        }
        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            std::weak_ptr<Observer<T>> wptr = observer;
            std::weak_ptr<Subject> wself = this->shared_from_this();

            Disposable d = [wptr, wself]{
                if (auto self = wself.lock())
                {
                    self->RemoveObserver(wptr.lock());
                }
            };

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

    private:
        void RemoveObserver(std::shared_ptr<Observer<T>> toRemove)
        {
            auto it = std::find(begin(observers), end(observers), toRemove);
            if (it != end(observers))
                *it = nullptr;
        }
    };

    template <class T>
    std::shared_ptr<Subject<T>> CreateSubject()
    {
        return std::make_shared<Subject<T>>();
    }


    //////////////////////////////////////////////////////////////////////
    // 
    // imperative functions

    template <class T>
    Disposable Subscribe(
        const std::shared_ptr<Observable<T>>& source,
        typename util::identity<std::function<void(const T&)>>::type onNext,
        std::function<void()> onCompleted = nullptr,
        std::function<void(const std::exception_ptr&)> onError = nullptr
        )
    {
        auto observer = CreateObserver<T>(std::move(onNext), std::move(onCompleted), std::move(onError));
        
        return source->Subscribe(observer);
    }

    // reference handle type for a container for composing disposables
    class ComposableDisposable 
    {
        struct State
        {
            std::vector<Disposable> disposables;
            std::mutex lock;
            bool isDisposed;

            State() : isDisposed(false)
            {
            }
            void Add(Disposable&& d)
            {
                std::unique_lock<decltype(lock)> guard(lock);
                if (isDisposed) {
                    guard.unlock();
                    d.Dispose();
                } else {
                    disposables.push_back(std::move(d));
                }
            }
            void Dispose() 
            {
                std::unique_lock<decltype(lock)> guard(lock);

                isDisposed = true;
                auto v = std::move(disposables);
                guard.unlock();

                std::for_each(v.begin(), v.end(),
                    [](Disposable& d) { d.Dispose(); });
            }
        };

        std::shared_ptr<State> state;

    public:
        ComposableDisposable() : state(new State)
        {
        }
        void Add(Disposable d) const
        {
            state->Add(std::move(d));
        }
        void Dispose() const
        {
            state->Dispose();
        }
        operator Disposable() const
        {
            auto d = Disposable([=]{ 
                state->Dispose(); 
            });
            return d;
        }
    };



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
    template <class T>
    std::shared_ptr<Observable<T>> Take(
        const std::shared_ptr<Observable<T>>& source,
        int n // TODO: long long?
        )    
    {
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer) 
            -> Disposable
            {
                auto remaining = std::make_shared<int>(n);

                ComposableDisposable cd;

                auto d = Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (*remaining)
                        {
                            observer->OnNext(element);
                            if (--*remaining == 0)
                            {
                                observer->OnCompleted();
                                cd.Dispose();
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        if (*remaining)
                        {
                            observer->OnCompleted();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        if (*remaining)
                        {
                            observer->OnError(error);
                        }
                    });
                cd.Add(std::move(d));
                return cd;
            });
    }



    //////////////////////////////////////////////////////////////////////
    // 
    // time and schedulers

    struct DefaultScheduler
    {
    private: DefaultScheduler(const DefaultScheduler&);
    public:
        static DefaultScheduler& Instance()
        {
            // TODO: leaks. race condition on atexit though.
            static DefaultScheduler* instance = new DefaultScheduler;
            return *instance;
        }

        typedef std::function<void()> Work;

        std::atomic<long> trampoline;
        
        std::vector<Work> queue;
        
        std::thread worker_thread;
        DefaultScheduler() : trampoline(0), shutdownRequested(false)
        {
            worker_thread = std::thread([=]{ worker(); });
        }
        ~DefaultScheduler()
        {
            {
                std::unique_lock<std::mutex> guard(scheduleLock);
                shutdownRequested = true;
            }
            cv.notify_all();
        }


        void ScopeEnter() {++trampoline;}
        void ScopeExit() {--trampoline; Schedule([]{});}

        void Schedule(Work work)
        {
            try {
                if (++trampoline == 1)
                {                
                    work();

                    while(!queue.empty())
                    {
                        work = std::move(queue.back());
                        queue.pop_back();
                        work();
                    }
               
                }
                else
                {
                    queue.push_back(std::move(work));
                }
                --trampoline;
            }
            catch (...) {
                --trampoline;
                throw;
            }
        }

        struct compare_work
        {
            template <class T>
            bool operator()(const T& work1, const T& work2) const {
                return work1.first > work2.first;
            }
        };
        
        typedef std::chrono::steady_clock Clock;

        bool                        shutdownRequested;
        std::mutex                  scheduleLock;
        std::condition_variable     cv;
        std::priority_queue< std::pair<Clock::time_point, Work>,
                             std::vector<std::pair<Clock::time_point, Work>>,
                             compare_work > scheduledWork;


        void Schedule(int milliseconds, Work work)
        {
            Clock::time_point dueTime = Clock::now() + std::chrono::duration<int, std::milli>(milliseconds);

            bool wake = false;

            {
                std::unique_lock<std::mutex> guard(scheduleLock);
                wake = scheduledWork.empty() || dueTime < scheduledWork.top().first;

                scheduledWork.push(std::make_pair(dueTime, std::move(work)));
            }
            
            if (wake)
                cv.notify_one();
        }
    private:
        void worker()
        {
            std::unique_lock<std::mutex> guard(scheduleLock);

            while(!shutdownRequested)
            {
                if (scheduledWork.empty())
                {
                    cv.wait(guard);
                    continue;
                }
                
                auto now = Clock::now();
                auto dueTime = scheduledWork.top().first;
                if (dueTime > now)
                {
                    cv.wait_until(guard, dueTime);
                    continue;
                }

                // dispatch work
                auto work = std::move(scheduledWork.top().second);
                scheduledWork.pop();

                guard.unlock();
                try { 
                    Schedule([=]{work();}); 
                } catch (...) { 
                    // work must catch all expected exceptions 
                    // (yes, expected exceptions is an oxymoron)
                    std::unexpected();
                }
                guard.lock();
            }
        }
    };

    template <class F>
    struct fix0_thunk {
        F f;
        fix0_thunk(F&& f) : f(std::move(f))
        {
        }
        void operator()() const 
        {
            f(*this);
        }
    };
    template <class F>
    fix0_thunk<F> fix0(F f)
    {
        return fix0_thunk<F>(std::move(f));
    }

    template <class Integral>
    auto Range(
        Integral start, Integral end = std::numeric_limits<Integral>::max(), Integral step = 1
        )
    -> std::shared_ptr<Observable<Integral>>
    {
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

            DefaultScheduler::Instance().Schedule(
                fix0([=](std::function<void()> self) // TODO:
            {
                try {
                    if (state->cancel)
                        return;

                    if (!state->rem)
                    {
                        observer->OnCompleted();
                    }
                    else
                    {
                        observer->OnNext(state->i);
                        --state->rem; 
                        state->i += step;
                        DefaultScheduler::Instance().Schedule(std::move(self));
                    }
                } catch (...) {
                    observer->OnError(std::current_exception());
                }                
            }));

            return Disposable([=]{
                state->cancel = true;
            });
        });
    }

    template <class T>
    std::shared_ptr<Observable<T>> Delay(
        const std::shared_ptr<Observable<T>>& source,
        int milliseconds)
    {
        // TODO: for some reason, poor interactions take place if
        //   on_dispatcher() dispatches from UI thread. 
#if 0
        if (milliseconds == 0)
            return source;
#endif   

        
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
            -> Disposable
            {
                auto cancel = std::make_shared<bool>(false);

                ComposableDisposable cd;

                cd.Add(Disposable([=]{ *cancel = true; }));
                cd.Add(Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        // TODO: queue
                        DefaultScheduler::Instance().Schedule(
                            milliseconds, 
                            [=]{ 
                                if (!*cancel)
                                    observer->OnNext(element); 
                            });
                    },
                // on completed
                    [=]
                    {
                        DefaultScheduler::Instance().Schedule(
                            milliseconds, 
                            [=]{ 
                                if (!*cancel)
                                    observer->OnCompleted(); 
                            });
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
                    T last; bool hasValue;
                };
        
                auto state = std::make_shared<State>();
                state->hasValue = false;

                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        if (!state->hasValue || state->last != element)
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
#if !defined(OBSERVE_ON_DISPATCHER_OP)
    typedef StdQueueDispatcher ObserveOnDispatcherOp;
#endif 

    template <class T, class Dispatcher>
    std::shared_ptr<Observable<T>> ObserveOnDispatcher(
        const std::shared_ptr<Observable<T>>& source, 
        std::shared_ptr<Dispatcher> dispatcher = nullptr)
    {
        if (!dispatcher)
        {
            dispatcher = std::make_shared<Dispatcher>();
        }
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
