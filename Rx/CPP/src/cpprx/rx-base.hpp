// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_BASE_HPP)
#define CPPRX_RX_BASE_HPP

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
        Disposable();
        Disposable(const Disposable&);
        typedef std::function<void()> dispose_type;
        dispose_type dispose;
    public:
        explicit Disposable(dispose_type disposearg) 
            : dispose(std::move(disposearg)) {
            disposearg = nullptr;}
        Disposable(Disposable&& other) 
            : dispose(std::move(other.dispose)) {
            other.dispose = nullptr; }
        Disposable& operator=(Disposable other) {
            swap(other); 
            return *this;
        }
        void Dispose()
        {
            if (dispose) {
                dispose();
                dispose = nullptr;
            }
        }
        void swap(Disposable& rhs) {{using std::swap; swap(dispose, rhs.dispose);}}
        static Disposable Empty() { return Disposable(nullptr); }
    };
    inline void swap(Disposable& lhs, Disposable& rhs) {lhs.swap(rhs);}

    template <class T>
    struct Observable
    {
        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer) = 0;
        virtual ~Observable() {}
    };

    template <class K, class T>
    struct GroupedObservable : Observable<T>
    {
        virtual K Key() = 0;
        virtual ~GroupedObservable() {}
    };

    struct Scheduler : public std::enable_shared_from_this<Scheduler>
    {
        typedef std::chrono::steady_clock clock;
        typedef std::shared_ptr<Scheduler> shared;
        typedef std::function<Disposable(shared)> Work;

        shared get() {return shared_from_this();}

        virtual ~Scheduler() {}

        virtual clock::time_point Now() =0;
        virtual Disposable Schedule(Work work) = 0;
        virtual Disposable Schedule(clock::duration due, Work work) = 0;
        virtual Disposable Schedule(clock::time_point due, Work work) = 0;
    };

    //////////////////////////////////////////////////////////////////////
    // 
    // disposables

    
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
                    disposables.emplace_back(std::move(d));
                }
            }
            void Dispose()
            {
                std::unique_lock<decltype(lock)> guard(lock);

                if (!isDisposed)
                {
                    isDisposed = true;
                    auto v = std::move(disposables);
                    guard.unlock();
                    
                    std::for_each(v.begin(), v.end(),
                                  [](Disposable& d) { 
                                    d.Dispose(); });
                }
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
            // make sure to capture state and not 'this'.
            // usage means that 'this' will usualy be destructed
            // immediately
            auto local = state;
            return Disposable([local]{
                local->Dispose();
            });
        }
    };

    class ScheduledDisposable
    {
        struct State : public std::enable_shared_from_this<State>
        {
            Scheduler::shared scheduler;
            Disposable disposable;
            
            State(Scheduler::shared scheduler, Disposable disposable) 
                : scheduler(std::move(scheduler))
                , disposable(std::move(disposable))
            {
            }
            void Dispose()
            {
                auto local = std::move(scheduler);
                if (local) {
                    auto keepAlive = shared_from_this();
                    local->Schedule([keepAlive] (Scheduler::shared) {
                        keepAlive->disposable.Dispose();
                        return Disposable::Empty();
                    });
                }
            }
        };
        
        std::shared_ptr<State> state;
        
        ScheduledDisposable();
    public:

        ScheduledDisposable(Scheduler::shared scheduler, Disposable disposable) 
            : state(new State(std::move(scheduler), std::move(disposable)))
        {
        }
        void Dispose() const
        {
            state->Dispose();
        }
        operator Disposable() const
        {
            // make sure to capture state and not 'this'.
            // usage means that 'this' will usualy be destructed
            // immediately
            auto local = state;
            return Disposable([local]{
                local->Dispose();
            });
        }
    };

    class SharedDisposable
    {
        typedef std::function<void()> dispose_type;

        struct State : public std::enable_shared_from_this<State>
        {
            mutable Disposable disposable;
            mutable std::mutex lock;

            State() : disposable(Disposable::Empty()) {}

            void Set(Disposable disposeArg) const
            {
                std::unique_lock<decltype(lock)> guard(lock);
                {using std::swap; swap(disposable, disposeArg);}
            }
            void Dispose()
            {
                std::unique_lock<decltype(lock)> guard(lock);
                auto local = std::move(disposable);
                guard.unlock();
                local.Dispose();
            }
        };
        
        std::shared_ptr<State> state;
        
    public:

        SharedDisposable() 
            : state(new State())
        {
        }
        void Dispose() const
        {
            state->Dispose();
        }
        void Set(Disposable disposeArg) const
        {
            state->Set(std::move(disposeArg));
        }
        operator Disposable() const
        {
            // make sure to capture state and not 'this'.
            // usage means that 'this' will usualy be destructed
            // immediately
            auto local = state;
            return Disposable([local]{
                local->Dispose();
            });
        }
    };

    struct LocalScheduler : public Scheduler
    {
    private:
        LocalScheduler(const LocalScheduler&);

    public:
        static void Do(Work& work, Scheduler::shared scheduler) throw()
        {
            if (work)
            {
                work(std::move(scheduler));
            }
        }

    public:
        LocalScheduler()
        {
        }
        virtual ~LocalScheduler()
        {
        }

        virtual clock::time_point Now() {return clock::now();}

        using Scheduler::Schedule;
        virtual Disposable Schedule(Work work)
        {
            clock::time_point dueTime = clock::now();
            return Schedule(dueTime, std::move(work));
        }
        
        virtual Disposable Schedule(clock::duration due, Work work)
        {
            clock::time_point dueTime = clock::now() + due;
            return Schedule(dueTime, std::move(work));
        }
    };

    template<class Target>
    class TupleDispatch {
        Target target;
    public:
        TupleDispatch(Target target) : target(std::move(target)) {
        }
        template<class Tuple>
        auto operator()(const Tuple& tuple) 
            -> decltype(util::tuple_dispatch(target, tuple)) {
            return      util::tuple_dispatch(target, tuple);}
        template<class Tuple>
        auto operator()(const Tuple& tuple) const 
            -> decltype(util::tuple_dispatch(target, tuple))  {
        return          util::tuple_dispatch(target, tuple);}
    };

    template<class Target>
    TupleDispatch<Target> MakeTupleDispatch(Target&& target) {
    return TupleDispatch<Target>(std::forward<Target>(target));}

    template<class Tuple, class Target>
    auto DispatchTuple(Tuple&& tuple, Target&& target) -> 
        decltype(util::tuple_dispatch(std::forward<Target>(target), std::forward<Tuple>(tuple))) {
        return   util::tuple_dispatch(std::forward<Target>(target), std::forward<Tuple>(tuple));}

