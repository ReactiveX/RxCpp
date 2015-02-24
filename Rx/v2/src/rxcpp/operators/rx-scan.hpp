// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SCAN_HPP)
#define RXCPP_OPERATORS_RX_SCAN_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Accumulator, class Seed>
struct scan : public operator_base<rxu::decay_t<Seed>>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Accumulator> accumulator_type;
    typedef rxu::decay_t<Seed> seed_type;

    struct scan_initial_type
    {
        scan_initial_type(source_type o, accumulator_type a, seed_type s)
            : source(std::move(o))
            , accumulator(std::move(a))
            , seed(s)
        {
        }
        source_type source;
        accumulator_type accumulator;
        seed_type seed;
    };
    scan_initial_type initial;

    template<class CT, class CS, class CP>
    static auto check(int) -> decltype((*(CP*)nullptr)(*(CS*)nullptr, *(CT*)nullptr));
    template<class CT, class CS, class CP>
    static void check(...);

    scan(source_type o, accumulator_type a, seed_type s)
        : initial(std::move(o), a, s)
    {
        static_assert(std::is_convertible<decltype(check<T, seed_type, accumulator_type>(0)), seed_type>::value, "scan Accumulator must be a function with the signature Seed(Seed, T)");
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        struct scan_state_type
            : public scan_initial_type
            , public std::enable_shared_from_this<scan_state_type>
        {
            scan_state_type(scan_initial_type i, Subscriber scrbr)
                : scan_initial_type(i)
                , result(scan_initial_type::seed)
                , out(std::move(scrbr))
            {
            }
            seed_type result;
            Subscriber out;
        };
        auto state = std::make_shared<scan_state_type>(initial, std::move(o));
        state->source.subscribe(
            state->out,
        // on_next
            [state](T t) {
                state->result = state->accumulator(state->result, t);
                state->out.on_next(state->result);
            },
        // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                state->out.on_completed();
            }
        );
    }
};

template<class Accumulator, class Seed>
class scan_factory
{
    typedef rxu::decay_t<Accumulator> accumulator_type;
    typedef rxu::decay_t<Seed> seed_type;

    accumulator_type accumulator;
    seed_type seed;
public:
    scan_factory(accumulator_type a, Seed s)
        : accumulator(std::move(a))
        , seed(s)
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::decay_t<Seed>, scan<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Accumulator, Seed>> {
        return  observable<rxu::decay_t<Seed>, scan<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Accumulator, Seed>>(
                                               scan<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, Accumulator, Seed>(std::forward<Observable>(source), accumulator, seed));
    }
};

}

template<class Seed, class Accumulator>
auto scan(Seed s, Accumulator&& a)
    ->      detail::scan_factory<Accumulator, Seed> {
    return  detail::scan_factory<Accumulator, Seed>(std::forward<Accumulator>(a), s);
}

}

}

#endif
