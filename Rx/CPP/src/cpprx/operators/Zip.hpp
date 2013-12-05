 // Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "../rx-operators.hpp"

#if !defined(CPPRX_RX_OPERATORS_ZIP_HPP)
#define CPPRX_RX_OPERATORS_ZIP_HPP

namespace rxcpp
{

    namespace detail{
        template<size_t Index, size_t SourcesSize, class SubscribeState>
        struct ZipSubscriber {
            typedef typename std::tuple_element<Index, typename SubscribeState::Queues>::type::second_type::value_type Item;
            typedef std::shared_ptr<Observer<typename SubscribeState::result_type>> ResultObserver;
            struct Next
            {
                std::shared_ptr<SubscribeState> state;
                const ResultObserver& observer;
                const ComposableDisposable& cd;
                explicit Next(
                    std::shared_ptr<SubscribeState> state,
                    const ResultObserver& observer,
                    const ComposableDisposable& cd)
                    : state(std::move(state))
                    , observer(observer)
                    , cd(cd) {
                }
#if RXCPP_USE_VARIADIC_TEMPLATES
                template<class... ZipQueue>
                void operator()(ZipQueue&... queue) {
                    // build array of bool that we can iterate to detect empty queues
                    bool empties[] = {queue.second.empty()...};
                    if (std::find(std::begin(empties), std::end(empties), true) == std::end(empties)) {
                        // all queues have an item.
                        //
                        // copy front of each queue
                        auto args = std::make_tuple(queue.second.front()...);
                        
                        // cause side-effect of pop on each queue
                        std::make_tuple((queue.second.pop(), true)...);
                        
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
                    // build new array to check for any empty queue
                    bool post_empties[] = {queue.first && queue.second.empty()...};
                    if (std::find(std::begin(post_empties), std::end(post_empties), true) != std::end(post_empties)) {
                        // at least one queue is empty and at least one of the sources has completed.
                        // it is time to stop.
                        observer->OnCompleted();
                        cd.Dispose();
                    }
                }
#else
                template<class ZipQueue1, class ZipQueue2>
                void operator()(ZipQueue1& queue1, ZipQueue2& queue2) {
                    // build array of bool that we can iterate to detect empty queues
                    bool empties[] = {queue1.second.empty(), queue2.second.empty()};
                    if (std::find(std::begin(empties), std::end(empties), true) == std::end(empties)) {
                        // all queues have an item.
                        //
                        // copy front of each queue
                        auto args = std::make_tuple(queue1.second.front(), queue2.second.front());
                        
                        queue1.second.pop();
                        queue2.second.pop();
                        
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
                    // build new array to check for any empty queue
                    bool post_empties[] = {queue1.first && queue1.second.empty(), queue2.first && queue2.second.empty()};
                    if (std::find(std::begin(post_empties), std::end(post_empties), true) != std::end(post_empties)) {
                        // at least one queue is empty and at least one of the sources has completed.
                        // it is time to stop.
                        observer->OnCompleted();
                        cd.Dispose();
                    }
                }
#endif //RXCPP_USE_VARIADIC_TEMPLATES
            };
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
                        std::unique_lock<std::mutex> guard(state->lock);
                        if (state->isStopped) {return;}
                        std::get<Index>(state->queues).second.push(element);
                        Next next(state, observer, cd);
                        util::tuple_dispatch(next, state->queues);
                    },
                // on completed
                    [=]
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        if (state->isStopped) {return;}
                        std::get<Index>(state->queues).first = true;
                        Next next(state, observer, cd);
                        util::tuple_dispatch(next, state->queues);
                    },
                // on error
                    [=](const std::exception_ptr& error)
                    {
                        std::unique_lock<std::mutex> guard(state->lock);
                        if (state->isStopped) {return;}
                        observer->OnError(error);
                        cd.Dispose();
                    }));
                ZipSubscriber<Index + 1, SourcesSize, SubscribeState>::
                    subscribe(cd, observer, state, sources);
            }
        };
        template<size_t SourcesSize, class SubscribeState>
        struct ZipSubscriber<SourcesSize, SourcesSize, SubscribeState> {
            static void subscribe(
                ComposableDisposable& ,
                const std::shared_ptr<Observer<typename SubscribeState::result_type>>& , 
                const std::shared_ptr<SubscribeState>& , 
                const typename SubscribeState::Sources& ) {}
        };
    }
#if RXCPP_USE_VARIADIC_TEMPLATES
    template <class... ZipSource, class S>
    auto Zip(
        S selector,
        const std::shared_ptr<Observable<ZipSource>>&... source
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const ZipSource&...)>::type>>
    {
        typedef typename std::result_of<S(const ZipSource&...)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<ZipSource>>...> Sources;
        typedef std::tuple<std::pair<bool, std::queue<ZipSource>>...> Queues;
        struct State {
            typedef Queues Queues;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : selector(std::move(selector))
                , isStopped(false)
            {}
            std::mutex lock;
            S selector;
            Queues queues;
            bool isStopped;
        };
        Sources sources(source...);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(std::move(selector)));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                cd.Add(Disposable([state](){state->isStopped = true;}));
                detail::ZipSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#else
    template <class ZipSource1, class ZipSource2, class S>
    auto Zip(
        S selector,
        const std::shared_ptr<Observable<ZipSource1>>& source1,
        const std::shared_ptr<Observable<ZipSource2>>& source2
        )
    -> std::shared_ptr<Observable<typename std::result_of<S(const ZipSource1&, const ZipSource2&)>::type>>
    {
        typedef typename std::result_of<S(const ZipSource1&, const ZipSource2&)>::type result_type;
        typedef std::tuple<std::shared_ptr<Observable<ZipSource1>>, std::shared_ptr<Observable<ZipSource2>>> Sources;
        typedef std::tuple<std::pair<bool, std::queue<ZipSource1>>, std::pair<bool, std::queue<ZipSource2>>> Queues;
        struct State {
            typedef Queues Queues;
            typedef Sources Sources;
            typedef result_type result_type;
            typedef std::tuple_size<Sources> SourcesSize;
            explicit State(S selector) 
                : selector(std::move(selector))
                , isStopped(false)
            {}
            std::mutex lock;
            S selector;
            Queues queues;
            bool isStopped;
        };
        Sources sources(source1, source2);
        // bug on osx prevents using make_shared 
        std::shared_ptr<State> state(new State(std::move(selector)));
        return CreateObservable<result_type>(
            [=](std::shared_ptr<Observer<result_type>> observer) -> Disposable
            {
                ComposableDisposable cd;
                detail::ZipSubscriber<0, State::SourcesSize::value, State>::subscribe(cd, observer, state, sources);
                return cd;
            });
    }
#endif //RXCPP_USE_VARIADIC_TEMPLATES
}

#endif