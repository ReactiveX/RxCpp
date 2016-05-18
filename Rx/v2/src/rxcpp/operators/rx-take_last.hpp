// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_LAST_HPP)
#define RXCPP_OPERATORS_RX_TAKE_LAST_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Count>
struct take_last : public operator_base<T>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Count> count_type;

    typedef std::queue<T> queue_type;
    typedef typename queue_type::size_type queue_size_type;

    struct values
    {
        values(source_type s, count_type t)
            : source(std::move(s))
            , count(static_cast<queue_size_type>(t))
        {
        }
        source_type source;
        queue_size_type count;
    };
    values initial;

    take_last(source_type s, count_type t)
        : initial(std::move(s), std::move(t))
    {
    }

    template<class Subscriber>
    void on_subscribe(const Subscriber& s) const {

        typedef Subscriber output_type;
        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(const values& i, const output_type& oarg)
                : values(i)
                , out(oarg)
            {
            }
            queue_type items;
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::make_shared<state_type>(initial, s);

        composite_subscription source_lifetime;

        s.add(source_lifetime);

        state->source.subscribe(
        // split subscription lifetime
            source_lifetime,
        // on_next
            [state, source_lifetime](T t) {
                if(state->count > 0) {
                    if (state->items.size() == state->count) {
                        state->items.pop();
                    }
                    state->items.push(t);
                }
            },
        // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                while(!state->items.empty()) {
                    state->out.on_next(std::move(state->items.front()));
                    state->items.pop();
                }
                state->out.on_completed();
            }
        );
    }
};

template<class T>
class take_last_factory
{
    typedef rxu::decay_t<T> count_type;
    count_type count;
public:
    take_last_factory(count_type t) : count(std::move(t)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take_last<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take_last<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>>(
                                                                          take_last<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, count_type>(std::forward<Observable>(source), count));
    }
};

}

template<class T>
auto take_last(T&& t)
    ->      detail::take_last_factory<T> {
    return  detail::take_last_factory<T>(std::forward<T>(t));
}

}

}

#endif
