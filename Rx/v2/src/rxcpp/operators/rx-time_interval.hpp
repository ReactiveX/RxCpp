// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TIME_INTERVAL_HPP)
#define RXCPP_OPERATORS_RX_TIME_INTERVAL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Coordination>
struct time_interval
{
    static_assert(is_coordination<Coordination>::value, "Coordination parameter must satisfy the requirements for a Coordination");

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    struct time_interval_values {
        time_interval_values(coordination_type c)
            : coordination(c)
        {
        }

        coordination_type coordination;
    };
    time_interval_values initial;

    time_interval(coordination_type coordination)
        : initial(coordination)
    {
    }

    template<class Subscriber>
    struct time_interval_observer
    {
        typedef time_interval_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        typedef rxsc::scheduler::clock_type::time_point time_point;
        dest_type dest;
        coordination_type coord;
        mutable time_point last;

        time_interval_observer(dest_type d, coordination_type coordination)
            : dest(std::move(d)),
              coord(std::move(coordination)),
              last(coord.now())
        {
        }

        void on_next(source_value_type) const {
            time_point now = coord.now();
            dest.on_next(now - last);
            last = now;
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, time_interval_values v) {
            return make_subscriber<value_type>(d, this_type(d, v.coordination));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(time_interval_observer<Subscriber>::make(std::move(dest), initial)) {
        return      time_interval_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template <class Coordination>
class time_interval_factory
{
    typedef rxu::decay_t<Coordination> coordination_type;

    coordination_type coordination;
public:
    time_interval_factory(coordination_type ct)
        : coordination(std::move(ct)) { }

    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<typename rxsc::scheduler::clock_type::time_point::duration>(time_interval<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>(coordination))) {
        return      source.template lift<typename rxsc::scheduler::clock_type::time_point::duration>(time_interval<rxu::value_type_t<rxu::decay_t<Observable>>, Coordination>(coordination));
    }

};

}

template <class Coordination>
inline auto time_interval(Coordination ct)
->      detail::time_interval_factory<Coordination> {
    return  detail::time_interval_factory<Coordination>(std::move(ct));
}

inline auto time_interval()
->      detail::time_interval_factory<identity_one_worker> {
    return  detail::time_interval_factory<identity_one_worker>(identity_current_thread());
}

}

}

#endif
