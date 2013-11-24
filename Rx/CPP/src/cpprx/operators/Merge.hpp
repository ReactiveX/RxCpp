 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_MERGE_HPP)
#define CPPRX_RX_OPERATORS_MERGE_HPP

namespace rxcpp
{

    namespace detail{
        template<size_t Index, size_t SourcesSize, class SubscribeState>
        struct MergeSubscriber {
            typedef typename SubscribeState::result_type Item;
            typedef std::shared_ptr<Observer<typename SubscribeState::result_type>> ResultObserver;
            static void subscribe(
                ComposableDisposable& cd, 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& observer, 
                const std::shared_ptr<SubscribeState>& state,
                const typename SubscribeState::Sources& sources) {
                cd.Add(Subscribe(
                    std::get<Index>(sources),
                // on next
                    [=](const Item& element)
                    {
                        observer->OnNext(element);
                    },
                // on completed
                    [=]
                    {
                        if (--state->pendingComplete == 0) {
                            observer->OnCompleted();
                            cd.Dispose();
                        }
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                MergeSubscriber<Index + 1, SourcesSize, SubscribeState>::
                    subscribe(cd, observer, state, sources);
            }
        };
        template<size_t SourcesSize, class SubscribeState>
        struct MergeSubscriber<SourcesSize, SourcesSize, SubscribeState> {
            static void subscribe(
                ComposableDisposable& , 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& , 
                const std::shared_ptr<SubscribeState>& ,
                const typename SubscribeState::Sources& ) {}
        };
    }
#if RXCPP_USE_VARIADIC_TEMPLATES
    template <class MergeSource, class... MergeSourceNext>
    std::shared_ptr<Observable<MergeSource>> Merge(
        const std::shared_ptr<Observable<MergeSource>>& firstSource,
        const std::shared_ptr<Observable<MergeSourceNext>>&... otherSource
        )
    {
        typedef MergeSource result_type;
        typedef decltype(std::make_tuple(firstSource, otherSource...)) Sources;
        struct State {
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            State()
                : pendingComplete(SourcesSize::value)
            {}
            std::atomic<size_t> pendingComplete;
        };
        Sources sources(firstSource, otherSource...);
        // bug on osx prevents using make_shared
        std::shared_ptr<State> state(new State());
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::MergeSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#else
    template <class MergeSource, class MergeSourceNext>
    std::shared_ptr<Observable<MergeSource>> Merge(
        const std::shared_ptr<Observable<MergeSource>>& firstSource,
        const std::shared_ptr<Observable<MergeSourceNext>>& otherSource
        )
    {
        typedef MergeSource result_type;
        typedef decltype(std::make_tuple(firstSource, otherSource)) Sources;
        struct State {
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            State()
                : pendingComplete(SourcesSize::value)
            {}
            std::atomic<size_t> pendingComplete;
        };
        Sources sources(firstSource, otherSource);
        // bug on osx prevents using make_shared
        std::shared_ptr<State> state(new State());
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::MergeSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#endif //RXCPP_USE_VARIADIC_TEMPLATES
}

#endif