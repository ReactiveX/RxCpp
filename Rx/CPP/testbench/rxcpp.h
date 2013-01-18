// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <deque>
#include <thread>
#include <vector>
#include <queue>
#include <chrono>
#include <condition_variable>

#include <Windows.h>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

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
        typename std::identity<std::function<void(const T&)>>::type onNext,
        std::function<void()> onCompleted = nullptr,
        std::function<void(const std::exception_ptr&)> onError = nullptr
        )
    {
        auto observer = CreateObserver<T>(std::move(onNext), std::move(onCompleted), std::move(onError));
        
        return source->Subscribe(observer);
    }

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
        Integral start, Integral end = (Integral)-1, Integral step = 1
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
            state->rem = (end - start) / step;

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
                        ++state->i;
                        DefaultScheduler::Instance().Schedule(std::move(self));
                    }
                } catch (...) {
                    observer->OnError(std::current_exception());
                }                
            }));

            return [=]{ state->cancel = true; };
        });
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
            std::unique_lock<std::mutex> guard(scheduleLock);
            shutdownRequested = true;
            guard.unlock();
            cv.notify_one();
        }

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
            }
            catch (...)
            {
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

            std::unique_lock<std::mutex> guard(scheduleLock);
            bool wake = scheduledWork.empty() || dueTime < scheduledWork.top().first;

            scheduledWork.push(std::make_pair(dueTime, std::move(work)));

            guard.unlock();
            
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
                    work(); 
                } catch (...) { 
                    // TODO: ??? what now?
                }
                guard.lock();
            }
        }
    };

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
            {
                struct State {
                    ULONGLONG dueTime;
                };
        
                auto state = std::make_shared<State>();
                state->dueTime = 0;

                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        auto now = ::GetTickCount64();

                        if (now >= state->dueTime)
                        {
                            observer->OnNext(element);
                            state->dueTime = now + (ULONGLONG)milliseconds;
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

    


    struct ObserveOnDispatcherOp
    {
        HWND hwnd;

        ObserveOnDispatcherOp(): hwnd(WindowClass::Instance().CreateWindow_())
        {
            if (!hwnd)
                throw std::exception("error");
        }
        ~ObserveOnDispatcherOp()
        {
            // send one last message to ourselves to shutdown.
            post([=]{ CloseWindow(hwnd); });
        }

        struct WindowClass
        {
            static const wchar_t* const className(){ return L"ObserveOnDispatcherOp::WindowClass"; }
            WindowClass()
            {
                WNDCLASS wndclass = {};
                wndclass.style = 0;
                wndclass.lpfnWndProc = &WndProc;
                wndclass.cbClsExtra;
                wndclass.cbWndExtra = 0;
                wndclass.hInstance = NULL;
                wndclass.lpszClassName = className();

                if (!RegisterClass(&wndclass))
                    throw std::exception("error");
                
            }
            HWND CreateWindow_()
            {
                return CreateWindowEx(0, WindowClass::className(), L"MessageOnlyWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
            }
            static const int WM_USER_DISPATCH = WM_USER + 1;

            static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
            {
                switch (message)
                {
                    // TODO: shatter attack surface. should validate the message, e.g. using a handle table.
                case WM_USER_DISPATCH:
                    ((void(*)(void*))wParam)((void*)lParam);
                    return 0;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
                }
            }
            static WindowClass& Instance() { 
                static WindowClass instance;
                return instance;
            }
        };

        template <class Fn>
        void post(Fn fn) const
        {
            auto p = new Fn(fn);
            ::PostMessage(hwnd, WindowClass::WM_USER_DISPATCH, (WPARAM)(void(*)(void*))&run_proc<Fn>, (LPARAM)(void*)p);
        }
        template <class Fn>
        static void run_proc(
            void* pvfn
        )
        {
            auto fn = (Fn*)(void*) pvfn;
            (*fn)();
            delete fn;
        }
    };

    template <class T>
    std::shared_ptr<Observable<T>> ObserveOnDispatcher(
        const std::shared_ptr<Observable<T>>& source)
    {
        auto dispatcher = std::make_shared<ObserveOnDispatcherOp>();
        return CreateObservable<T>(
            [=](std::shared_ptr<Observer<T>> observer)
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

#include "rxcpp-binder.h"

#pragma pop_macro("min")
#pragma pop_macro("max")
