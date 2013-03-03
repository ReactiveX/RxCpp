// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_HPP)
#define CPPRX_RX_HPP

namespace rxcpp
{    
    template<class T, class Obj>
    class Binder
    {
        Obj obj;
        static T defaultValueSelector(T t){return std::move(t);}
    public:
        Binder(Obj obj) : obj(std::move(obj))
        {
        }
        template <class S>
        auto select(S selector) -> decltype(from(Select<T>(obj, selector))) {
            return from(Select<T>(obj, selector));
        }
        template <class P>
        auto where(P predicate) -> decltype(from(Where<T>(obj, predicate))) {
            return from(Where<T>(obj, predicate));
        }
        Obj publish() {
            return obj;
        }
        template <class KS>
        auto group_by(
            KS keySelector)
            -> decltype(from(GroupBy<T>(obj, keySelector, defaultValueSelector, std::less<decltype(keySelector((*(T*)0)))>()))) {
            return from(GroupBy<T>(obj, keySelector, defaultValueSelector, std::less<decltype(keySelector((*(T*)0)))>()));
        }
        template <class KS, class VS>
        auto group_by(
            KS keySelector,
            VS valueSelector)
            -> decltype(from(GroupBy<T>(obj, keySelector, valueSelector, std::less<decltype(keySelector((*(T*)0)))>()))) {
            return from(GroupBy<T>(obj, keySelector, valueSelector, std::less<decltype(keySelector((*(T*)0)))>()));
        }
        template <class KS, class VS, class L>
        auto group_by(
            KS keySelector,
            VS valueSelector,
            L less)
            -> decltype(from(GroupBy<T>(obj, keySelector, valueSelector, less))) {
            return from(GroupBy<T>(obj, keySelector, valueSelector, less));
        }
        template <class Integral>
        auto take(Integral n) -> decltype(from(Take<T>(obj, n))) {
            return from(Take<T>(obj, n));
        }
        auto delay(Scheduler::clock::duration due) -> decltype(from(Delay<T>(obj, due))) {
            return from(Delay<T>(obj, due));
        }
        auto delay(Scheduler::clock::duration due, Scheduler::shared scheduler) -> decltype(from(Delay<T>(obj, due, scheduler))) {
            return from(Delay<T>(obj, due, scheduler));
        }
        auto limit_window(int milliseconds) -> decltype(from(LimitWindow<T>(obj, milliseconds))) {
            return from(LimitWindow<T>(obj, milliseconds));
        }
        auto distinct_until_changed() -> decltype(from(DistinctUntilChanged<T>(obj))) {
            return from(DistinctUntilChanged<T>(obj));
        }
        auto subscribe_on(Scheduler::shared scheduler)
        -> decltype(from(SubscribeOnObservable<T>(obj, std::move(scheduler))))
        {
            return from(SubscribeOnObservable<T>(obj, std::move(scheduler)));
        }
        auto observe_on(Scheduler::shared scheduler)
        -> decltype(from(ObserveOnObserver<T>(obj, std::move(scheduler))))
        {
            return from(ObserveOnObserver<T>(obj, std::move(scheduler)));
        }
        auto on_dispatcher() 
        -> decltype(from(ObserveOnDispatcher<T>(obj)))
        {
            return from(ObserveOnDispatcher<T>(obj));
        }
        template <class OnNext>
        void for_each(OnNext onNext) {
            ForEach<T>(obj, onNext);
        }
        template <class OnNext>
        auto subscribe(OnNext onNext) -> decltype(Subscribe<T>(obj, onNext)) {
            auto result = Subscribe<T>(obj, onNext);
            return result;
        }
        template <class OnNext, class OnComplete>
        auto subscribe(OnNext onNext, OnComplete onComplete) 
            -> decltype(Subscribe<T>(obj, onNext, onComplete)) {
            auto result = Subscribe<T>(obj, onNext, onComplete);
            return result;
        }
        template <class OnNext, class OnComplete, class OnError>
        auto subscribe(OnNext onNext, OnComplete onComplete, OnError onError) 
            -> decltype(Subscribe<T>(obj, onNext, onComplete, onError)) {
            auto result = Subscribe<T>(obj, onNext, onComplete, onError);
            return result;
        }
    };

    template<class T>
    Binder<T, std::shared_ptr<Observable<T>>> from(std::shared_ptr<Observable<T>> obj) { 
        return Binder<T, std::shared_ptr<Observable<T>>>(std::move(obj)); }

    template<class K, class T>
    Binder<T, std::shared_ptr<GroupedObservable<K, T>>> from(std::shared_ptr<GroupedObservable<K, T>> obj) { 
        return Binder<T, std::shared_ptr<GroupedObservable<K, T>>>(std::move(obj)); }

    template<class T, class Obj>
    Binder<T, Obj> from(Binder<T, Obj> binder) { 
        return std::move(binder); }

    template<class T>
    T item(const Binder<T, std::shared_ptr<Observable<T>>>&);

    template<class T, class K>
    T item(const Binder<T, std::shared_ptr<GroupedObservable<K, T>>>&);

    template<class T>
    T item(const std::shared_ptr<Observable<T>>&);

    template<class K, class T>
    T item(const std::shared_ptr<GroupedObservable<K,T>>&);
}

#endif
