// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-reduce.hpp

    \brief For each item from this observable use Accumulator to combine items, when completed use ResultSelector to produce a value that will be emitted from the new observable that is returned.

    \tparam Seed            the type of the initial value for the accumulator
    \tparam Accumulator     the type of the data accumulating function
    \tparam ResultSelector  the type of the result producing function

    \param seed  the initial value for the accumulator
    \param a     an accumulator function to be invoked on each item emitted by the source observable, the result of which will be used in the next accumulator call
    \param rs    a result producing function that makes the final value from the last accumulator call result

    \return  An observable that emits a single item that is the result of accumulating the output from the items emitted by the source observable.

    Some basic reduce-type operators have already been implemented:
    - rxcpp::operators::first
    - rxcpp::operators::last
    - rxcpp::operators::count
    - rxcpp::operators::sum
    - rxcpp::operators::average
    - rxcpp::operators::min
    - rxcpp::operators::max

    \sample
    Geometric mean of source values:
    \snippet reduce.cpp reduce sample
    \snippet output.txt reduce sample

    If the source observable completes without emitting any items, the resulting observable emits the result of passing the initial seed to the result selector:
    \snippet reduce.cpp reduce empty sample
    \snippet output.txt reduce empty sample

    If the accumulator raises an exception, it is returned by the resulting observable in on_error:
    \snippet reduce.cpp reduce exception from accumulator sample
    \snippet output.txt reduce exception from accumulator sample

    The same for exceptions raised by the result selector:
    \snippet reduce.cpp reduce exception from result selector sample
    \snippet output.txt reduce exception from result selector sample
*/

#if !defined(RXCPP_OPERATORS_RX_REDUCE_HPP)
#define RXCPP_OPERATORS_RX_REDUCE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct reduce_invalid_arguments {};

template<class... AN>
struct reduce_invalid : public rxo::operator_base<reduce_invalid_arguments<AN...>> {
    using type = observable<reduce_invalid_arguments<AN...>, reduce_invalid<AN...>>;
};
template<class... AN>
using reduce_invalid_t = typename reduce_invalid<AN...>::type;

template<class Seed, class ResultSelector>
struct is_result_function_for {

    using result_selector_type = rxu::decay_t<ResultSelector>;
    using seed_type = rxu::decay_t<Seed>;

    struct tag_not_valid {};

    template<class CS, class CRS>
    static auto check(int) -> decltype(std::declval<CRS>()(std::declval<CS>()));
    template<class CS, class CRS>
    static tag_not_valid check(...);

    using type = rxu::decay_t<decltype(check<seed_type, result_selector_type>(0))>;
    static const bool value = !std::is_same_v<type, tag_not_valid>;
};

template<class T, class Observable, class Accumulator, class ResultSelector, class Seed>
struct reduce_traits
{
    using source_type = rxu::decay_t<Observable>;
    using accumulator_type = rxu::decay_t<Accumulator>;
    using result_selector_type = rxu::decay_t<ResultSelector>;
    using seed_type = rxu::decay_t<Seed>;

    using source_value_type = T;

    using value_type = typename is_result_function_for<seed_type, result_selector_type>::type;
};

template<class T, class Observable, class Accumulator, class ResultSelector, class Seed>
struct reduce : public operator_base<rxu::value_type_t<reduce_traits<T, Observable, Accumulator, ResultSelector, Seed>>>
{
    using this_type = reduce<T, Observable, Accumulator, ResultSelector, Seed>;
    using traits = reduce_traits<T, Observable, Accumulator, ResultSelector, Seed>;

    using source_type = typename traits::source_type;
    using accumulator_type = typename traits::accumulator_type;
    using result_selector_type = typename traits::result_selector_type;
    using seed_type = typename traits::seed_type;

    using source_value_type = typename traits::source_value_type;
    using value_type = typename traits::value_type;

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
            source_type source;
            seed_type current;
            Subscriber out;

        private:
            reduce_state_type& operator=(reduce_state_type o) RXCPP_DELETE;
        };
        auto state = std::make_shared<reduce_state_type>(initial, std::move(o));
        state->source.subscribe(
            state->out,
        // on_next
            [state](auto&& t) {
                state->current = state->accumulator(std::move(state->current), std::forward<decltype(t)>(t));
            },
        // on_error
            [state](rxu::error_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                auto result = on_exception(
                    [&](){return state->result_selector(std::move(state->current));},
                    state->out);
                if (result.empty()) {
                    return;
                }
                state->out.on_next(std::move(result.get()));
                state->out.on_completed();
            }
        );
    }
private:
    reduce& operator=(reduce o) RXCPP_DELETE;
};

