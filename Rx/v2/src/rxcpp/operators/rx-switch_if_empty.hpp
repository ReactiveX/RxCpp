// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SWITCH_IF_EMPTY_HPP)
#define RXCPP_OPERATORS_RX_SWITCH_IF_EMPTY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class BackupSource>
struct switch_if_empty
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<BackupSource> backup_source_type;

    backup_source_type backup;

    switch_if_empty(backup_source_type b)
        : backup(std::move(b))
    {
    }

    template<class Subscriber>
    struct switch_if_empty_observer
    {
        typedef switch_if_empty_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;

        dest_type dest;
        composite_subscription lifetime;
        backup_source_type backup;
        mutable bool is_empty;

        switch_if_empty_observer(dest_type d, composite_subscription cs, backup_source_type b)
            : dest(std::move(d))
            , lifetime(std::move(cs))
            , backup(std::move(b))
            , is_empty(true)
        {
            dest.add(lifetime);
        }
        void on_next(source_value_type v) const {
            is_empty = false;
            dest.on_next(std::move(v));
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(std::move(e));
        }
        void on_completed() const {
            if(!is_empty) {
                dest.on_completed();
            } else {
                backup.subscribe(dest);
            }
        }

        static subscriber<value_type, observer_type> make(dest_type d, backup_source_type b) {
            auto cs = composite_subscription();
            return make_subscriber<value_type>(cs, observer_type(this_type(std::move(d), cs, std::move(b))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(switch_if_empty_observer<Subscriber>::make(std::move(dest), std::move(backup))) {
        return      switch_if_empty_observer<Subscriber>::make(std::move(dest), std::move(backup));
    }
};

template<class BackupSource>
class switch_if_empty_factory
{
    typedef rxu::decay_t<BackupSource> backup_source_type;
    backup_source_type backup;
public:
    switch_if_empty_factory(backup_source_type b) : backup(std::move(b)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(switch_if_empty<rxu::value_type_t<rxu::decay_t<Observable>>, backup_source_type>(backup))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(switch_if_empty<rxu::value_type_t<rxu::decay_t<Observable>>, backup_source_type>(backup));
    }
};

}

template<class BackupSource>
auto switch_if_empty(BackupSource&& b)
    ->      detail::switch_if_empty_factory<BackupSource> {
    return  detail::switch_if_empty_factory<BackupSource>(std::forward<BackupSource>(b));
}

}

}

#endif
