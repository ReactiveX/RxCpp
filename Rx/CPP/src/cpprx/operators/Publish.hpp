 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_PUBLISH_HPP)
#define CPPRX_RX_OPERATORS_PUBLISH_HPP

namespace rxcpp
{

    template <class T>
    std::shared_ptr<ConnectableObservable<T>> Publish(const std::shared_ptr < Observable < T >> &source)
    {
        auto multicastSubject = std::make_shared<Subject<T>>();
        return Multicast(source, multicastSubject);
    }

    template <class T, class V>
    std::shared_ptr<ConnectableObservable<T>> Publish(const std::shared_ptr < Observable < T >> &source, V value)
    {
        auto multicastSubject = std::make_shared<BehaviorSubject<T>>(value);
        return Multicast(source, multicastSubject);
    }

    template <class T>
    std::shared_ptr<ConnectableObservable<T>> PublishLast(const std::shared_ptr < Observable < T >> &source)
    {
        auto multicastSubject = std::make_shared<AsyncSubject<T>>();
        return Multicast(source, multicastSubject);
    }
}

#endif