template<class T>
struct initialize_seeder {
    using seed_type = T;
    static seed_type seed() {
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
        rxu::maybe<T> value;
        int count;
        rxu::detail::maybe<double> stage;
    };
    static seed_type seed() {
        return seed_type{};
    }
    template<class U>
    seed_type operator()(seed_type a, U&& v) {
        if (a.count != 0 &&
            (a.count == std::numeric_limits<int>::max() ||
            ((v > 0) && (*(a.value) > (std::numeric_limits<T>::max() - v))) ||
            ((v < 0) && (*(a.value) < (std::numeric_limits<T>::min() - v))))) {
            // would overflow, calc existing and reset for next batch
            // this will add error to the final result, but the alternative
            // is to fail on overflow
            double avg = static_cast<double>(*(a.value)) / a.count;
            if (!a.stage.empty()) {
                a.stage.reset((*a.stage + avg) / 2);
            } else {
                a.stage.reset(avg);
            }
            a.value.reset(std::forward<U>(v));
            a.count = 1;
        } else if (a.value.empty()) {
            a.value.reset(std::forward<U>(v));
            a.count = 1;
        } else {
            *(a.value) += v;
            ++a.count;
        }
        return a;
    }
    double operator()(seed_type a) {
        if (!a.value.empty()) {
            double avg = static_cast<double>(*(a.value)) / a.count;
            if (!a.stage.empty()) {
                avg = (*a.stage + avg) / 2;
            }
            return avg;
        }
        rxu::throw_exception(rxcpp::empty_error("average() requires a stream with at least one value"));
    }
};

template<class T>
struct sum {
    using seed_type = rxu::maybe<T>;
    static seed_type seed() {
        return seed_type();
    }
    template<class U>
    seed_type operator()(seed_type a, U&& v) const {
        if (a.empty())
            a.reset(std::forward<U>(v));
        else
            *a = *a + v;
        return a;
    }
    T operator()(seed_type a) const {
        if (a.empty())
            rxu::throw_exception(rxcpp::empty_error("sum() requires a stream with at least one value"));
        return *a;
    }
};

template<class T>
struct max {
    using seed_type = rxu::maybe<T>;
    static seed_type seed() {
        return seed_type();
    }
    template<class U>
    seed_type operator()(seed_type a, U&& v) {
        if (a.empty() || *a < v)
            a.reset(std::forward<U>(v));
        return a;
    }
    T operator()(seed_type a) {
        if (a.empty())
            rxu::throw_exception(rxcpp::empty_error("max() requires a stream with at least one value"));
        return *a;
    }
};

template<class T>
struct min {
    using seed_type = rxu::maybe<T>;
    static seed_type seed() {
        return seed_type();
    }
    template<class U>
    seed_type operator()(seed_type a, U&& v) {
        if (a.empty() || v < *a)
            a.reset(std::forward<U>(v));
        return a;
    }
    T operator()(seed_type a) {
        if (a.empty())
            rxu::throw_exception(rxcpp::empty_error("min() requires a stream with at least one value"));
        return *a;
    }
};

template<class T>
struct first {
    using seed_type = rxu::maybe<T>;
    static seed_type seed() {
        return seed_type();
    }
    template<class U>
    seed_type operator()(seed_type a, U&& v) {
        a.reset(std::forward<U>(v));
        return a;
    }
    T operator()(seed_type a) {
        if (a.empty()) {
            rxu::throw_exception(rxcpp::empty_error("first() requires a stream with at least one value"));
        }
        return *a;
    }
};

template<class T>
struct last {
    using seed_type = rxu::maybe<T>;
    static seed_type seed() {
        return seed_type();
    }
    template<class U>
    seed_type operator()(seed_type a, U&& v) {
        a.reset(std::forward<U>(v));
        return a;
    }
    T operator()(seed_type a) {
        if (a.empty()) {
            rxu::throw_exception(rxcpp::empty_error("last() requires a stream with at least one value"));
        }
        return *a;
    }
};

}

