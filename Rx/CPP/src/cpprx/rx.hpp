// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPRX_RX_HPP)
#define CPPRX_RX_HPP
#pragma once

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

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
#include <future>
#include <vector>
#include <queue>
#include <chrono>
#include <condition_variable>


#include "rx-util.hpp"
#include "rx-base.hpp"
#include "rx-scheduler.h"
#include "rx-windows.hpp"
#include "rx-operators.hpp"

namespace rxcpp
{    
    template <class Obj>
    class Binder
    {
        Obj obj;
    public:
        Binder(Obj obj) : obj(std::move(obj))
        {
        }
        template <class S>
        auto select(S selector) -> decltype(from(Select(obj, selector))) {
            return from(Select(obj, selector));
        }
        template <class P>
        auto where(P predicate) -> decltype(from(Where(obj, predicate))) {
            return from(Where(obj, predicate));
        }
        template <class Integral>
        auto take(Integral n) -> decltype(from(Take(obj, n))) {
            return from(Take(obj, n));
        }
        auto delay(Scheduler::clock::duration due) -> decltype(from(Delay(obj, due))) {
            return from(Delay(obj, due));
        }
        auto delay(Scheduler::clock::duration due, Scheduler::shared scheduler) -> decltype(from(Delay(obj, due, scheduler))) {
            return from(Delay(obj, due, scheduler));
        }
        auto limit_window(int milliseconds) -> decltype(from(LimitWindow(obj, milliseconds))) {
            return from(LimitWindow(obj, milliseconds));
        }
        auto distinct_until_changed() -> decltype(from(DistinctUntilChanged(obj))) {
            return from(DistinctUntilChanged(obj));
        }
        auto subscribe_on(Scheduler::shared scheduler)
        -> decltype(from(SubscribeOnObservable(obj, std::move(scheduler))))
        {
            return from(SubscribeOnObservable(obj, std::move(scheduler)));
        }
        auto observe_on(Scheduler::shared scheduler)
        -> decltype(from(ObserveOnObserver(obj, std::move(scheduler))))
        {
            return from(ObserveOnObserver(obj, std::move(scheduler)));
        }
        auto on_dispatcher() 
        -> decltype(from(ObserveOnDispatcher(obj)))
        {
            return from(ObserveOnDispatcher(obj));
        }
        template <class OnNext>
        auto subscribe(OnNext onNext) -> decltype(Subscribe(obj, onNext)) {
            auto result = Subscribe(obj, onNext);
            return result;
        }
        template <class OnNext, class OnComplete>
        auto subscribe(OnNext onNext, OnComplete onComplete) -> decltype(Subscribe(obj, onNext, onComplete)) {
            auto result = Subscribe(obj, onNext, onComplete);
            return result;
        }
        template <class OnNext, class OnComplete, class OnError>
        auto subscribe(OnNext onNext, OnComplete onComplete, OnError onError) 
            -> decltype(Subscribe(obj, onNext, onComplete, onError)) {
            auto result = Subscribe(obj, onNext, onComplete, onError);
            return result;
        }
    };
    template <class Obj>
    Binder<typename std::remove_reference<Obj>::type> from(Obj&& obj) { 
        return Binder<typename std::remove_reference<Obj>::type>(std::forward<Obj>(obj)); }

}

#pragma pop_macro("min")
#pragma pop_macro("max")

#endif
