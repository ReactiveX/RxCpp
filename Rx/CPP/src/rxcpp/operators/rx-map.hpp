// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_MAP_HPP)
#define RXCPP_OPERATORS_RX_MAP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Observable, class Selector>
struct map : public operator_base<decltype((*(Selector*)nullptr)(*(typename Observable::value_type*)nullptr)), typename Observable::value_type>
{
    typedef map<Observable, Selector> this_type;
    Observable source;
    Selector select;

    struct tag_not_valid {};
    template<class CF, class CP>
    static auto check(int) -> decltype((*(CP*)nullptr)(*(CF*)nullptr));
    template<class CF, class CP>
    static tag_not_valid check(...);

    static_assert(!std::is_same<decltype(check<typename this_type::source_value_type, Selector>(0)), tag_not_valid>::value, "map Selector must be a function with the signature map::value_type(map::source_value_type)");

    map(Observable o, Selector s)
        : source(std::move(o))
        , select(std::move(s))
    {
    }
    template<class I>
    void on_subscribe(observer<typename this_type::value_type, I> o) {
        o.add(source.subscribe(
            o.get_subscription(),
        // on_next
            [this, o](typename this_type::source_value_type st) {
                util::detail::maybe<typename this_type::value_type> selected;
                try {
                   selected.reset(this->select(st));
                } catch(...) {
                    o.on_error(std::current_exception());
                }
                if (!selected.empty()) {
                    o.on_next(std::move(*selected));
                }
            },
        // on_error
            [o](std::exception_ptr e) {
                o.on_error(e);
            },
        // on_completed
            [o]() {
                o.on_completed();
            }
        ));
    }
};

template<class Selector>
class map_factory
{
    Selector selector;
public:
    map_factory(Selector p) : selector(std::move(p)) {}
    template<class Observable>
    auto operator()(Observable source)
        ->      observable<typename map<Observable, Selector>::value_type,  map<Observable, Selector>> {
        return  observable<typename map<Observable, Selector>::value_type,  map<Observable, Selector>>(
                                                                            map<Observable, Selector>(source, std::move(selector)));
    }
};

}

template<class Selector>
auto map(Selector p)
    ->      detail::map_factory<Selector> {
    return  detail::map_factory<Selector>(std::move(p));
}

}

}

#endif
