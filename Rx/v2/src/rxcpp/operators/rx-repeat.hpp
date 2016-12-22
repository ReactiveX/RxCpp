// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-repeat.hpp

    \brief Repeat this observable for the given number of times or infinitely.

    \tparam Count  the type of the counter (optional).

    \param t  the number of times the source observable items are repeated (optional). If not specified or 0, infinitely repeats the source observable.

    \return  An observable that repeats the sequence of items emitted by the source observable for t times.

    \sample
    \snippet repeat.cpp repeat count sample
    \snippet output.txt repeat count sample

    If the source observable calls on_error, repeat stops:
    \snippet repeat.cpp repeat error sample
    \snippet output.txt repeat error sample
*/

#if !defined(RXCPP_OPERATORS_RX_REPEAT_HPP)
#define RXCPP_OPERATORS_RX_REPEAT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct repeat_invalid_arguments {};

template<class... AN>
struct repeat_invalid : public rxo::operator_base<repeat_invalid_arguments<AN...>> {
    using type = observable<repeat_invalid_arguments<AN...>, repeat_invalid<AN...>>;
};
template<class... AN>
using repeat_invalid_t = typename repeat_invalid<AN...>::type;

template<class T, class Observable, class Count>
struct repeat : public operator_base<T>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Count> count_type;
    struct values
    {
        values(source_type s, count_type t)
            : source(std::move(s))
            , remaining(std::move(t))
            , repeat_infinitely(t == 0)
        {
        }
        source_type source;
        count_type remaining;
        bool repeat_infinitely;
    };
    values initial;

    repeat(source_type s, count_type t)
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
                , source_lifetime(composite_subscription::empty())
                , out(oarg)
            {
            }
            composite_subscription source_lifetime;
            output_type out;
            composite_subscription::weak_subscription lifetime_token;

            void do_subscribe() {
                auto state = this->shared_from_this();
                
                state->out.remove(state->lifetime_token);
                state->source_lifetime.unsubscribe();

                state->source_lifetime = composite_subscription();
                state->lifetime_token = state->out.add(state->source_lifetime);

                state->source.subscribe(
                    state->out,
                    state->source_lifetime,
                // on_next
                    [state](T t) {
                        state->out.on_next(t);
                    },
                // on_error
                    [state](std::exception_ptr e) {
                        state->out.on_error(e);
                    },
                // on_completed
                    [state]() {
                        if (state->repeat_infinitely || (--state->remaining > 0)) {
                            state->do_subscribe();
                        } else {
                            state->out.on_completed();
                        }
                    }
                );
            }
        };

        // take a copy of the values for each subscription
        auto state = std::make_shared<state_type>(initial, s);

        // start the first iteration
        state->do_subscribe();
    }
};

}

/*! @copydoc rx-repeat.hpp
*/
template<class... AN>
auto repeat(AN&&... an)
->     operator_factory<repeat_tag, AN...> {
    return operator_factory<repeat_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<repeat_tag>
{
    template<class Observable,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Repeat = rxo::detail::repeat<SourceValue, rxu::decay_t<Observable>, int>,
        class Value = rxu::value_type_t<Repeat>,
        class Result = observable<Value, Repeat>>
    static Result member(Observable&& o) {
        return Result(Repeat(std::forward<Observable>(o), 0));
    }

    template<class Observable,
        class Count,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Repeat = rxo::detail::repeat<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<Count>>,
        class Value = rxu::value_type_t<Repeat>,
        class Result = observable<Value, Repeat>>
    static Result member(Observable&& o, Count&& c) {
        return Result(Repeat(std::forward<Observable>(o), std::forward<Count>(c)));
    }

    template<class... AN>
    static operators::detail::repeat_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "repeat takes (optional Count)");
    }
};

}

#endif
