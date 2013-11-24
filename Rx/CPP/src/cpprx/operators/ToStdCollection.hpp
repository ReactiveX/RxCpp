 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_TOSTDCOLLECTION_HPP)
#define CPPRX_RX_OPERATORS_TOSTDCOLLECTION_HPP

namespace rxcpp
{

    template <class StdCollection>
    std::shared_ptr<Observable<StdCollection>> ToStdCollection(
        const std::shared_ptr<Observable<typename StdCollection::value_type>>& source
        )
    {
        typedef typename StdCollection::value_type Value;
        return CreateObservable<StdCollection>(
            [=](std::shared_ptr<Observer<StdCollection>> observer) -> Disposable
            {
                auto stdCollection = std::make_shared<StdCollection>();
                return Subscribe(
                    source,
                // on next
                    [=](const Value& element)
                    {
                        stdCollection->insert(stdCollection->end(), element);
                    },
                // on completed
                    [=]
                    {
                        observer->OnNext(std::move(*stdCollection.get()));
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