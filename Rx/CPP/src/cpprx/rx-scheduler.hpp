// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_SCHEDULERS_HPP)
#define CPPRX_RX_SCHEDULERS_HPP

namespace rxcpp
{

    //////////////////////////////////////////////////////////////////////
    // 
    // schedulers

    struct CurrentThreadQueue
    {
        typedef Scheduler::clock clock;
        typedef Scheduler::Work Work;
        typedef ScheduledItem<clock::time_point> QueueItem;

    private:
        ~CurrentThreadQueue();
        
        struct compare_work
        {
            bool operator()(const QueueItem& work1, const QueueItem& work2) const {
                return work1.due > work2.due;
            }
        };
        
        typedef std::priority_queue<
            QueueItem,
            std::vector<QueueItem>,
            compare_work 
        > ScheduledWork;

    public:
        struct ThreadLocalQueue {
            Scheduler::shared scheduler;
            ScheduledWork queue;
        };

    private:
        static ThreadLocalQueue*& threadLocalQueue() {
            RXCPP_THREAD_LOCAL static ThreadLocalQueue* queue;
            return queue;
        }
        
    public:

        static Scheduler::shared GetScheduler() { 
            return !!threadLocalQueue() ? threadLocalQueue()->scheduler : Scheduler::shared(); 
        }
        static bool empty() {
            if (!threadLocalQueue()) {
                abort();
            }
            return threadLocalQueue()->queue.empty();
        }
        static ScheduledWork::const_reference top() {
            if (!threadLocalQueue()) {
                abort();
            }
            return threadLocalQueue()->queue.top();
        }
        static void pop() {
            if (!threadLocalQueue()) {
                abort();
            }
            threadLocalQueue()->queue.pop();
        }
        static void push(QueueItem item) {
            if (!threadLocalQueue()) {
                abort();
            }
            threadLocalQueue()->queue.push(std::move(item));
        }
        static void EnsureQueue(Scheduler::shared scheduler) {
            if (!!threadLocalQueue()) {
                abort();
            }
            // create and publish new queue
            threadLocalQueue() = new ThreadLocalQueue();
            threadLocalQueue()->scheduler = scheduler;
        }
        static std::unique_ptr<ThreadLocalQueue> CreateQueue(Scheduler::shared scheduler) {
            std::unique_ptr<ThreadLocalQueue> result(new ThreadLocalQueue());
            result->scheduler = std::move(scheduler);
            return result;
        }
        static void SetQueue(ThreadLocalQueue* queue) {
            if (!!threadLocalQueue()) {
                abort();
            }
            // publish new queue
            threadLocalQueue() = queue;
        }
        static void DestroyQueue(ThreadLocalQueue* queue) {
            delete queue;
        }
        static void DestroyQueue() {
            if (!threadLocalQueue()) {
                abort();
            }
            DestroyQueue(threadLocalQueue());
            threadLocalQueue() = nullptr;
        }
    };

    struct CurrentThreadScheduler : public LocalScheduler
    {
    private:
        CurrentThreadScheduler(const CurrentThreadScheduler&);

        struct Derecurser : public LocalScheduler
        {
        private:
            Derecurser(const Derecurser&);
        public:
            Derecurser()
            {
            }
            virtual ~Derecurser()
            {
            }

            static bool IsScheduleRequired() { return false; }
        
            using LocalScheduler::Schedule;
            virtual Disposable Schedule(clock::time_point dueTime, Work work)
            {
                CurrentThreadQueue::push(CurrentThreadQueue::QueueItem(dueTime, work));

                // work is disposable
                return work;
            }
        };

    public:
        CurrentThreadScheduler()
        {
        }
        virtual ~CurrentThreadScheduler()
        {
        }

        static bool IsScheduleRequired() { return !CurrentThreadQueue::GetScheduler(); }
        
        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto localScheduler = CurrentThreadQueue::GetScheduler();
            // check ownership
            if (!!localScheduler) 
            {
                // already has an owner - delegate
                return localScheduler->Schedule(dueTime, std::move(work));
            }

