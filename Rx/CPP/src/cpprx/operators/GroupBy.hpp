 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_GROUPBY_HPP)
#define CPPRX_RX_OPERATORS_GRUOPBY_HPP

namespace rxcpp
{

    template <class T, class KS, class VS, class L>
    auto GroupBy(
        const std::shared_ptr<Observable<T>>& source,
        KS keySelector,
        VS valueSelector,
        L less) 
        -> std::shared_ptr<Observable<std::shared_ptr<GroupedObservable<
            typename std::decay<decltype(keySelector((*(T*)0)))>::type, 
            typename std::decay<decltype(valueSelector((*(T*)0)))>::type>>>>
    {
        typedef typename std::decay<decltype(keySelector((*(T*)0)))>::type Key;
        typedef typename std::decay<decltype(valueSelector((*(T*)0)))>::type Value;

        typedef std::shared_ptr<GroupedObservable<Key, Value>> LocalGroupObservable;

        return CreateObservable<LocalGroupObservable>(
            [=](std::shared_ptr<Observer<LocalGroupObservable>> observer) -> Disposable
            {
                typedef std::map<Key, std::shared_ptr<GroupedSubject<Key, Value>>, L> Groups;

                struct State
                {
                    explicit State(L less) : groups(std::move(less)) {}
                    std::mutex lock;
                    Groups groups;
                };
                auto state = std::make_shared<State>(std::move(less));

                return Subscribe(
                    source,
                // on next
                    [=](const T& element)
                    {
                        util::maybe<Key> key;
                        try {
                            key.set(keySelector(element));
                        } catch(...) {
                            observer->OnError(std::current_exception());
                        }

                        if (!!key) {
                            auto keySubject = CreateGroupedSubject<Value>(*key.get());

                            typename Groups::iterator groupIt;
                            bool newGroup = false;

                            {
                                std::unique_lock<std::mutex> guard(state->lock);
                                std::tie(groupIt, newGroup) = state->groups.insert(
                                    std::make_pair(*key.get(), keySubject)
                                );
                            }

                            if (newGroup)
                            {
                                LocalGroupObservable nextGroup(std::move(keySubject));
                                observer->OnNext(nextGroup);
                            }

                            util::maybe<Value> result;
                            try {
                                result.set(valueSelector(element));
                            } catch(...) {
                                observer->OnError(std::current_exception());
                            }
                            if (!!result) {
                                groupIt->second->OnNext(std::move(*result.get()));
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        for(auto& group : state->groups) {
                            group.second->OnCompleted();
                        }
                        observer->OnCompleted();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        for(auto& group : state->groups) {
                            group.second->OnError(error);
                        }
                        observer->OnError(error);
                    });
            });
    }
}

#endif