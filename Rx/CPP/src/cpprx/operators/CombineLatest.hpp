 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_COMBINELATEST_HPP)
#define CPPRX_RX_OPERATORS_COMBINELATEST_HPP

namespace rxcpp
{

    namespace detail{
        template<size_t Index, size_t SourcesSize, class SubscribeState>
        struct CombineLatestSubscriber {
            typedef typename SubscribeState::Latest Latest;
            static void subscribe(
                ComposableDisposable& cd, 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& observer, 
                const std::shared_ptr<SubscribeState>& state, 
                const typename SubscribeState::Sources& sources) {
                cd.Add(Subscribe(
                    std::get<Index>(sources),
                // on next
                    [=](const typename std::tuple_element<Index, Latest>::type& element)
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        if (state->done) {return;}
                        std::get<Index>(state->latest) = element;
                        if (!std::get<Index>(state->latestValid)) {
                            std::get<Index>(state->latestValid) = true;
                            --state->pendingFirst;
                        }
                        if (state->pendingFirst == 0) {
                            auto args = state->latest;
                            typedef decltype(util::tuple_dispatch(state->selector, args)) U;
                            util::maybe<U> result;
                            try {
                                result.set(util::tuple_dispatch(state->selector, args));
                            } catch(...) {
                                observer->OnError(std::current_exception());
                            }
                            if (!!result) {
                                observer->OnNext(std::move(*result.get()));
                            }
                        }
                    },
                // on completed
                    [=]
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        if (state->done) {return;}
                        state->done = true;
                        observer->OnCompleted();
                        cd.Dispose();
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        if (state->done) {return;}
                        state->done = true;
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                CombineLatestSubscriber<Index + 1, SourcesSize, SubscribeState>::
                    subscribe(cd, observer, state, sources);
            }
        };
        template<size_t SourcesSize, class SubscribeState>
        struct CombineLatestSubscriber<SourcesSize, SourcesSize, SubscribeState> {
            static void subscribe(
                ComposableDisposable& , 
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& , 
                const std::shared_ptr<SubscribeState>& , 
                const typename SubscribeState::Sources& ) {}
        };
    }

#if RXCPP_USE_VARIADIC_TEMPLATES
    template <class... CombineLSource, class S>
    auto CombineLatest(
        S selector,
        const std::shared_ptr<Observable<CombineLSource>>&... source
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const CombineLSource&...)>::type>>
    {
        typedef typename std::result_of<S(const CombineLSource&...)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<CombineLSource>>...> Sources;
        typedef std::tuple<CombineLSource...> Latest;
        typedef decltype(std::make_tuple((source, true)...)) LatestValid;
        struct State {
            typedef Latest Latest;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : latestValid()
                , pendingFirst(SourcesSize::value)
                , done(false)
                , selector(std::move(selector))
            {}
            std::mutex lock;
            LatestValid latestValid;
            size_t pendingFirst;
            bool done;
            S selector;
            Latest latest;
        };
        Sources sources(source...);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(selector));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                cd.Add(Disposable([state](){state->done = true;}));
                detail::CombineLatestSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#else
    template <class CombineLSource1, class CombineLSource2, class S>
    auto CombineLatest(
        S selector,
        const std::shared_ptr<Observable<CombineLSource1>>& source1,
        const std::shared_ptr<Observable<CombineLSource2>>& source2
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const CombineLSource1&, const CombineLSource2&)>::type>>
    {
        typedef typename std::result_of<S(const CombineLSource1&, const CombineLSource2&)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<CombineLSource1>>, std::shared_ptr<Observable<CombineLSource2>>> Sources;
        typedef std::tuple<CombineLSource1, CombineLSource2> Latest;
        typedef std::tuple<bool, bool> LatestValid;
        struct State {
            typedef Latest Latest;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : latestValid()
                , pendingFirst(SourcesSize::value)
                , done(false)
                , selector(std::move(selector))
            {}
            std::mutex lock;
            LatestValid latestValid;
            size_t pendingFirst;
            bool done;
            S selector;
            Latest latest;
        };
        Sources sources(source1, source2);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(std::move(selector)));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                cd.Add(Disposable([state](){state->done = true;}));
                detail::CombineLatestSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#endif

}

#endif