/*! @copydoc rx-reduce.hpp
*/
template<class... AN>
auto reduce(AN&&... an)
    ->     operator_factory<reduce_tag, AN...> {
    return operator_factory<reduce_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

/*! @copydoc rx-reduce.hpp
*/
template<class... AN>
auto accumulate(AN&&... an)
    ->     operator_factory<reduce_tag, AN...> {
    return operator_factory<reduce_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

/*! \brief For each item from this observable reduce it by sending only the first item.

    \return  An observable that emits only the very first item emitted by the source observable.

    \sample
    \snippet math.cpp first sample
    \snippet output.txt first sample

    When the source observable calls on_error:
    \snippet math.cpp first empty sample
    \snippet output.txt first empty sample
*/
inline auto first()
    ->     operator_factory<first_tag> {
    return operator_factory<first_tag>(std::tuple<>{});
}

/*! \brief For each item from this observable reduce it by sending only the last item.

    \return  An observable that emits only the very last item emitted by the source observable.

    \sample
    \snippet math.cpp last sample
    \snippet output.txt last sample

    When the source observable calls on_error:
    \snippet math.cpp last empty sample
    \snippet output.txt last empty sample
*/
inline auto last()
    ->     operator_factory<last_tag> {
    return operator_factory<last_tag>(std::tuple<>{});
}

/*! \brief For each item from this observable reduce it by incrementing a count.

    \return  An observable that emits a single item: the number of elements emitted by the source observable.

    \sample
    \snippet math.cpp count sample
    \snippet output.txt count sample

    When the source observable calls on_error:
    \snippet math.cpp count error sample
    \snippet output.txt count error sample
*/
inline auto count()
    ->     operator_factory<reduce_tag, int, rxu::count, rxu::detail::take_at<0>> {
    return operator_factory<reduce_tag, int, rxu::count, rxu::detail::take_at<0>>(std::make_tuple(0, rxu::count(), rxu::take_at<0>()));
}

/*! \brief For each item from this observable reduce it by adding to the previous values and then dividing by the number of items at the end.

    \return  An observable that emits a single item: the average of elements emitted by the source observable.

    \sample
    \snippet math.cpp average sample
    \snippet output.txt average sample

    When the source observable completes without emitting any items:
    \snippet math.cpp average empty sample
    \snippet output.txt average empty sample

    When the source observable calls on_error:
    \snippet math.cpp average error sample
    \snippet output.txt average error sample
*/
inline auto average()
    ->     operator_factory<average_tag> {
    return operator_factory<average_tag>(std::tuple<>{});
}

/*! \brief For each item from this observable reduce it by adding to the previous items.

    \return  An observable that emits a single item: the sum of elements emitted by the source observable.

    \sample
    \snippet math.cpp sum sample
    \snippet output.txt sum sample

    When the source observable completes without emitting any items:
    \snippet math.cpp sum empty sample
    \snippet output.txt sum empty sample

    When the source observable calls on_error:
    \snippet math.cpp sum error sample
    \snippet output.txt sum error sample
*/
inline auto sum()
    ->     operator_factory<sum_tag> {
    return operator_factory<sum_tag>(std::tuple<>{});
}

/*! \brief For each item from this observable reduce it by taking the min value of the previous items.

    \return  An observable that emits a single item: the min of elements emitted by the source observable.

    \sample
    \snippet math.cpp min sample
    \snippet output.txt min sample

    When the source observable completes without emitting any items:
    \snippet math.cpp min empty sample
    \snippet output.txt min empty sample

    When the source observable calls on_error:
    \snippet math.cpp min error sample
    \snippet output.txt min error sample
*/
inline auto min()
    ->     operator_factory<min_tag> {
    return operator_factory<min_tag>(std::tuple<>{});
}

/*! \brief For each item from this observable reduce it by taking the max value of the previous items.

    \return  An observable that emits a single item: the max of elements emitted by the source observable.

    \sample
    \snippet math.cpp max sample
    \snippet output.txt max sample

    When the source observable completes without emitting any items:
    \snippet math.cpp max empty sample
    \snippet output.txt max empty sample

    When the source observable calls on_error:
    \snippet math.cpp max error sample
    \snippet output.txt max error sample
*/
inline auto max()
    ->     operator_factory<max_tag> {
    return operator_factory<max_tag>(std::tuple<>{});
}

}

template<>
struct member_overload<reduce_tag>
{

    template<class Observable, class Seed, class Accumulator, class ResultSelector,
        class Reduce = rxo::detail::reduce<rxu::value_type_t<Observable>, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class Value = rxu::value_type_t<Reduce>,
        class Result = observable<Value, Reduce>>
    static Result member(Observable&& o, Seed&& s, Accumulator&& a, ResultSelector&& r)
    {
        return Result(Reduce(std::forward<Observable>(o), std::forward<Accumulator>(a), std::forward<ResultSelector>(r), std::forward<Seed>(s)));
    }

    template<class Observable, class Seed, class Accumulator,
        class ResultSelector=rxu::detail::take_at<0>,
        class Reduce = rxo::detail::reduce<rxu::value_type_t<Observable>, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class Value = rxu::value_type_t<Reduce>,
        class Result = observable<Value, Reduce>>
    static Result member(Observable&& o, Seed&& s, Accumulator&& a)
    {
        return Result(Reduce(std::forward<Observable>(o), std::forward<Accumulator>(a), rxu::detail::take_at<0>(), std::forward<Seed>(s)));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "reduce takes (Seed, Accumulator, optional ResultSelector), Accumulator takes (Seed, Observable::value_type) -> Seed, ResultSelector takes (Observable::value_type) -> ResultValue");
    }
};

