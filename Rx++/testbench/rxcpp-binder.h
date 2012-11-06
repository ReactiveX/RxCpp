// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

namespace rxcpp
{
    template <class Obj>
    class Binder
    {
        Obj obj;
    public:
        Binder(Obj&& obj) : obj(std::move(obj))
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
        auto delay(int milliseconds) -> decltype(from(Delay(obj, milliseconds))) {
            return from(Delay(obj, milliseconds));
        }
        auto limit_window(int milliseconds) -> decltype(from(LimitWindow(obj, milliseconds))) {
            return from(LimitWindow(obj, milliseconds));
        }
        auto distinct_until_changed() -> decltype(from(DistinctUntilChanged(obj))) {
            return from(DistinctUntilChanged(obj));
        }
        auto on_dispatcher() -> decltype(from(ObserveOnDispatcher(obj)))
        {
            return from(ObserveOnDispatcher(obj));
        }
        template <class OnNext>
        auto subscribe(OnNext onNext) -> decltype(Subscribe(obj, onNext)) {
            return Subscribe(obj, onNext);
        }
    };
    template <class Obj>
    Binder<Obj> from(Obj&& obj) { return Binder<Obj>(std::move(obj)); }

}