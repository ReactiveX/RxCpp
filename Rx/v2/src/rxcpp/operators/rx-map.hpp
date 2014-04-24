// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_MAP_HPP)
#define RXCPP_OPERATORS_RX_MAP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Observable, class Selector>
struct map
    : public operator_base<decltype((*(Selector*)nullptr)(*(typename Observable::value_type*)nullptr))>
{
    typedef map<Observable, Selector> this_type;

    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Selector>::type select_type;

    struct values
    {
        values(source_type o, select_type s)
            : source(std::move(o))
            , select(std::move(s))
        {
        }
        source_type source;
        select_type select;
    };
    values initial;

    typedef typename Observable::value_type source_value_type;

    struct tag_not_valid {};
    template<class CF, class CP>
    static auto check(int) -> decltype((*(CP*)nullptr)(*(CF*)nullptr));
    template<class CF, class CP>
    static tag_not_valid check(...);

    static_assert(!std::is_same<decltype(check<source_value_type, select_type>(0)), tag_not_valid>::value, "map Selector must be a function with the signature map::value_type(map::source_value_type)");

    map(source_type o, select_type s)
        : initial(std::move(o), std::move(s))
    {
    }

    template<class Subscriber>
    void on_subscribe(const Subscriber& o) {

        typedef Subscriber output_type;
        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(values i, output_type oarg)
                : values(std::move(i))
                , out(std::move(oarg))
            {
            }
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::shared_ptr<state_type>(new state_type(initial, std::move(o)));

        state->source.subscribe(
            state->out,
        // on_next
            [state](typename this_type::source_value_type st) {
                util::detail::maybe<typename this_type::value_type> selected;
                try {
                   selected.reset(state->select(st));
                } catch(...) {
                    state->out.on_error(std::current_exception());
                    return;
                }
                state->out.on_next(std::move(*selected));
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

template<class Selector>
class map_factory
{
    typedef typename std::decay<Selector>::type select_type;
    select_type selector;
public:
    map_factory(select_type p) : selector(std::move(p)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename map<Observable, Selector>::value_type,  map<Observable, Selector>> {
        return  observable<typename map<Observable, Selector>::value_type,  map<Observable, Selector>>(
                                                                            map<Observable, Selector>(std::forward<Observable>(source), std::move(selector)));
    }
};

}

template<class Selector>
auto map(Selector&& p)
    ->      detail::map_factory<Selector> {
    return  detail::map_factory<Selector>(std::forward<Selector>(p));
}

}

}

#endif