            // take ownership

            CurrentThreadQueue::EnsureQueue(std::make_shared<Derecurser>());
            RXCPP_UNWIND_AUTO([]{
                CurrentThreadQueue::DestroyQueue();
            });

            CurrentThreadQueue::push(CurrentThreadQueue::QueueItem(dueTime, std::move(work)));

            // loop until queue is empty
            for (
                 auto dueTime = CurrentThreadQueue::top().due;
                 std::this_thread::sleep_until(dueTime), true;
                 dueTime = CurrentThreadQueue::top().due
                 )
            {
                // dispatch work
                auto work = CurrentThreadQueue::top().work;

                CurrentThreadQueue::pop();
                
                Do(work, get());
                
                if (CurrentThreadQueue::empty()) {break;}
            }

            return Disposable::Empty();
        }
    };

    struct ImmediateScheduler : public LocalScheduler
    {
    private:
        ImmediateScheduler(const ImmediateScheduler&);
        
        mutable std::mutex lock;
        mutable bool hasFaulted;
        mutable bool isAquired;
        mutable CurrentThreadQueue::ThreadLocalQueue* queue;
    public:
        ImmediateScheduler()
            : hasFaulted(false)
            , isAquired(false)
            , queue(nullptr)
        {
        }
        virtual ~ImmediateScheduler()
        {
        }

        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto item = CurrentThreadQueue::QueueItem(dueTime, std::move(work));
            bool isOwner = false;

            {
                std::unique_lock<std::mutex> guard(lock);
                if (!hasFaulted && !queue && !isAquired)
                {
                    queue = CurrentThreadQueue::CreateQueue(this->shared_from_this()).release();
                    isOwner = !isAquired;
                    isAquired = true;
                }
            }

            if (isOwner) 
            {
                RXCPP_UNWIND_AUTO([&](){
                    std::unique_lock<std::mutex> guard(lock);
                    CurrentThreadQueue::DestroyQueue(queue);
                    queue = nullptr;
                    isAquired = false;
                });

                for(;;) 
                {
                    std::this_thread::sleep_until(item.due);
                    try 
                    {
                        Do(item.work, queue->scheduler);
                    }
                    catch (...)
                    {
                        std::unique_lock<std::mutex> guard(lock);
                        while (!queue->queue.empty()) {queue->queue.pop();}
                        hasFaulted = true;
                        throw;
                    }

                    {
                        std::unique_lock<std::mutex> guard(lock);
                        if (queue->queue.empty())
                        {
                            break;
                        }
                        item = std::move(queue->queue.top());
                        queue->queue.pop();
                    }
                }
                return Disposable::Empty();
            }
            else
            {
                std::unique_lock<std::mutex> guard(lock);
                queue->queue.push(item);
                return item.work;
            }
        }
    };
    
    template<class T>
    class ScheduledObserver : 
        public Observer<T>, 
        public std::enable_shared_from_this<ScheduledObserver<T>>
    {
        typedef std::function<void()> Action;

        Scheduler::shared scheduler;
        mutable std::shared_ptr<Observer<T>> observer;
        mutable SerialDisposable sd;
        mutable std::queue<Action> queue;
        mutable std::mutex lock;
        mutable bool hasFaulted;
        mutable bool isAquired;

    public:
        ScheduledObserver(Scheduler::shared scheduler, std::shared_ptr<Observer<T>> observer)
            : scheduler(std::move(scheduler))
            , observer(std::move(observer))
            , hasFaulted(false)
            , isAquired(false)
        {
        }

        void Dispose() const
        {
            sd.Dispose();
        }
        operator Disposable() const
        {
            auto local = this->shared_from_this();
            return Disposable([local]{
                local->Dispose();
            });
        }

        virtual void OnNext(const T& element)
        {
            std::unique_lock<std::mutex> guard(lock);
            queue.push(Action([=](){
                    this->observer->OnNext(std::move(element));
                }));
        }
        virtual void OnCompleted() 
        {
            std::unique_lock<std::mutex> guard(lock);
            queue.push(Action([=](){
                    this->observer->OnCompleted();
                }));
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            std::unique_lock<std::mutex> guard(lock);
            queue.push(Action([=](){
                    this->observer->OnError(std::move(error));
                }));
        }

        void EnsureActive()
        {
            bool isOwner = false;

            {
                std::unique_lock<std::mutex> guard(lock);
                if (!hasFaulted && !queue.empty())
                {
                    isOwner = !isAquired;
                    isAquired = true;
                }
            }

            if (isOwner)
            {
                auto keepAlive = this->shared_from_this();
                sd.Set(scheduler->Schedule(
                    [keepAlive](Scheduler::shared sched){
                        return keepAlive->Run(sched);}));
            }
        }

    private:
        Disposable Run(Scheduler::shared sched)
        {
            auto keepAlive = this->shared_from_this();

            Action action;
            {
                std::unique_lock<std::mutex> guard(lock);
                if(!queue.empty())
                {
                    action = std::move(queue.front());
                    queue.pop();
                }
                else
                {
                    isAquired = false;
                    return Disposable::Empty();
                }
            }

            try 
            {
                action();
            }
            catch (...)
            {
                std::unique_lock<std::mutex> guard(lock);
                while (!queue.empty()) {queue.pop();}
                hasFaulted = true;
                throw;
            }

            sd.Set(sched->Schedule(
                [keepAlive](Scheduler::shared sched){
                    return keepAlive->Run(sched);}));
            return sd;
        }
    };

    struct EventLoopScheduler : public LocalScheduler
    {
    private:
        EventLoopScheduler(const EventLoopScheduler&);

        struct Derecurser : public std::enable_shared_from_this<Derecurser>
        {
        private:
            Derecurser(const Derecurser&);

            mutable bool isAquired;
            mutable std::mutex lock;
            mutable std::condition_variable wake;
            CurrentThreadQueue::ThreadLocalQueue* queue;

        public:
            Derecurser()
                : isAquired(false)
            {
            }
            virtual ~Derecurser()
            {
            }

            typedef std::function<void()> RunLoop;
            typedef std::function<std::thread(RunLoop)> Factory;

            static bool IsScheduleRequired() { return false; }
        
            typedef std::tuple<util::maybe<std::thread>, Disposable> EnsureThreadResult;
            EnsureThreadResult EnsureThread(Factory& factory, Scheduler::shared owner, clock::time_point dueTime, Work work)
            {
                bool isOwner = false;
                EnsureThreadResult result(util::maybe<std::thread>(), Disposable::Empty());

                // work is disposable
                std::get<1>(result) = work;

                std::unique_lock<std::mutex> guard(lock);

                if (!isAquired)
                {
                    RXCPP_UNWIND(unwindQueue, [&](){
                        CurrentThreadQueue::DestroyQueue(queue); queue = nullptr;});
                    queue = CurrentThreadQueue::CreateQueue(owner).release();

                    queue->queue.push(CurrentThreadQueue::QueueItem(dueTime, std::move(work)));

                    auto local = std::static_pointer_cast<Derecurser>(shared_from_this());
                    auto localQueue = queue;
                    std::get<0>(result).set(factory([local, localQueue]{
                               local->Run(localQueue);}));

                    isOwner = !isAquired;
                    isAquired = true;

                    // queue lifetime is now owned by the thread
                    unwindQueue.dismiss();
                }

                if (!isOwner)
                {
                    queue->queue.push(CurrentThreadQueue::QueueItem(dueTime, std::move(work)));
                    wake.notify_one();
                }

                return std::move(result);
            }

        private:
            void Run(CurrentThreadQueue::ThreadLocalQueue* queue) {
                auto keepAlive = shared_from_this();
                {
                    std::unique_lock<std::mutex> guard(lock);

                    RXCPP_UNWIND_AUTO([&]{
                        isAquired = false;});

                    RXCPP_UNWIND(unwindQueue, [&](){
                        CurrentThreadQueue::DestroyQueue(); 
                        queue = nullptr;});

                    CurrentThreadQueue::SetQueue(queue);

#if 0
                    auto start = queue->scheduler->Now();
                    auto ms = std::chrono::milliseconds(1);
#endif
                    while(!CurrentThreadQueue::empty())
                    {
                        auto now = queue->scheduler->Now();
#if 0
                        {std::wstringstream out;
                        out << L"eventloop (run) pending: " << std::boolalpha << CurrentThreadQueue::empty() 
                            << L", now: " << ((now - start) / ms);
                            if (!CurrentThreadQueue::empty()) {
                                out << L", due: " << ((CurrentThreadQueue::top().due - start) / ms);}
                        out << std::endl;
                        OutputDebugString(out.str().c_str());}
#endif
                        if (CurrentThreadQueue::empty()) {
#if 0
                            {std::wstringstream out;
                            out << L"eventloop (wait for work) pending: " << std::boolalpha << CurrentThreadQueue::empty() 
                                << L", now: " << ((now - start) / ms);
                            out << std::endl;
                            OutputDebugString(out.str().c_str());}
#endif
                            wake.wait(guard, [&](){
                                return !CurrentThreadQueue::empty();});
                            continue;
                        }

                        auto item = &CurrentThreadQueue::top();
                        if (!item->work) {
#if 0
                            {std::wstringstream out;
                            out << L"eventloop (pop disposed work) pending: " << std::boolalpha << CurrentThreadQueue::empty() 
                                << L", now: " << ((now - start) / ms);
                                if (!CurrentThreadQueue::empty()) {
                                    out << L", due: " << ((CurrentThreadQueue::top().due - start) / ms);}
                            out << std::endl;
                            OutputDebugString(out.str().c_str());}
#endif
                            CurrentThreadQueue::pop(); continue;}
                        
                        // wait until the work is due
                        if (now < item->due)
                        {
#if 0
                            {std::wstringstream out;
                            out << L"eventloop (wait for due) pending: " << std::boolalpha << CurrentThreadQueue::empty() 
                                << L", now: " << ((now - start) / ms);
                                if (!CurrentThreadQueue::empty()) {
                                    out << L", due: " << ((CurrentThreadQueue::top().due - start) / ms);}
                            out << std::endl;
                            OutputDebugString(out.str().c_str());}
#endif
                            wake.wait_until(guard, item->due);
                            continue;
                        }
#if 0
                        {std::wstringstream out;
                        out << L"eventloop (dispatch) pending: " << std::boolalpha << CurrentThreadQueue::empty() 
                            << L", now: " << ((now - start) / ms);
                            if (!CurrentThreadQueue::empty()) {
                                out << L", due: " << ((CurrentThreadQueue::top().due - start) / ms);}
                        out << std::endl;
                        OutputDebugString(out.str().c_str());}
#endif                        
                        // dispatch work
                        auto work = item->work;

                        CurrentThreadQueue::pop();
                        
                        RXCPP_UNWIND_AUTO([&]{
                            guard.lock();});
                        guard.unlock();
                        LocalScheduler::Do(work, queue->scheduler);
                    }
                }
            }
        };

        std::thread worker;
        Derecurser::Factory factory;
        std::shared_ptr<Derecurser> derecurser;

    public:
        EventLoopScheduler()
            : derecurser(std::make_shared<Derecurser>())
        {
            auto local = derecurser;
            factory = [local] (Derecurser::RunLoop rl) -> std::thread {
                return std::thread([local, rl]{rl();});
            };
        }
        template<class Factory>
        EventLoopScheduler(Factory factoryarg) 
            : factory(std::move(factoryarg))
            , derecurser(std::make_shared<Derecurser>())
        {
        }
        virtual ~EventLoopScheduler()
        {
            if (worker.joinable()) {
                worker.detach();
            }
        }

        static bool IsScheduleRequired() { return !CurrentThreadQueue::GetScheduler(); }
        
        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto maybeThread = derecurser->EnsureThread(factory, this->shared_from_this(), dueTime, std::move(work));
            if (std::get<0>(maybeThread))
            {
                if (worker.joinable())
                {
                    worker.join();
                }
                worker = std::move(*std::get<0>(maybeThread).get());
            }
            return std::move(std::get<1>(maybeThread));
        }
    };
    
    struct NewThreadScheduler : public LocalScheduler
    {
    public:
        typedef std::function<std::thread(std::function<void()>)> Factory;
    private:
        NewThreadScheduler(const NewThreadScheduler&);
        
        Factory factory;
    public:
        
        
        NewThreadScheduler() : factory([](std::function<void()> start){return std::thread(std::move(start));})
        {
        }
        NewThreadScheduler(Factory factory) : factory(factory)
        {
        }
        virtual ~NewThreadScheduler()
        {
        }
        
        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto scheduler = std::make_shared<EventLoopScheduler>(factory);
            return scheduler->Schedule(dueTime, work);
        }
    };

    template<class Absolute, class Relative>
    class VirtualTimeScheduler : public VirtualTimeSchedulerBase<Absolute, Relative>
    {
    private:
        VirtualTimeScheduler(const VirtualTimeScheduler&);

        typedef VirtualTimeSchedulerBase<Absolute, Relative> Base;

        typedef typename Base::QueueItem QueueItem;

        struct compare_work
        {
            bool operator()(const QueueItem& work1, const QueueItem& work2) const {
                return work1.due > work2.due;
            }
        };
        
        typedef std::priority_queue<
            QueueItem,
            std::vector<QueueItem>,
            compare_work 
        > ScheduledWork;

        ScheduledWork queue;

    public:
        typedef typename Base::clock clock;
        typedef typename Base::Work Work;

        virtual ~VirtualTimeScheduler()
        {
        }

    protected:
        VirtualTimeScheduler()
        {
        }
        explicit VirtualTimeScheduler(Absolute initialClock)
            : Base(initialClock)
        {
        }

        virtual util::maybe<QueueItem> GetNext() {
            util::maybe<QueueItem> next;
            while (!queue.empty()) {
                next.set(queue.top());
                if (!next->work) {
                    queue.pop();
                }
                else {
                    return next;
                }
            }
            return next;
        }

        Disposable ScheduleAbsolute(Absolute dueTime, Work work)
        {
            Work cancelable;

            auto run = [cancelable, work](Scheduler::shared scheduler) -> Disposable {
                auto local = work;
                cancelable.Dispose();
                return Base::Do(local, std::move(scheduler));
            };
            
            cancelable = run;

            auto si = QueueItem(dueTime, cancelable);
            queue.push(si);

            return cancelable;
        }

    };

    class TestScheduler : public VirtualTimeScheduler<long, long>
    {
    public:
        typedef VirtualTimeScheduler<long, long> Base;
        typedef Base::clock clock;
        typedef Base::Work Work;
        typedef std::shared_ptr<TestScheduler> shared;

        static const long Created = 100;
        static const long Subscribed = 200;
        static const long Disposed = 1000;

        template<class T>
        struct Messages 
        {
            typedef Recorded<std::shared_ptr<Notification<T>>> RecordedT;

            static 
            RecordedT OnNext(long ticks, T value)
            {
                return RecordedT(ticks, Notification<T>::CreateOnNext(value));
            }

            static 
            RecordedT OnCompleted(long ticks)
            {
                return RecordedT(ticks, Notification<T>::CreateOnCompleted());
            }

            static 
            RecordedT OnError(long ticks, std::exception_ptr ep)
            {
                return RecordedT(ticks, Notification<T>::CreateOnError(ep));
            }

            template<class Exception>
            static 
            RecordedT OnError(long ticks, Exception e)
            {
                return RecordedT(ticks, Notification<T>::CreateOnError(e));
            }

            static 
            Subscription Subscribe(long subscribe, long unsubscribe)
            {
                return Subscription(subscribe, unsubscribe);
            }

            template<class Item, size_t size>
            static 
            auto ToVector(const Item (&arr) [size]) -> std::vector<Item> {
                return std::vector<Item>(std::begin(arr), std::end(arr));
            }

        private:
            ~Messages();
        };

        virtual Disposable ScheduleAbsolute(long dueTime, Work work)
        {
            if (dueTime <= Base::clock_now)
                dueTime = Base::clock_now + 1;

            return Base::ScheduleAbsolute(dueTime, std::move(work));
        }

        virtual long Add(long absolute, long relative)
        {
            return absolute + relative;
        }

        virtual clock::time_point ToTimePoint(long absolute)
        {
            return clock::time_point(clock::duration(absolute));
        }

        virtual long ToRelative(clock::duration d)
        {
            return static_cast<long>(d.count());
        }

        using Base::Start;

        template<class T>
        std::shared_ptr<TestableObserver<T>> Start(std::function<std::shared_ptr<Observable<T>>()> create, long created, long subscribed, long disposed)
        {
            auto observer = CreateObserver<T>();

            struct State 
            {
                std::shared_ptr<Observable<T>> source;
                SerialDisposable subscription;
                std::shared_ptr<TestableObserver<T>> observer;
            };
            auto state = std::make_shared<State>();

            state->observer = observer;

            ScheduleAbsolute(created, [create, state](Scheduler::shared scheduler) -> Disposable { 
                state->source = create(); return Disposable::Empty(); });
            ScheduleAbsolute(subscribed, [state](Scheduler::shared scheduler) -> Disposable { 
                state->subscription.Set(state->source->Subscribe(state->observer)); return Disposable::Empty(); });
            ScheduleAbsolute(disposed, [state](Scheduler::shared scheduler) -> Disposable { 
                state->subscription.Dispose(); return Disposable::Empty(); });

            Start();

            return observer;
        }

        template<class T>
        std::shared_ptr<TestableObserver<T>> Start(std::function<std::shared_ptr<Observable<T>>()> create, long disposed)
        {
            return Start(create, Created, Subscribed, disposed);
        }

        template<class T>
        std::shared_ptr<TestableObserver<T>> Start(std::function<std::shared_ptr<Observable<T>>()> create)
        {
            return Start(create, Created, Subscribed, Disposed);
        }

        template<class T>
        std::shared_ptr<TestableObservable<T>> CreateHotObservable(std::vector<Recorded<std::shared_ptr<Notification<T>>>> messages);

        template<class T>
        std::shared_ptr<TestableObservable<T>> CreateColdObservable(std::vector<Recorded<std::shared_ptr<Notification<T>>>> messages);

        template<class T>
        std::shared_ptr<TestableObserver<T>> CreateObserver();
    };

    template<class T>
    class MockObserver : public TestableObserver<T>
    {
        typedef Notification<T> NotificationT;
        typedef Recorded<std::shared_ptr<Notification<T>>> RecordedT;

        TestScheduler::shared scheduler;
        std::vector<RecordedT> messages;

    public:
        MockObserver(TestScheduler::shared scheduler)
            : scheduler(scheduler)
        {
        }

        virtual void OnNext(const T& value)
        {
            messages.push_back(RecordedT(scheduler->Clock(), NotificationT::CreateOnNext(value)));
        }

        virtual void OnError(const std::exception_ptr& exception)
        {
            messages.push_back(RecordedT(scheduler->Clock(), NotificationT::CreateOnError(exception)));
        }

        virtual void OnCompleted()
        {
            messages.push_back(RecordedT(scheduler->Clock(), NotificationT::CreateOnCompleted()));
        }

        std::vector<RecordedT> Messages()
        {
            return messages;
        }
    };

    template<class T>
    std::shared_ptr<TestableObserver<T>> TestScheduler::CreateObserver()
    {
        return std::make_shared<MockObserver<T>>(std::static_pointer_cast<TestScheduler>(shared_from_this()));
    }

    template<class T>
    class ColdObservable : public TestableObservable<T>, public std::enable_shared_from_this<ColdObservable<T>>
    {
        TestScheduler::shared scheduler;
        typedef Recorded<std::shared_ptr<Notification<T>>> RecordedT;
        std::vector<RecordedT> messages;
        std::vector<Subscription> subscriptions;

    public:

        ColdObservable(TestScheduler::shared scheduler, std::vector<RecordedT> messages)
            : scheduler(scheduler)
            , messages(std::move(messages))
        {
        }

        template<class Iterator>
        ColdObservable(TestScheduler::shared scheduler, Iterator begin, Iterator end)
            : scheduler(scheduler)
            , messages(begin, end)
        {
        }

        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            subscriptions.push_back(Subscription(scheduler->Clock()));
            auto index = subscriptions.size() - 1;

            ComposableDisposable d;

            for (auto& message : messages) {
                auto notification = message.Value();
                d.Add(scheduler->ScheduleRelative(message.Time(), [notification, observer](Scheduler::shared) -> Disposable { 
                    notification->Accept(observer); return Disposable::Empty(); 
                }));
            }

            auto sharedThis = this->shared_from_this();
            return Disposable([sharedThis, index, d]() {
                sharedThis->subscriptions[index] = Subscription(sharedThis->subscriptions[index].Subscribe(), sharedThis->scheduler->Clock());
                d.Dispose();
            });
        }

        virtual std::vector<Subscription> Subscriptions() {
            return subscriptions;
        }

        virtual std::vector<Recorded<std::shared_ptr<Notification<T>>>> Messages() {
            return messages;
        }
    };

    template<class T>
    std::shared_ptr<TestableObservable<T>> TestScheduler::CreateColdObservable(std::vector<Recorded<std::shared_ptr<Notification<T>>>> messages)
    {
        return std::make_shared<ColdObservable<T>>(std::static_pointer_cast<TestScheduler>(shared_from_this()), std::move(messages));
    }

    template<class T>
    class HotObservable : public TestableObservable<T>, public std::enable_shared_from_this<HotObservable<T>>
    {
        TestScheduler::shared scheduler;
        typedef Recorded<std::shared_ptr<Notification<T>>> RecordedT;
        std::vector<RecordedT> messages;
        std::vector<Subscription> subscriptions;
        std::vector<std::shared_ptr<Observer<T>>> observers;

    public:

        HotObservable(TestScheduler::shared scheduler, std::vector<RecordedT> messages)
            : scheduler(scheduler)
            , messages(messages)
        {
            for (auto& message : messages) {
                auto notification = message.Value();
                scheduler->ScheduleAbsolute(message.Time(), [this, notification](Scheduler::shared) -> Disposable {
                    auto local = this->observers;
                    for (auto& observer : local) {
                        notification->Accept(observer);
                    }
                    return Disposable::Empty();
                });
            }
        }

        virtual Disposable Subscribe(std::shared_ptr<Observer<T>> observer)
        {
            observers.push_back(observer);
            subscriptions.push_back(Subscription(scheduler->Clock()));
            auto index = subscriptions.size() - 1;

            auto sharedThis = this->shared_from_this();
            return Disposable([sharedThis, index, observer]() {
                sharedThis->observers.erase(std::find(sharedThis->observers.begin(), sharedThis->observers.end(), observer));
                sharedThis->subscriptions[index] = Subscription(sharedThis->subscriptions[index].Subscribe(), sharedThis->scheduler->Clock());
            });
        }

        virtual std::vector<Subscription> Subscriptions() {
            return subscriptions;
        }

        virtual std::vector<Recorded<std::shared_ptr<Notification<T>>>> Messages() {
            return messages;
        }
    };

    template<class T>
    std::shared_ptr<TestableObservable<T>> TestScheduler::CreateHotObservable(std::vector<Recorded<std::shared_ptr<Notification<T>>>> messages)
    {
        return std::make_shared<HotObservable<T>>(std::static_pointer_cast<TestScheduler>(shared_from_this()), std::move(messages));
    }

}

#endif
