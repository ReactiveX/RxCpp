 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SELECT_HPP)
#define CPPRX_RX_OPERATORS_SELECT_HPP

namespace rxcpp
{

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
                        util::maybe<U> result;
                        try {
                            result.set(selector(element));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }
                        if (!!result) {
                            observer->OnNext(std::move(*result.get()));
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
}

#endif