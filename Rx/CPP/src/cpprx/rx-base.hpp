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
        virtual void OnNext(const T&) {}
        virtual void OnCompleted() {}
        virtual void OnError(const std::exception_ptr&) {}

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

    template <class T>
    struct ConnectableObservable : Observable<T>
    {
        virtual Disposable Connect() = 0;
        virtual ~ConnectableObservable() {}
    };

    struct Scheduler : public std::enable_shared_from_this<Scheduler>
    {
        typedef std::chrono::steady_clock clock;
        typedef std::shared_ptr<Scheduler> shared;

        class Work {
            struct bool_r{}; 
            static bool_r bool_true(){return bool_r();}
            typedef decltype(&bool_true) bool_type;

            struct State
            {
                typedef std::function<Disposable(shared)> F;
                F work;
                bool disposed;
                inline void Dispose() {
                    disposed = true;
                }
            };
            std::shared_ptr<State> state;

            template<int, bool sameType>
            struct assign 
            {
                void operator()(const std::shared_ptr<State>& state, State::F fn) {
                    state->work = std::move(fn);
                }
            };

            template<int dummy>
            struct assign<dummy, true>
            {
                void operator()(std::shared_ptr<State>& state, const Work& o) {
                    state = o.state;
                }
                void operator()(std::shared_ptr<State>& state, Work&& o) {
                    state = std::move(o.state);
                }
            };
        public:
            Work()
                : state(std::make_shared<State>())
            {
                state->disposed = false;
            }
            template<class Fn>
            Work(Fn&& fn)
                : state(std::make_shared<State>())
            {
                assign<1, std::is_same<Work, typename std::decay<Fn>::type>::value>()(state, std::forward<Fn>(fn));
                state->disposed = false;
            }
            template<class Fn>
            Work& operator=(Fn&& fn)
            {
                assign<1, std::is_same<Work, typename std::decay<Fn>::type>::value>()(state, std::forward<Fn>(fn));
                return *this;
            }            
            inline Disposable operator()(shared s) {
                if (!state->disposed) {
                    return state->work(s);
                } 
                return Disposable::Empty();
            }
            inline operator bool_type() const {return (!state->disposed && !!state->work) ? &bool_true : nullptr;}
            inline void Dispose() const {
                state->Dispose();
            }
            inline operator Disposable() const
            {
                // make sure to capture state and not 'this'.
                auto local = state;
                return Disposable([local]{
                    local->Dispose();
                });
            }
        };

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
        typedef std::shared_ptr<Disposable> shared_disposable;
        typedef std::weak_ptr<Disposable> weak_disposable;
        struct State
        {
            std::vector<shared_disposable> disposables;
            std::mutex lock;
            bool isDisposed;
            
            State() : isDisposed(false)
            {
            }
            weak_disposable Add(Disposable&& d)
            {
                return Add(std::make_shared<Disposable>(std::move(d)));
            }
            weak_disposable Add(shared_disposable s)
            {
                std::unique_lock<decltype(lock)> guard(lock);
                if (isDisposed) {
                    guard.unlock();
                    s->Dispose();
                } else {
                    auto end = std::end(disposables);
                    auto it = std::find(std::begin(disposables), end, s);
                    if (it == end)
                    {
                        disposables.emplace_back(s);
                    }
                }
                return s;
            }
            void Remove(weak_disposable w) 
            {
                std::unique_lock<decltype(lock)> guard(lock);
                auto s = w.lock();
                if (s) 
                {
                    auto end = std::end(disposables);
                    auto it = std::find(std::begin(disposables), end, s);
                    if (it != end)
                    {
                        disposables.erase(it);
                    }
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
                                  [](shared_disposable& d) { 
                                    d->Dispose(); });
                }
            }
        };
        
        mutable std::shared_ptr<State> state;
        
    public:

        ComposableDisposable()
            : state(std::make_shared<State>())
        {
        }
        weak_disposable Add(shared_disposable s) const
        {
            return state->Add(std::move(s));
        }
        weak_disposable Add(Disposable d) const
        {
            return state->Add(std::move(d));
        }
        void Remove(weak_disposable w) const
        {
            state->Remove(w);
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
                    local->Schedule([keepAlive] (Scheduler::shared) -> Disposable {
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
        ScheduledDisposable(const ScheduledDisposable& o)
            : state(o.state)
        {
        }
        ScheduledDisposable(ScheduledDisposable&& o)
            : state(std::move(o.state))
        {
            o.state = nullptr;
        }
        ScheduledDisposable& operator=(ScheduledDisposable o) {
            using std::swap;
            swap(o.state, state);
            return *this;
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

    class SerialDisposable
    {
        struct State 
        {
            mutable Disposable disposable;
            mutable bool disposed;
            mutable std::mutex lock;

            State() : disposable(Disposable::Empty()), disposed(false) {}

            void Set(Disposable disposeArg) const
            { 
                std::unique_lock<decltype(lock)> guard(lock);
                if (!disposed) {
                    using std::swap; 
                    swap(disposable, disposeArg);
                }
                guard.unlock();
                disposeArg.Dispose();
            }
            void Dispose()
            {
                std::unique_lock<decltype(lock)> guard(lock);
                if (!disposed) {
                    disposed = true;
                    auto local = std::move(disposable);
                    guard.unlock();
                    local.Dispose();
                }
            }
        };
        
        mutable std::shared_ptr<State> state;
        
    public:

        SerialDisposable() 
            : state(std::make_shared<State>())
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

    template<class Absolute>
    class ScheduledItem {
    public:
        ScheduledItem(Absolute due, Scheduler::Work work)
            : due(due)
            , work(std::move(work))
        {}

        static Disposable Do(Scheduler::Work& work, Scheduler::shared scheduler) throw()
        {
            if (work)
            {
                return work(std::move(scheduler));
            }
            return Disposable::Empty();
        }

        Absolute due;
        Scheduler::Work work;
    };

    struct LocalScheduler : public Scheduler
    {
    private:
        LocalScheduler(const LocalScheduler&);

    public:
        typedef ScheduledItem<clock::time_point> QueueItem;
        static Disposable Do(Work& work, const Scheduler::shared& scheduler) throw()
        {
            return QueueItem::Do(work, scheduler);
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

    template<class Absolute, class Relative>
    struct VirtualTimeSchedulerBase : public Scheduler
    {
    private:
        VirtualTimeSchedulerBase(const VirtualTimeSchedulerBase&);

        bool is_enabled;

    public:
        virtual ~VirtualTimeSchedulerBase()
        {
        }

    protected:
        VirtualTimeSchedulerBase()
            : is_enabled(false)
            , clock_now(0)
        {
        }
        explicit VirtualTimeSchedulerBase(Absolute initialClock)
            : is_enabled(false)
            , clock_now(initialClock)
        {
        }

        typedef Scheduler::clock clock;
        typedef Scheduler::Work Work;
        typedef ScheduledItem<Absolute> QueueItem;

        Absolute clock_now;

        virtual Absolute Add(Absolute, Relative) =0;

        virtual clock::time_point ToTimePoint(Absolute) =0;
        virtual Relative ToRelative(clock::duration) =0;

        virtual util::maybe<QueueItem> GetNext() =0;

    public:
        static Disposable Do(Work& work, const Scheduler::shared& scheduler) throw()
        {
            return QueueItem::Do(work, scheduler);
        }

        virtual Disposable ScheduleAbsolute(Absolute, Work) =0;

        virtual Disposable ScheduleRelative(Relative dueTime, Work work) {
            auto at = Add(clock_now, dueTime);
            return ScheduleAbsolute(at, std::move(work));
        }

        virtual clock::time_point Now() {return ToTimePoint(clock_now);}

        bool IsEnabled() {return is_enabled;}
        Absolute Clock() {return clock_now;}

        using Scheduler::Schedule;
        virtual Disposable Schedule(Work work)
        {
            return ScheduleAbsolute(clock_now, std::move(work));
        }
        
        virtual Disposable Schedule(clock::duration due, Work work)
        {
            return ScheduleRelative(ToRelative(due), std::move(work));
        }

        virtual Disposable Schedule(clock::time_point due, Work work)
        {
            return ScheduleRelative(ToRelative(due - Now()), std::move(work));
        }

        void Start()
        {
            if (!is_enabled) {
                is_enabled = true;
                do {
                    auto next = GetNext();
                    if (!!next && !!next->work) {
                        if (next->due > clock_now) {
                            clock_now = next->due;
                        }
                        Do(next->work, shared_from_this());
                    }
                    else {
                        is_enabled = false;
                    }
                } while (is_enabled);
            }
        }

        void Stop()
        {
            is_enabled = false;
        }

        void AdvanceTo(Absolute time)
        {
            if (time < clock_now) {
                abort();
            }

            if (time == clock_now) {
                return;
            }

            if (!is_enabled) {
                is_enabled = true;
                do {
                    auto next = GetNext();
                    if (!!next && !!next->work && next->due <= time) {
                        if (next->due > clock_now) {
                            clock_now = next->due;
                        }
                        Do(next->work, shared_from_this());
                    }
                    else {
                        is_enabled = false;
                    }
                } while (is_enabled);

                clock_now = time;
            }
            else {
                abort();
            }
        }

        void AdvanceBy(Relative time)
        {
            auto dt = Add(clock_now, time);

            if (dt < clock_now) {
                abort();
            }

            if (dt == clock_now) {
                return;
            }

            if (!is_enabled) {
                AdvanceTo(dt);
            }
            else {
                abort();
            }
        }

        void Sleep(Relative time)
        {
            auto dt = Add(clock_now, time);

            if (dt < clock_now) {
                abort();
            }

            clock_now = dt;
        }

    };

    template<class T>
    class Recorded
    {
        long time;
        T value;
    public:
        Recorded(long time, T value) : time(time), value(value) {
        }
        long Time() const {return time;}
        const T& Value() const {return value;}
    };

    template<class T>
    bool operator == (Recorded<T> lhs, Recorded<T> rhs) {
        return lhs.Time() == rhs.Time() && lhs.Value() == rhs.Value();
    }

    template<class T>
    std::ostream& operator<< (std::ostream& out, const Recorded<T>& r) {
        out << "@" << r.Time() << "-" << r.Value();
        return out;
    }

    class Subscription
    {
        long subscribe;
        long unsubscribe;

    public:
        explicit inline Subscription(long subscribe) : subscribe(subscribe), unsubscribe(std::numeric_limits<long>::max()) {
        }
        inline Subscription(long subscribe, long unsubscribe) : subscribe(subscribe), unsubscribe(unsubscribe) {
        }
        inline long Subscribe() const {return subscribe;}
        inline long Unsubscribe() const {return unsubscribe;}
    };

    inline bool operator == (Subscription lhs, Subscription rhs) {
        return lhs.Subscribe() == rhs.Subscribe() && lhs.Unsubscribe() == rhs.Unsubscribe();
    }

    inline std::ostream& operator<< (std::ostream& out, const Subscription& s) {
        out << s.Subscribe() << "-" << s.Unsubscribe();
        return out;
    }

    template<typename T>
    struct Notification 
    {
        typedef std::function<void (const T&)> OnNext;
        typedef std::function<void ()> OnCompleted;
        typedef std::function<void (const std::exception_ptr&)> OnError;

        virtual ~Notification() {}

        virtual void Out(std::ostream& out) =0;
        virtual bool Equals(std::shared_ptr<Notification<T>> other) = 0;
        virtual void Accept(std::shared_ptr<Observer<T>>) =0;
        virtual void Accept(OnNext onnext, OnCompleted oncompleted, OnError onerror) =0;

    private:
        struct OnNextNotification : public Notification<T> {
            OnNextNotification(T value) : value(std::move(value)) {
            }
            virtual void Out(std::ostream& out) {
                out << "OnNext( " << value << ")";
            }
            virtual bool Equals(std::shared_ptr<Notification<T>> other) {
                bool result = false;
                other->Accept([this, &result](T value) { result = this->value == value;}, [](){}, [](std::exception_ptr){});
                return result;
            }
            virtual void Accept(std::shared_ptr<Observer<T>> o) {
                if (!o) {
                    abort();
                }
                o->OnNext(value);
            }
            virtual void Accept(OnNext onnext, OnCompleted oncompleted, OnError onerror) {
                if (!onnext || !oncompleted || !onerror) {
                    abort();
                }
                onnext(value);
            }
            const T value;
        };

        struct OnCompletedNotification : public Notification<T> {
            OnCompletedNotification() {
            }
            virtual void Out(std::ostream& out) {
                out << "OnCompleted()";
            }
            virtual bool Equals(std::shared_ptr<Notification<T>> other) {
                bool result = false;
                other->Accept([](T) {}, [&result](){result = true;}, [](std::exception_ptr){});
                return result;
            }
            virtual void Accept(std::shared_ptr<Observer<T>> o) {
                if (!o) {
                    abort();
                }
                o->OnCompleted();
            }
            virtual void Accept(OnNext onnext, OnCompleted oncompleted, OnError onerror) {
                if (!onnext || !oncompleted || !onerror) {
                    abort();
                }
                oncompleted();
            }
        };

        struct OnErrorNotification : public Notification<T> {
            OnErrorNotification(std::exception_ptr ep) : ep(ep) {
            }
            virtual void Out(std::ostream& out) {
                out << "OnError()";
            }
            virtual bool Equals(std::shared_ptr<Notification<T>> other) {
                bool result = false;
                // not trying to compare exceptions
                other->Accept([](T) {}, [](){}, [&result](std::exception_ptr){result = true;});
                return result;
            }
            virtual void Accept(std::shared_ptr<Observer<T>> o) {
                if (!o) {
                    abort();
                }
                o->OnError(ep);
            }
            virtual void Accept(OnNext onnext, OnCompleted oncompleted, OnError onerror) {
                if (!onnext || !oncompleted || !onerror) {
                    abort();
                }
                onerror(ep);
            }
            const std::exception_ptr ep;
        };

    public:
        static
        std::shared_ptr<OnNextNotification> CreateOnNext(T value) {
            return std::make_shared<OnNextNotification>(std::move(value));
        }
        static
        std::shared_ptr<OnCompletedNotification> CreateOnCompleted() {
            return std::make_shared<OnCompletedNotification>();
        }
        template<typename Exception>
        static
        std::shared_ptr<OnErrorNotification> CreateOnError(Exception&& e) {
            std::exception_ptr ep;
            try {throw std::forward<Exception>(e);}
            catch (...) {ep = std::current_exception();}
            return std::make_shared<OnErrorNotification>(ep);
        }
        static
        std::shared_ptr<OnErrorNotification> CreateOnError(std::exception_ptr ep) {
            return std::make_shared<OnErrorNotification>(ep);
        }
    };

    template<class T>
    bool operator == (std::shared_ptr<Notification<T>> lhs, std::shared_ptr<Notification<T>> rhs) {
        if (!lhs && !rhs) {return true;}
        if (!lhs || !rhs) {return false;}
        return lhs->Equals(rhs);
    }

    template<class T>
    std::ostream& operator<< (std::ostream& out, const std::shared_ptr<Notification<T>>& n) {
        n->Out(out);
        return out;
    }

    template<class T>
    struct TestableObserver : public Observer<T>
    {
        virtual std::vector<Recorded<std::shared_ptr<Notification<T>>>> Messages() =0;
    };

    template<class T>
    struct TestableObservable : public Observable<T>
    {
        virtual std::vector<Subscription> Subscriptions() =0;

        virtual std::vector<Recorded<std::shared_ptr<Notification<T>>>> Messages() =0;
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
    template<class T>
    auto TieTuple(T&& t) -> 
        decltype(util::tuple_tie(std::forward<T>(t))) {
        return   util::tuple_tie(std::forward<T>(t));}
#endif //RXCPP_USE_VARIADIC_TEMPLATES

    template<class T>
    class Subject;

    template<class T>
    class BehaviorSubject;

    template<class T>
    class AsyncSubject;

    template<class Source, class Subject>
    class ConnectableSubject;

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

    template<class T>
    struct is_observable<std::shared_ptr<BehaviorSubject<T>>> {static const bool value = true;};

    template<class T>
    struct is_observable < std::shared_ptr < AsyncSubject<T >> > {static const bool value = true; };

    template<class Source, class Subject>
    struct is_observable < std::shared_ptr < ConnectableSubject<Source, Subject >> > {static const bool value = true; };

    template<class K, class T>
    struct is_observable<std::shared_ptr<GroupedObservable<K, T>>> {static const bool value = true;};

    template<class K, class T>
    struct is_observable<std::shared_ptr<GroupedSubject<K, T>>> {static const bool value = true;};

namespace detail {
    template<class Observable>
    struct observable_item;

    template<class T>
    struct observable_item<std::shared_ptr<Observable<T>>> {typedef T type;};

    template<class T>
    struct observable_item<std::shared_ptr<TestableObservable<T>>> {typedef T type;};

    template<class T>
    struct observable_item < std::shared_ptr < ConnectableObservable<T >> > {typedef T type; };

    template<class K, class T>
    struct observable_item<std::shared_ptr<GroupedObservable<K, T>>> {typedef T type;};
}
    template<class Observable>
    struct observable_item
    {
        typedef typename detail::observable_item<typename std::decay<Observable>::type>::type type;
    };

    template<class Observable>
    struct observable_observer;

    template<class T>
    struct observable_observer<std::shared_ptr<Observable<T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class T>
    struct observable_observer < std::shared_ptr < ConnectableObservable<T >> > {typedef std::shared_ptr < Observer < T >> type; };

    template<class K, class T>
    struct observable_observer<std::shared_ptr<GroupedObservable<K, T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class Observer>
    struct observer_item;

    template<class T>
    struct observer_item<std::shared_ptr<Observer<T>>> {typedef T type;};

    template<class T>
    struct observer_item<std::shared_ptr<TestableObserver<T>>> {typedef T type;};

    template<class Subject>
    struct subject_item;

    template<class T>
    struct subject_item<std::shared_ptr<Subject<T>>> {typedef T type;};

    template<class T>
    struct subject_item<std::shared_ptr<BehaviorSubject<T>>> {typedef T type;};

    template<class T>
    struct subject_item < std::shared_ptr < AsyncSubject<T >> > {typedef T type; };

    template<class Source, class Subject>
    struct subject_item < std::shared_ptr < ConnectableSubject<Source, Subject >> > : subject_item<Subject> {};

    template<class K, class T>
    struct subject_item<std::shared_ptr<GroupedSubject<K, T>>> {typedef T type;};

    template<class Subject>
    struct subject_observer;

    template<class T>
    struct subject_observer<std::shared_ptr<Subject<T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class T>
    struct subject_observer<std::shared_ptr<BehaviorSubject<T>>> {typedef std::shared_ptr<Observer<T>> type;};

    template<class T>
    struct subject_observer < std::shared_ptr < AsyncSubject<T >> > {typedef std::shared_ptr < Observer < T >> type; };

    template<class Source, class Subject>
    struct subject_observer < std::shared_ptr < ConnectableSubject<Source, Subject >> > : subject_observer<Subject> {};

    template<class Subject>
    struct subject_observable;

    template<class T>
    struct subject_observable<std::shared_ptr<Subject<T>>> {typedef std::shared_ptr<Observable<T>> type;};

    template<class T>
    struct subject_observable<std::shared_ptr<BehaviorSubject<T>>> {typedef std::shared_ptr<Observable<T>> type;};

    template<class T>
    struct subject_observable < std::shared_ptr < AsyncSubject<T >> > {typedef std::shared_ptr < Observable < T >> type; };

    template<class Source, class Subject>
    struct subject_observable < std::shared_ptr < ConnectableSubject<Source, Subject >> > : subject_observable<Subject> {};

    template<class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr < Observable < T >> &o){ return o; }

    template<class K, class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr < GroupedObservable < K, T >> &o){ return std::static_pointer_cast < Observable < T >> (o); }

    template<class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr < TestableObservable < T >> &o){ return std::static_pointer_cast < Observable < T >> (o); }

    template<class T>
    std::shared_ptr< Observable < T >> observable(const std::shared_ptr < ConnectableObservable < T >> &o){
        return std::static_pointer_cast < Observable < T >> (o); }

    template<class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr<Subject<T>>& s){return std::static_pointer_cast<Observable<T>>(s);}

    template<class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr<BehaviorSubject<T>>& s){return std::static_pointer_cast<Observable<T>>(s);}

    template<class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr < AsyncSubject < T >> &s){ return std::static_pointer_cast < Observable < T >> (s); }

    template<class Source, class Subject>
    typename subject_observable<Subject>::type observable(const std::shared_ptr < ConnectableSubject < Source, Subject >> &s){ 
        return std::static_pointer_cast < Observable < typename subject_item<Subject>::type >> (s); }

    template<class K, class T>
    std::shared_ptr<Observable<T>> observable(const std::shared_ptr<GroupedSubject<K, T>>& s){return std::static_pointer_cast<Observable<T>>(s);}

    template<class T>
    std::shared_ptr<Observer<T>> observer(const std::shared_ptr < Observer < T >> &o){ return o; }

    template<class T>
    std::shared_ptr<Observer<T>> observer(const std::shared_ptr<Subject<T>>& s){return std::static_pointer_cast<Observer<T>>(s);}

    template<class T>
    std::shared_ptr<Observer<T>> observer(const std::shared_ptr<BehaviorSubject<T>>& s){return std::static_pointer_cast<Observer<T>>(s);}

    template<class T>
    std::shared_ptr<Observer<T>> observer(const std::shared_ptr < AsyncSubject < T >> &s){ return std::static_pointer_cast < Observer < T >> (s); }

    template<class Source, class Subject>
    void observer(const std::shared_ptr < ConnectableSubject < Source, Subject >> &s); // no observer

    template<class K, class T>
    std::shared_ptr<Observer<T>> observer(const std::shared_ptr<GroupedSubject<K, T>>& s){return std::static_pointer_cast<Observer<T>>(s);}

    template<class T>
    std::shared_ptr<Observer<T>> observer(const std::shared_ptr<TestableObserver<T>>& o){return std::static_pointer_cast<Observer<T>>(o);}

    template<class K, class T>
    std::shared_ptr<GroupedObservable<K, T>> grouped_observable(const std::shared_ptr<GroupedSubject<K, T>>& s){return std::static_pointer_cast<GroupedObservable<K, T>>(s);}
}
#endif
