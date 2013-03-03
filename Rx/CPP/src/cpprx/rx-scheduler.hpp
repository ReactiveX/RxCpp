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

    struct CurrentThreadScheduler : public LocalScheduler
    {
    private:
        CurrentThreadScheduler(const CurrentThreadScheduler&);
        
        struct compare_work
        {
            template <class T>
            bool operator()(const T& work1, const T& work2) const {
                return work1.first > work2.first;
            }
        };
        
        typedef std::priority_queue<
            std::pair<clock::time_point, std::shared_ptr<Work>>,
            std::vector<std::pair<clock::time_point, std::shared_ptr<Work>>>,
            compare_work 
        > ScheduledWork;

        RXCPP_THREAD_LOCAL static ScheduledWork* scheduledWork;
        
    public:
        CurrentThreadScheduler()
        {
        }
        virtual ~CurrentThreadScheduler()
        {
        }

        static bool IsScheduleRequired() { return scheduledWork == nullptr; }
        
        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto cancelable = std::make_shared<Work>(std::move(work));
            // check trampoline
            if (!!scheduledWork) 
            {
                scheduledWork->push(std::make_pair(dueTime, cancelable));
                return Disposable([=]{(*cancelable.get()) = nullptr;});
            }

            // create and publish new queue 
            scheduledWork = new ScheduledWork();
            RXCPP_UNWIND_AUTO([]{
                delete scheduledWork;
                scheduledWork = nullptr;
            });

            scheduledWork->push(std::make_pair(dueTime, cancelable));

            // loop until queue is empty
            for (
                 auto dueTime = scheduledWork->top().first;
                 std::this_thread::sleep_until(dueTime), true;
                 dueTime = scheduledWork->top().first
                 )
            {
                // dispatch work
                auto work = std::move(*scheduledWork->top().second.get());
                scheduledWork->pop();
                
                Do(work, get());

                if (scheduledWork->empty()) {break;}
            }

            return Disposable::Empty();
        }
    };
    // static 
    RXCPP_SELECT_ANY RXCPP_THREAD_LOCAL CurrentThreadScheduler::ScheduledWork* CurrentThreadScheduler::scheduledWork = nullptr;

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
            Do(work, ct);
            return Disposable::Empty();
        }
    };
    

    class WorkQueue : public std::enable_shared_from_this<WorkQueue>
    {
        typedef LocalScheduler::Work Work;
        typedef LocalScheduler::clock clock;
        typedef std::pair<Scheduler::shared, Work> Item;
        typedef std::shared_ptr<WorkQueue> shared;

        struct compare_work
        {
            template <class T>
            bool operator()(const T& work1, const T& work2) const {
                return work1.first > work2.first;
            }
        };
        
        std::atomic<size_t> trampoline;
        std::priority_queue< 
            std::pair<clock::time_point, std::shared_ptr<Item>>,
            std::vector<std::pair<clock::time_point, std::shared_ptr<Item>>>,
            compare_work > scheduledWork;
        mutable std::mutex lock;
        mutable std::condition_variable wake;
        mutable std::atomic<bool> exit;

        struct WorkQueueScheduler : public LocalScheduler
        {
        private:
            WorkQueueScheduler();
            WorkQueueScheduler(const WorkQueueScheduler&);
         
            WorkQueue::shared queue;

        public:
            WorkQueueScheduler(WorkQueue::shared queue) : queue(queue)
            {
            }
            virtual ~WorkQueueScheduler()
            {
            }

            using LocalScheduler::Schedule;
            virtual Disposable Schedule(clock::time_point dueTime, Work work)
            {
                auto cancelable = std::make_shared<Item>(std::make_pair(get(), std::move(work)));
                {
                    std::unique_lock<std::mutex> guard(queue->lock);
                    queue->scheduledWork.push(std::make_pair(dueTime, cancelable));
                }
                queue->wake.notify_one();
                auto local = queue;
                return Disposable([local, cancelable]{
                    std::unique_lock<std::mutex> guard(local->lock);
                    cancelable.get()->second = nullptr;});
            }
        };

    public:
        typedef std::function<void()> RunLoop;
        typedef std::function<std::thread(RunLoop)> Factory;

        WorkQueue()
            : trampoline(0)
            , exit(false)
        {
        }

        util::maybe<std::thread> EnsureThread(Factory& factory)
        {
            util::maybe<std::thread> result;
            std::unique_lock<std::mutex> guard(lock);
            RXCPP_UNWIND(unwindTrampoline, [&]{--trampoline;});
            if (++trampoline == 1)
            {
                auto local = shared_from_this();
                result.set(factory([local]{local->Run();}));
                // trampoline lifetime is now owned by the thread
                unwindTrampoline.dismiss();
            }
            return result;
        }

        void Dispose() const
        {
            std::unique_lock<std::mutex> guard(lock);
            if (trampoline > 0)
            {
                exit = true;
                wake.notify_one();
            }
        }
        operator Disposable() const
        {
            auto local = shared_from_this();
            return Disposable([local]{
                local->Dispose();
            });
        }
        Scheduler::shared GetScheduler()
        {
            return std::make_shared<WorkQueueScheduler>(shared_from_this());
        }
        void Run()
        {
            auto keepAlive = shared_from_this();
            {
                RXCPP_UNWIND_AUTO([&]{--trampoline;});

                std::unique_lock<std::mutex> guard(lock);

                while(!exit && !scheduledWork.empty())
                {
                    // wait until there is work
                    wake.wait(guard, [this]{ return this->exit || !this->scheduledWork.empty();});
                    if (exit || scheduledWork.empty()) {break;}
                
                    auto item = &scheduledWork.top();
                    if (!item->second) {scheduledWork.pop(); continue;}
                
                    // wait until the work is due
                    while (!exit && item->second && item->second.get()->first->Now() < item->first)
                    {
                        wake.wait_until(guard, item->first);
                        item = &scheduledWork.top();
                    }
                    if (exit || scheduledWork.empty()) {break;}
                    if (!item->second) {scheduledWork.pop(); continue;}
                
                    // dispatch work
                    auto work = std::move(item->second.get()->second);
                    auto scheduler = std::move(item->second.get()->first);
                    scheduledWork.pop();
                
                    RXCPP_UNWIND_AUTO([&]{guard.lock();});
                    guard.unlock();
                    LocalScheduler::Do(work, scheduler);
                }
            }
            exit = false;
        }

        Disposable RunOnScheduler(Scheduler::shared scheduler)
        {
            RXCPP_UNWIND_AUTO([&]{--trampoline;});
            if (++trampoline == 1)
            {
                // this is decremented when no further work is scheduled
                ++trampoline;
                auto keepAlive = shared_from_this();
                return scheduler->Schedule(
                    [keepAlive, scheduler](Scheduler::shared){
                        return keepAlive->RunOnSchedulerWork(scheduler);});
            }
            return Disposable::Empty();
        }
    private:
        Disposable RunOnSchedulerWork(Scheduler::shared scheduler)
        {
            auto keepAlive = shared_from_this();

            std::unique_lock<std::mutex> guard(lock);

            if(!exit && !scheduledWork.empty())
            {                
                auto& item = scheduledWork.top();
                
                // wait until the work is due
                if(!exit && item.second.get()->first->Now() < item.first)
                {
                    return scheduler->Schedule(
                        item.first, 
                        [keepAlive, scheduler](Scheduler::shared){
                            return keepAlive->RunOnSchedulerWork(scheduler);});
                }
                
                if(!exit)
                {
                    // dispatch work
                    auto work = std::move(item.second.get()->second);
                    auto scheduler = std::move(item.second.get()->first);
                    scheduledWork.pop();
                    
                    RXCPP_UNWIND_AUTO([&]{guard.lock();});
                    guard.unlock();
                    LocalScheduler::Do(work, scheduler);
                }
            }

            if(!exit && !scheduledWork.empty())
            {
                return scheduler->Schedule(
                    [keepAlive, scheduler](Scheduler::shared){
                        return keepAlive->RunOnSchedulerWork(scheduler);});
            }

            // only decrement when no further work is scheduled
            --trampoline;
            exit = false;
            return Disposable::Empty();
        }
    };


    struct EventLoopScheduler : public LocalScheduler
    {
    private:
        EventLoopScheduler(const EventLoopScheduler&);
        
        std::shared_ptr<WorkQueue> queue;
        Scheduler::shared scheduler;
        std::thread worker;
        WorkQueue::Factory factory;
        
    public:
        EventLoopScheduler()
            : queue(std::make_shared<WorkQueue>())
            , scheduler(queue->GetScheduler())
        {
            auto local = queue;
            factory = [local] (WorkQueue::RunLoop rl) -> std::thread {
                return std::thread([local, rl]{rl();});
            };
        }
        template<class Factory>
        EventLoopScheduler(Factory factoryarg) 
            : queue(std::make_shared<WorkQueue>())
            , scheduler(queue->GetScheduler())
        {
            auto local = queue;
            factory = std::move(factoryarg);
        }
        virtual ~EventLoopScheduler()
        {
            if (worker.joinable()) {
                worker.detach();
            }
        }
        
        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            Disposable result = scheduler->Schedule(dueTime, std::move(work));

            auto maybeThread = queue->EnsureThread(factory);
            if (maybeThread)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
                worker = std::move(*maybeThread.get());
            }
            return std::move(result);
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
