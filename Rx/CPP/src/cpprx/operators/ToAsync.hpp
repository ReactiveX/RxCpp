 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_TOASYNC_HPP)
#define CPPRX_RX_OPERATORS_TOASYNC_HPP

namespace rxcpp
{

#if RXCPP_USE_VARIADIC_TEMPLATES
    template<class... A, class F>
    auto ToAsync(F f, Scheduler::shared scheduler = nullptr)
        ->std::function < std::shared_ptr < Observable< decltype(f((*(A*)nullptr)...)) >> (const A&...)>
    {
        typedef decltype(f((*(A*) nullptr)...)) R;
        if (!scheduler)
        {
            scheduler = std::make_shared<EventLoopScheduler>();
        }
        return [=](const A&... a) -> std::shared_ptr < Observable<R >>
        {
            auto args = std::make_tuple(a...);
            auto result = CreateAsyncSubject<R>();
            scheduler->Schedule([=](Scheduler::shared) -> Disposable
            {
                util::maybe<R> value;
                try
                {
                    value.set(util::tuple_dispatch(f, args));
                }
                catch (...)
                {
                    result->OnError(std::current_exception());
                    return Disposable::Empty();
                }
                result->OnNext(*value.get());
                result->OnCompleted();
                return Disposable::Empty();
            });
            return result;
        };
    }
#endif
}

#endif