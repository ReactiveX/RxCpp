// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_REDUCE_HPP)
#define RXCPP_OPERATORS_RX_REDUCE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

class empty_error: public std::runtime_error
{
    public:
        explicit empty_error(const std::string& msg):
            std::runtime_error(msg)
        {}
};

namespace operators {

namespace detail {

template<class T, class Seed, class Accumulator>
struct is_accumulate_function_for {

    typedef rxu::decay_t<Accumulator> accumulator_type;
    typedef rxu::decay_t<Seed> seed_type;
    typedef T source_value_type;

    struct tag_not_valid {};
    template<class CS, class CV, class CRS>
    static auto check(int) -> decltype((*(CRS*)nullptr)(*(CS*)nullptr, *(CV*)nullptr));
    template<class CS, class CV, class CRS>
    static tag_not_valid check(...);

    typedef decltype(check<seed_type, source_value_type, accumulator_type>(0)) type;
    static const bool value = std::is_same<type, seed_type>::value;
};

template<class Seed, class ResultSelector>
struct is_result_function_for {

    typedef rxu::decay_t<ResultSelector> result_selector_type;
    typedef rxu::decay_t<Seed> seed_type;

    struct tag_not_valid {};

    template<class CS, class CRS>
    static auto check(int) -> decltype((*(CRS*)nullptr)(*(CS*)nullptr));
    template<class CS, class CRS>
    static tag_not_valid check(...);

    typedef decltype(check<seed_type, result_selector_type>(0)) type;
    static const bool value = !std::is_same<type, tag_not_valid>::value;
};

template<class T, class SourceOperator, class Accumulator, class ResultSelector, class Seed>
struct reduce_traits
{
    typedef rxu::decay_t<SourceOperator> source_type;
    typedef rxu::decay_t<Accumulator> accumulator_type;
    typedef rxu::decay_t<ResultSelector> result_selector_type;
    typedef rxu::decay_t<Seed> seed_type;

    typedef T source_value_type;

    static_assert(is_accumulate_function_for<source_value_type, seed_type, accumulator_type>::value, "reduce Accumulator must be a function with the signature Seed(Seed, reduce::source_value_type)");

    static_assert(is_result_function_for<seed_type, result_selector_type>::value, "reduce ResultSelector must be a function with the signature reduce::value_type(Seed)");

    typedef typename is_result_function_for<seed_type, result_selector_type>::type value_type;
};

template<class T, class SourceOperator, class Accumulator, class ResultSelector, class Seed>
struct reduce : public operator_base<rxu::value_type_t<reduce_traits<T, SourceOperator, Accumulator, ResultSelector, Seed>>>
{
    typedef reduce<T, SourceOperator, Accumulator, ResultSelector, Seed> this_type;
    typedef reduce_traits<T, SourceOperator, Accumulator, ResultSelector, Seed> traits;

    typedef typename traits::source_type source_type;
    typedef typename traits::accumulator_type accumulator_type;
    typedef typename traits::result_selector_type result_selector_type;
    typedef typename traits::seed_type seed_type;

    typedef typename traits::source_value_type source_value_type;
    typedef typename traits::value_type value_type;

    struct reduce_initial_type
    {
        ~reduce_initial_type()
        {
        }
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

    private:
        reduce_initial_type& operator=(reduce_initial_type o) RXCPP_DELETE;
    };
    reduce_initial_type initial;

    ~reduce()
    {
    }
    reduce(source_type o, accumulator_type a, result_selector_type rs, seed_type s)
        : initial(std::move(o), std::move(a), std::move(rs), std::move(s))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        struct reduce_state_type
            : public reduce_initial_type
            , public std::enable_shared_from_this<reduce_state_type>
        {
            reduce_state_type(reduce_initial_type i, Subscriber scrbr)
                : reduce_initial_type(i)
                , source(i.source)
                , current(reduce_initial_type::seed)
                , out(std::move(scrbr))
            {
            }
            observable<T, SourceOperator> source;
            seed_type current;
            Subscriber out;

        private:
            reduce_state_type& operator=(reduce_state_type o) RXCPP_DELETE;
        };
        auto state = std::make_shared<reduce_state_type>(initial, std::move(o));
        state->source.subscribe(
            state->out,
        // on_next
            [state](T t) {
                auto next = state->accumulator(state->current, t);
                state->current = next;
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
private:
    reduce& operator=(reduce o) RXCPP_DELETE;
};

template<class Accumulator, class ResultSelector, class Seed>
class reduce_factory
{
    typedef rxu::decay_t<Accumulator> accumulator_type;
    typedef rxu::decay_t<ResultSelector> result_selector_type;
    typedef rxu::decay_t<Seed> seed_type;

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
    auto operator()(const Observable& source)
        ->      observable<seed_type,   reduce<rxu::value_type_t<Observable>, typename Observable::source_operator_type, Accumulator, ResultSelector, Seed>> {
        return  observable<seed_type,   reduce<rxu::value_type_t<Observable>, typename Observable::source_operator_type, Accumulator, ResultSelector, Seed>>(
                                        reduce<rxu::value_type_t<Observable>, typename Observable::source_operator_type, Accumulator, ResultSelector, Seed>(source.source_operator, accumulator, result_selector, seed));
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
        throw rxcpp::empty_error("average() requires a stream with at least one value");
    }
};

template<class T>
struct sum {
    typedef rxu::maybe<T> seed_type;
    seed_type seed() {
        return seed_type();
    }
    seed_type operator()(seed_type a, T v) {
        if (a.empty())
            a.reset(v);
        else
            *a = *a + v;
        return a;
    }
    T operator()(seed_type a) {
        if (a.empty())
            throw rxcpp::empty_error("sum() requires a stream with at least one value");
        return *a;
    }
};

}

template<class Seed, class Accumulator, class ResultSelector>
auto reduce(Seed s, Accumulator a, ResultSelector rs)
    ->      detail::reduce_factory<Accumulator, ResultSelector, Seed> {
    return  detail::reduce_factory<Accumulator, ResultSelector, Seed>(std::move(a), std::move(rs), std::move(s));
}

}

}

#endif