#if RXCPP_USE_VARIADIC_TEMPLATES
    template<class Lhs, class Rhs>
    auto ConcatTuple(Lhs&& lhs, Rhs&& rhs) -> 
        decltype(util::tuple_concat(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs))) {
        return   util::tuple_concat(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs));}

    template<class T>
    auto TieTuple(T&& t) -> 
        decltype(util::tuple_tie(std::forward<T>(t))) {
        return   util::tuple_tie(std::forward<T>(t));}
#endif //RXCPP_USE_VARIADIC_TEMPLATES

    template<class T>
    class Subject;

    template<class K, class T>
    class GroupedSubject;

    template<class T>
    T item(const std::shared_ptr<Observable<T>>&);

    template<class K, class T>
    T item(const std::shared_ptr<GroupedObservable<K,T>>&);

    template<class Observable>
    struct is_observable {static const bool value = false;};

    template<class T>
    struct is_observable<std::shared_ptr<Observable<T>>> {static const bool value = true;};

    template<class T>
    struct is_observable<std::shared_ptr<Subject<T>>> {static const bool value = true;};

    template<class K, class T>
    struct is_observable<std::shared_ptr<GroupedObservable<K, T>>> {static const bool value = true;};

    template<class K, class T>
    struct is_observable<std::shared_ptr<GroupedSubject<K, T>>> {static const bool value = true;};

    template<class Observable>
    struct observable_item;

    template<class T>
    struct observable_item<std::shared_ptr<Observable<T>>> {typedef T type;};

    template<class K, class T>
    struct observable_item<std::shared_ptr<GroupedObservable<K, T>>> {typedef T type;};

    template<class Observable>
    struct observable_observer;

    template<class T>
    struct observable_observer<std::shared_ptr<Observable<T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class K, class T>
    struct observable_observer<std::shared_ptr<GroupedObservable<K, T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class Observer>
    struct observer_item;

    template<class T>
    struct observer_item<std::shared_ptr<Observer<T>>> {typedef T type;};

    template<class Subject>
    struct subject_item;

    template<class T>
    struct subject_item<std::shared_ptr<Subject<T>>> {typedef T type;};

    template<class Subject>
    struct subject_observer;

    template<class T>
    struct subject_observer<std::shared_ptr<Subject<T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class Subject>
    struct subject_observable;

    template<class T>
    struct subject_observable<std::shared_ptr<Subject<T>>> {typedef std::shared_ptr<Observable<T>> type;};
}
#endif
