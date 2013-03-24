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
        struct QueueItem {
            QueueItem(clock::time_point due, std::shared_ptr<Work> work)
                : due(due)
                , work(std::move(work))
            {}
            clock::time_point due;
            std::shared_ptr<Work> work;
        };

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
        RXCPP_THREAD_LOCAL static ThreadLocalQueue* threadLocalQueue;
        
    public:

        static Scheduler::shared GetScheduler() { 
            return !!threadLocalQueue ? threadLocalQueue->scheduler : Scheduler::shared(); 
        }
        static bool empty() {
            if (!threadLocalQueue) {
                throw std::logic_error("this thread does not have a queue!");
            }
            return threadLocalQueue->queue.empty();
        }
        static ScheduledWork::const_reference top() {
            if (!threadLocalQueue) {
                throw std::logic_error("this thread does not have a queue!");
            }
            return threadLocalQueue->queue.top();
        }
        static void pop() {
            if (!threadLocalQueue) {
                throw std::logic_error("this thread does not have a queue!");
            }
            threadLocalQueue->queue.pop();
        }
        static void push(QueueItem item) {
            if (!threadLocalQueue) {
                throw std::logic_error("this thread does not have a queue!");
            }
            threadLocalQueue->queue.push(std::move(item));
        }
        static void EnsureQueue(Scheduler::shared scheduler) {
            if (!!threadLocalQueue) {
                throw std::logic_error("this thread already has a queue!");
            }
            // create and publish new queue
            threadLocalQueue = new ThreadLocalQueue();
            threadLocalQueue->scheduler = scheduler;
        }
        static std::unique_ptr<ThreadLocalQueue> CreateQueue(Scheduler::shared scheduler) {
            std::unique_ptr<ThreadLocalQueue> result(new ThreadLocalQueue());
            result->scheduler = std::move(scheduler);
            return result;
        }
        static void SetQueue(ThreadLocalQueue* queue) {
            if (!!threadLocalQueue) {
                throw std::logic_error("this thread already has a queue!");
            }
            // create and publish new queue
            threadLocalQueue = queue;
        }
        static void DestroyQueue(ThreadLocalQueue* queue) {
            delete queue;
        }
        static void DestroyQueue() {
            if (!threadLocalQueue) {
                throw std::logic_error("this thread does not have a queue!");
            }
            DestroyQueue(threadLocalQueue);
            threadLocalQueue = nullptr;
        }
    };
    // static 
    RXCPP_SELECT_ANY RXCPP_THREAD_LOCAL CurrentThreadQueue::ThreadLocalQueue* CurrentThreadQueue::threadLocalQueue = nullptr;

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
                auto cancelable = std::make_shared<Work>(std::move(work));

                CurrentThreadQueue::push(CurrentThreadQueue::QueueItem(dueTime, cancelable));

                return Disposable([cancelable]{
                    *cancelable.get() = nullptr;});
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

            auto cancelable = std::make_shared<Work>(std::move(work));

            CurrentThreadQueue::EnsureQueue(std::make_shared<Derecurser>());
            RXCPP_UNWIND_AUTO([]{
                CurrentThreadQueue::DestroyQueue();
            });

            CurrentThreadQueue::push(CurrentThreadQueue::QueueItem(dueTime, std::move(cancelable)));

            // loop until queue is empty
            for (
                 auto dueTime = CurrentThreadQueue::top().due;
                 std::this_thread::sleep_until(dueTime), true;
                 dueTime = CurrentThreadQueue::top().due
                 )
            {
                // dispatch work
                auto work = std::move(*CurrentThreadQueue::top().work.get());
                *CurrentThreadQueue::top().work.get() = nullptr;
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
        
    public:
        ImmediateScheduler()
        {
        }
        virtual ~ImmediateScheduler()
        {
        }

        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto ct = std::make_shared<CurrentThreadScheduler>();
            std::this_thread::sleep_until(dueTime);
            Do(work, ct);
            return Disposable::Empty();
        }
    };
    
    template<class T>
    class ScheduledObserver : 
        public Observer<T>, 
        public std::enable_shared_from_this<ScheduledObserver<T>>
    {
        typedef std::function<void()> Action;

        Scheduler::shared scheduler;
        std::shared_ptr<Observer<T>> observer;
        std::atomic<size_t> trampoline;
        SharedDisposable sd;
        mutable std::queue<Action> queue;
        mutable std::mutex lock;

    public:
        ScheduledObserver(Scheduler::shared scheduler, std::shared_ptr<Observer<T>> observer)
            : scheduler(std::move(scheduler))
            , observer(std::move(observer))
            , trampoline(0)
        {
        }

        void Dispose() const
        {
            std::unique_lock<std::mutex> guard(lock);
            while (!queue.empty()) {queue.pop();}
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
            auto local = this->shared_from_this();
            queue.push(Action([=](){
                local->observer->OnNext(std::move(element));}));
        }
        virtual void OnCompleted() 
        {
            std::unique_lock<std::mutex> guard(lock);
            auto local = this->shared_from_this();
            queue.push(Action([=](){
                local->observer->OnCompleted();}));
        }
        virtual void OnError(const std::exception_ptr& error) 
        {
            std::unique_lock<std::mutex> guard(lock);
            auto local = this->shared_from_this();
            queue.push(Action([=](){
                local->observer->OnError(std::move(error));}));
        }

        void EnsureActive()
        {
            RXCPP_UNWIND_AUTO([&]{--trampoline;});
            if (++trampoline == 1)
            {
                // this is decremented when no further work is scheduled
                ++trampoline;
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

            std::unique_lock<std::mutex> guard(lock);

            if(!queue.empty())
            {
                auto& item = queue.front();
                
                // dispatch action
                auto action = std::move(item);
                item = nullptr;
                queue.pop();
                    
                RXCPP_UNWIND_AUTO([&]{guard.lock();});
                guard.unlock();
                action();
            }

            if(!queue.empty())
            {
                guard.unlock();
                sd.Set(sched->Schedule(
                    [keepAlive](Scheduler::shared sched){
                        return keepAlive->Run(sched);}));
                return Disposable::Empty();
            }

            // only decrement when no further work is scheduled
            --trampoline;
            return Disposable::Empty();
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

            std::atomic<size_t> trampoline;
            mutable std::mutex lock;
            mutable std::condition_variable wake;
            mutable std::atomic<bool> exit;
            CurrentThreadQueue::ThreadLocalQueue* queue;

        public:
            Derecurser()
                : trampoline(0)
                , exit(false)
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
                EnsureThreadResult result(util::maybe<std::thread>(), Disposable::Empty());

                auto cancelable = std::make_shared<Work>(std::move(work));

                std::unique_lock<std::mutex> guard(lock);
                RXCPP_UNWIND(unwindTrampoline, [&]{--trampoline;});

                if (++trampoline == 1)
                {
                    RXCPP_UNWIND(unwindQueue, [&](){
                        CurrentThreadQueue::DestroyQueue(queue); queue = nullptr;});
                    queue = CurrentThreadQueue::CreateQueue(owner).release();

                    std::get<1>(result) = Disposable([cancelable]{
                        *cancelable.get() = nullptr;});
                    queue->queue.push(CurrentThreadQueue::QueueItem(dueTime, std::move(cancelable)));

                    auto local = std::static_pointer_cast<Derecurser>(shared_from_this());
                    auto localQueue = queue;
                    std::get<0>(result).set(factory([local, localQueue]{
                               local->Run(localQueue);}));

                    // trampoline and queue lifetime is now owned by the thread
                    unwindTrampoline.dismiss();
                    unwindQueue.dismiss();
                }
                else
                {
                    std::get<1>(result) = Disposable([cancelable]{
                        *cancelable.get() = nullptr;});
                    queue->queue.push(CurrentThreadQueue::QueueItem(dueTime, std::move(cancelable)));
                    wake.notify_one();
                }
                return std::move(result);
            }

        private:
            void Run(CurrentThreadQueue::ThreadLocalQueue* queue) {
                auto keepAlive = shared_from_this();
                {
                    RXCPP_UNWIND_AUTO([&]{--trampoline;});

                    std::unique_lock<std::mutex> guard(lock);

                    CurrentThreadQueue::SetQueue(queue);
                    RXCPP_UNWIND(unwindQueue, [&](){
                        CurrentThreadQueue::DestroyQueue(); 
                        queue = nullptr;});

                    while(!exit && (!CurrentThreadQueue::empty() || trampoline > 1))
                    {
                        if (CurrentThreadQueue::empty()) {
                            wake.wait(guard, [&](){
                                return exit || !CurrentThreadQueue::empty() || trampoline == 1;});
                            continue;
                        }

                        auto item = &CurrentThreadQueue::top();
                        if (!item->work || !*item->work.get()) {CurrentThreadQueue::pop(); continue;}
                        
                        // wait until the work is due
                        if (!exit && queue->scheduler->Now() < item->due)
                        {
                            wake.wait_until(guard, item->due);
                            continue;
                        }
                        if (exit || CurrentThreadQueue::empty()) {break;}
                        if (!item->work || !*item->work.get()) {CurrentThreadQueue::pop(); continue;}
                        
                        // dispatch work
                        auto work = std::move(*item->work.get());
                        *item->work.get() = nullptr;
                        CurrentThreadQueue::pop();
                        
                        RXCPP_UNWIND_AUTO([&]{guard.lock();});
                        guard.unlock();
                        LocalScheduler::Do(work, queue->scheduler);
                    }
                }
                exit = false;
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
            : derecurser(std::make_shared<Derecurser>())
            , factory(std::move(factoryarg))
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
}

#endif
