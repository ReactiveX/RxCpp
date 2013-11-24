 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_FOREACH_HPP)
#define CPPRX_RX_OPERATORS_FOREACH_HPP

namespace rxcpp
{

    template <class T>
    void ForEach(
        const std::shared_ptr<Observable<T>>& source,
        typename util::identity<std::function<void(const T&)>>::type onNext
        )
    {
        std::mutex lock;
        std::condition_variable wake;
        bool done = false;
        std::exception_ptr error;
        auto observer = CreateObserver<T>(std::move(onNext), 
        //on completed
            [&]{
                std::unique_lock<std::mutex> guard(lock);
                done = true;
                wake.notify_one();
            }, 
        //on error
            [&](const std::exception_ptr& e){
                std::unique_lock<std::mutex> guard(lock);
                done = true;
                error = std::move(e);
                wake.notify_one();
            });
        
        source->Subscribe(observer);

        {
            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&]{return done;});
        }

        if (error != std::exception_ptr()) {
            std::rethrow_exception(error);}
    }
}

#endif