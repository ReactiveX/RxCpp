// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_REDUCE_HPP)
#define RXCPP_OPERATORS_RX_REDUCE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Accumulator, class ResultSelector, class Seed>
struct reduce_traits
{
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Accumulator>::type accumulator_type;
    typedef typename std::decay<ResultSelector>::type result_selector_type;
    typedef typename std::decay<Seed>::type seed_type;

    typedef T source_value_type;

    struct tag_not_valid {};

    template<class CS, class CV, class CRS>
    static auto accumulate_check(int) -> decltype((*(CRS*)nullptr)(*(CS*)nullptr, *(CV*)nullptr));
    template<class CS, class CV, class CRS>
    static tag_not_valid accumulate_check(...);

    static_assert(std::is_same<decltype(accumulate_check<seed_type, source_value_type, accumulator_type>(0)), seed_type>::value, "reduce Accumulator must be a function with the signature Seed(Seed, reduce::source_value_type)");

    template<class CS, class CRS>
    static auto result_check(int) -> decltype((*(CRS*)nullptr)(*(CS*)nullptr));
    template<class CS, class CRS>
    static tag_not_valid result_check(...);

    static_assert(!std::is_same<decltype(result_check<seed_type, result_selector_type>(0)), tag_not_valid>::value, "reduce ResultSelector must be a function with the signature reduce::value_type(Seed)");

    typedef decltype(result_check<seed_type, result_selector_type>(0)) value_type;
};

template<class T, class Observable, class Accumulator, class ResultSelector, class Seed>
struct reduce : public operator_base<typename reduce_traits<T, Observable, Accumulator, ResultSelector, Seed>::value_type>
{
    typedef reduce<T, Observable, Accumulator, ResultSelector, Seed> this_type;
    typedef reduce_traits<T, Observable, Accumulator, ResultSelector, Seed> traits;

    typedef typename traits::source_type source_type;
    typedef typename traits::accumulator_type accumulator_type;
    typedef typename traits::result_selector_type result_selector_type;
    typedef typename traits::seed_type seed_type;

    typedef typename traits::source_value_type source_value_type;
    typedef typename traits::value_type value_type;

    struct reduce_initial_type
    {
        reduce_initial_type(source_type o, accumulator_type a, result_selector_type rs, seed_type s)
            : source(std::move(o))
            , accumulator(std::move(a))
            , result_selector(std::move(rs))
            , seed(std::move(s))
        {
        }
        source_type source;
        accumulator_type accumulator;
        result_selector_type result_selector;
        seed_type seed;
    };
    reduce_initial_type initial;

    reduce(source_type o, accumulator_type a, result_selector_type rs, seed_type s)
        : initial(std::move(o), std::move(a), std::move(rs), std::move(s))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) {
        struct reduce_state_type
            : public reduce_initial_type
            , public std::enable_shared_from_this<reduce_state_type>
        {
            reduce_state_type(reduce_initial_type i, Subscriber scrbr)
                : reduce_initial_type(i)
                , current(reduce_initial_type::seed)
                , out(std::move(scrbr))
            {
            }
            seed_type current;
            Subscriber out;
        };
        auto state = std::make_shared<reduce_state_type>(initial, std::move(o));
        state->source.subscribe(
            state->out,
        // on_next
            [state](T t) {
                auto next = on_exception(
                    [&](){return state->accumulator(state->current, t);},
                    state->out);
                if (next.empty()) {
                    return;
                }
                state->current = next.get();
            },
        // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                auto result = on_exception(
                    [&](){return state->result_selector(state->current);},
                    state->out);
                if (result.empty()) {
                    return;
                }
                state->out.on_next(result.get());
                state->out.on_completed();
            }
        );
    }
};

template<class Accumulator, class ResultSelector, class Seed>
class reduce_factory
{
    typedef typename std::decay<Accumulator>::type accumulator_type;
    typedef typename std::decay<ResultSelector>::type result_selector_type;
    typedef typename std::decay<Seed>::type seed_type;

    accumulator_type accumulator;
    result_selector_type result_selector;
    seed_type seed;
public:
    reduce_factory(accumulator_type a, result_selector_type rs, Seed s)
        : accumulator(std::move(a))
        , result_selector(std::move(rs))
        , seed(std::move(s))
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<seed_type,   reduce<typename std::decay<Observable>::type::value_type, Observable, Accumulator, ResultSelector, Seed>> {
        return  observable<seed_type,   reduce<typename std::decay<Observable>::type::value_type, Observable, Accumulator, ResultSelector, Seed>>(
                                        reduce<typename std::decay<Observable>::type::value_type, Observable, Accumulator, ResultSelector, Seed>(std::forward<Observable>(source), accumulator, result_selector, seed));
    }
};

template<class T>
struct initialize_seeder {
    typedef T seed_type;
    seed_type seed() {
        return seed_type{};
    }
};

template<class T>
struct average {
    struct seed_type
    {
        seed_type()
            : value()
            , count(0)
        {
        }
        T value;
        int count;
        rxu::detail::maybe<double> stage;
    };
    seed_type seed() {
        return seed_type{};
    }
    seed_type operator()(seed_type a, T v) {
        if (a.count != 0 &&
            (a.count == std::numeric_limits<int>::max() ||
            ((v > 0) && (a.value > (std::numeric_limits<T>::max() - v))) ||
            ((v < 0) && (a.value < (std::numeric_limits<T>::min() - v))))) {
            // would overflow, calc existing and reset for next batch
            // this will add error to the final result, but the alternative
            // is to fail on overflow
            double avg = a.value / a.count;
            a.value = v;
            a.count = 1;
            if (!a.stage.empty()) {
                a.stage.reset((*a.stage + avg) / 2);
            } else {
                a.stage.reset(avg);
            }
        } else {
            a.value += v;
            ++a.count;
        }
        return a;
    }
    double operator()(seed_type a) {
        if (a.count > 0) {
            double avg = a.value / a.count;
            if (!a.stage.empty()) {
                avg = (*a.stage + avg) / 2;
            }
            return avg;
        }
        return a.value;
    }
};

}

template<class Seed, class Accumulator, class ResultSelector>
auto reduce(Seed s, Accumulator a, ResultSelector rs)
    ->      detail::reduce_factory<Accumulator, ResultSelector, Seed> {
    return  detail::reduce_factory<Accumulator, ResultSelector, Seed>(std::move(a), std::move(rs), std::move(s));
}

template<template<class X> class Seeder, template<class T> class Accumulator, template<class U> class ResultSelector>
struct specific_reduce
{
    template<class V>
    Accumulator<V> accumulator() const {
        return Accumulator<V>{};
    }
    template<class V>
    ResultSelector<V> result_selector() const {
        return ResultSelector<V>{};
    }
    template<class V>
    auto seed() const
        -> typename Seeder<V>::seed_type {
        return Seeder<V>().seed();
    }
};

static const specific_reduce<detail::initialize_seeder, std::plus, identity_for> sum{};

static const specific_reduce<detail::average, detail::average, detail::average> average{};


}

}

#endif