template<>
struct member_overload<first_tag>
{
    template<class Observable,
        class SValue = rxu::value_type_t<Observable>,
        class Operation = operators::detail::first<SValue>,
        class Seed = decltype(Operation::seed()),
        class Accumulator = Operation,
        class ResultSelector = Operation,
        class TakeOne = decltype(std::declval<rxu::decay_t<Observable>>().take(1)),
        class Reduce = rxo::detail::reduce<SValue, rxu::decay_t<TakeOne>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class RValue = rxu::value_type_t<Reduce>,
        class Result = observable<RValue, Reduce>>
    static Result member(Observable&& o)
    {
        return Result(Reduce(o.take(1), Operation{}, Operation{}, Operation::seed()));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "first does not support Observable::value_type");
    }
};

template<>
struct member_overload<last_tag>
{
    template<class Observable,
        class SValue = rxu::value_type_t<Observable>,
        class Operation = operators::detail::last<SValue>,
        class Seed = decltype(Operation::seed()),
        class Accumulator = Operation,
        class ResultSelector = Operation,
        class Reduce = rxo::detail::reduce<SValue, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class RValue = rxu::value_type_t<Reduce>,
        class Result = observable<RValue, Reduce>>
    static Result member(Observable&& o)
    {
        return Result(Reduce(std::forward<Observable>(o), Operation{}, Operation{}, Operation::seed()));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "last does not support Observable::value_type");
    }
};

template<>
struct member_overload<sum_tag>
{
    template<class Observable,
        class SValue = rxu::value_type_t<Observable>,
        class Operation = operators::detail::sum<SValue>,
        class Seed = decltype(Operation::seed()),
        class Accumulator = Operation,
        class ResultSelector = Operation,
        class Reduce = rxo::detail::reduce<SValue, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class RValue = rxu::value_type_t<Reduce>,
        class Result = observable<RValue, Reduce>>
    static Result member(Observable&& o)
    {
        return Result(Reduce(std::forward<Observable>(o), Operation{}, Operation{}, Operation::seed()));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "sum does not support Observable::value_type");
    }
};

template<>
struct member_overload<average_tag>
{
    template<class Observable,
        class SValue = rxu::value_type_t<Observable>,
        class Operation = operators::detail::average<SValue>,
        class Seed = decltype(Operation::seed()),
        class Accumulator = Operation,
        class ResultSelector = Operation,
        class Reduce = rxo::detail::reduce<SValue, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class RValue = rxu::value_type_t<Reduce>,
        class Result = observable<RValue, Reduce>>
    static Result member(Observable&& o)
    {
        return Result(Reduce(std::forward<Observable>(o), Operation{}, Operation{}, Operation::seed()));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "average does not support Observable::value_type");
    }
};

template<>
struct member_overload<max_tag>
{
    template<class Observable,
        class SValue = rxu::value_type_t<Observable>,
        class Operation = operators::detail::max<SValue>,
        class Seed = decltype(Operation::seed()),
        class Accumulator = Operation,
        class ResultSelector = Operation,
        class Reduce = rxo::detail::reduce<SValue, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class RValue = rxu::value_type_t<Reduce>,
        class Result = observable<RValue, Reduce>>
    static Result member(Observable&& o)
    {
        return Result(Reduce(std::forward<Observable>(o), Operation{}, Operation{}, Operation::seed()));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "max does not support Observable::value_type");
    }
};

template<>
struct member_overload<min_tag>
{
    template<class Observable,
        class SValue = rxu::value_type_t<Observable>,
        class Operation = operators::detail::min<SValue>,
        class Seed = decltype(Operation::seed()),
        class Accumulator = Operation,
        class ResultSelector = Operation,
        class Reduce = rxo::detail::reduce<SValue, rxu::decay_t<Observable>, rxu::decay_t<Accumulator>, rxu::decay_t<ResultSelector>, rxu::decay_t<Seed>>,
        class RValue = rxu::value_type_t<Reduce>,
        class Result = observable<RValue, Reduce>>
    static Result member(Observable&& o)
    {
        return Result(Reduce(std::forward<Observable>(o), Operation{}, Operation{}, Operation::seed()));
    }

    template<class... AN>
    static operators::detail::reduce_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "min does not support Observable::value_type");
    }
};

}

#endif
