// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAP_HPP)
#define RXCPP_OPERATORS_RX_TAP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class MakeObserverArgN>
struct tap_observer_factory;

template<class T, class... ArgN>
struct tap_observer_factory<T, std::tuple<ArgN...>>
{
    using source_value_type = rxu::decay_t<T>;
    using out_type = decltype(make_observer<source_value_type, rxcpp::detail::OnErrorIgnore>(*((ArgN*)nullptr)...));
    auto operator()(ArgN&&... an) -> out_type const {
        return make_observer<source_value_type, rxcpp::detail::OnErrorIgnore>(std::forward<ArgN>(an)...);
    }
};

template<class T, class MakeObserverArgN, class Factory = tap_observer_factory<T, MakeObserverArgN>>
struct tap
{
    using source_value_type = rxu::decay_t<T>;
    using args_type = rxu::decay_t<MakeObserverArgN>;
    using factory_type = Factory;
    using out_type = typename factory_type::out_type;
    out_type out;

    tap(args_type a)
        : out(rxu::apply(std::move(a), factory_type()))
    {
    }

    template<class Subscriber>
    struct tap_observer
    {
        using this_type = tap_observer<Subscriber>;
        using value_type = source_value_type;
        using dest_type = rxu::decay_t<Subscriber>;
        using factory_type = Factory;
        using out_type = typename factory_type::out_type;
        using observer_type = observer<value_type, this_type>;
        dest_type dest;
        out_type out;

        tap_observer(dest_type d, out_type o)
            : dest(std::move(d))
            , out(std::move(o))
        {
        }
        void on_next(source_value_type v) const {
            out.on_next(v);
            dest.on_next(v);
        }
        void on_error(std::exception_ptr e) const {
            out.on_error(e);
            dest.on_error(e);
        }
        void on_completed() const {
            out.on_completed();
            dest.on_completed();
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, out_type o) {
            return make_subscriber<value_type>(d, this_type(d, std::move(o)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(tap_observer<Subscriber>::make(std::move(dest), out)) {
        return      tap_observer<Subscriber>::make(std::move(dest), out);
    }
};

template<class MakeObserverArgN>
class tap_factory
{
    typedef rxu::decay_t<MakeObserverArgN> args_type;
    args_type args;
public:
    tap_factory(args_type a) : args(std::move(a)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(tap<rxu::value_type_t<rxu::decay_t<Observable>>, args_type>(args))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(tap<rxu::value_type_t<rxu::decay_t<Observable>>, args_type>(args));
    }
};

}

template<class... MakeObserverArgN>
auto tap(MakeObserverArgN&&... an)
    ->      detail::tap_factory<std::tuple<MakeObserverArgN...>> {
    return  detail::tap_factory<std::tuple<MakeObserverArgN...>>(std::make_tuple(std::forward<MakeObserverArgN>(an)...));
}

}

}

#endif
