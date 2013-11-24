 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_SUBSCRIBE_HPP)
#define CPPRX_RX_OPERATORS_SUBSCRIBE_HPP

namespace rxcpp
{

    template <class S>
    Disposable Subscribe(
        const S& source,
        typename util::identity<std::function<void(const typename observable_item<S>::type&)>>::type onNext,
        std::function<void()> onCompleted = nullptr,
        std::function<void(const std::exception_ptr&)> onError = nullptr
        )
    {
        auto observer = CreateObserver<typename observable_item<S>::type>(
            std::move(onNext), std::move(onCompleted), std::move(onError));
        
        return source->Subscribe(observer);
    }
}

#endif