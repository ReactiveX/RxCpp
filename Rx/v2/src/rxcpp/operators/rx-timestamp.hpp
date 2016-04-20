// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TIMESTAMP_HPP)
#define RXCPP_OPERATORS_RX_TIMESTAMP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Coordination>
struct timestamp
{
    static_assert(is_coordination<Coordination>::value, "Coordination parameter must satisfy the requirements for a Coordination");

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    struct timestamp_values {
        timestamp_values(coordination_type c)
            : coordination(c)
        {
        }

        coordination_type coordination;
    };
    timestamp_values initial;

    timestamp(coordination_type coordination)
        : initial(coordination)
    {
    }

    template<class Subscriber>
    struct timestamp_observer
    {
        typedef timestamp_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        coordination_type coord;

        timestamp_observer(dest_type d, coordination_type coordination)
            : dest(std::move(d)),
              coord(std::move(coordination))
        {
        }

        void on_next(source_value_type v) const {
            dest.on_next(std::make_pair(v, coord.now()));
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, timestamp_values v) {
            return make_subscriber<value_type>(d, this_type(d, v.coordination));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(timestamp_observer<Subscriber>::make(std::move(dest), initial)) {
        return      timestamp_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template <class Coordination>
class timestamp_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;
public:
    timestamp_factory(coordination_type ct)
        : coordination(std::move(ct)) { }

    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<std::pair<rxu::value_type_t<rxu::decay_t<Observable>>, typename rxsc::scheduler::clock_type::time_point>>(timestamp<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>(coordination))) {
        return      source.template lift<std::pair<rxu::value_type_t<rxu::decay_t<Observable>>, typename rxsc::scheduler::clock_type::time_point>>(timestamp<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>(coordination));
    }

};

}

template <class Coordination>
inline auto timestamp(Coordination ct)
->      detail::timestamp_factory<Coordination> {
    return  detail::timestamp_factory<Coordination>(std::move(ct));
}

inline auto timestamp()
->      detail::timestamp_factory<identity_one_worker> {
    return  detail::timestamp_factory<identity_one_worker>(identity_current_thread());
}

}

}

#endif
