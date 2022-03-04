// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_REPLAYSUBJECT_HPP)
#define RXCPP_RX_REPLAYSUBJECT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {

template<class Coordination>
struct replay_traits
{
    using count_type = rxu::maybe<std::size_t>;
    using period_type = rxu::maybe<rxsc::scheduler::clock_type::duration>;
    using time_point_type = rxsc::scheduler::clock_type::time_point;
    using coordination_type = rxu::decay_t<Coordination>;
    using coordinator_type = typename coordination_type::coordinator_type;
};

template<class T, class Coordination>
class replay_observer : public detail::multicast_observer<T>
{
    using this_type = replay_observer<T, Coordination>;
    using base_type = detail::multicast_observer<T>;

    using traits = replay_traits<Coordination>;
    using count_type = typename traits::count_type;
    using period_type = typename traits::period_type;
    using time_point_type = typename traits::time_point_type;
    using coordination_type = typename traits::coordination_type;
    using coordinator_type = typename traits::coordinator_type;

    class replay_observer_state : public std::enable_shared_from_this<replay_observer_state>
    {
        mutable std::mutex lock;
        mutable std::list<T> values;
        mutable std::list<time_point_type> time_points;
        mutable count_type count;
        mutable period_type period;
        mutable composite_subscription replayLifetime;
    public:
        mutable coordination_type coordination;
        mutable coordinator_type coordinator;

    private:
        void remove_oldest() const {
            values.pop_front();
            if (!period.empty()) {
                time_points.pop_front();
            }
        }

    public:
        ~replay_observer_state(){
            replayLifetime.unsubscribe();
        }
        explicit replay_observer_state(count_type _count, period_type _period, coordination_type _coordination, coordinator_type _coordinator, composite_subscription _replayLifetime)
            : count(_count)
            , period(_period)
            , replayLifetime(_replayLifetime)
            , coordination(std::move(_coordination))
            , coordinator(std::move(_coordinator))
        {
        }

        void add(const T& v) const {
            std::unique_lock<std::mutex> guard(lock);

            if (!count.empty()) {
                if (values.size() == count.get())
                    remove_oldest();
            }

            if (!period.empty()) {
                auto now = coordination.now();
                while (!time_points.empty() && (now - time_points.front() > period.get()))
                    remove_oldest();
                time_points.push_back(now);
            }

            values.push_back(v);
        }
        std::list<T> get() const {
            std::unique_lock<std::mutex> guard(lock);
            return values;
        }
    };

    std::shared_ptr<replay_observer_state> state;

public:
    replay_observer(count_type count, period_type period, coordination_type coordination, composite_subscription replayLifetime, composite_subscription subscriberLifetime)
        : base_type(subscriberLifetime)
    {
        replayLifetime.add(subscriberLifetime);
        auto coordinator = coordination.create_coordinator(replayLifetime);
        state = std::make_shared<replay_observer_state>(std::move(count), std::move(period), std::move(coordination), std::move(coordinator), std::move(replayLifetime));
    }

    subscriber<T> get_subscriber() const {
        return make_subscriber<T>(this->get_id(), this->get_subscription(), observer<T, detail::replay_observer<T, Coordination>>(*this)).as_dynamic();
    }

    std::list<T> get_values() const {
        return state->get();
    }

    coordinator_type& get_coordinator() const {
        return state->coordinator;
    }

    void on_next(const T& v) const {
        state->add(v);
        base_type::on_next(v);
    }
};

}

template<class T, class Coordination>
class replay
{
    using traits = detail::replay_traits<Coordination>;
    using count_type = typename traits::count_type;
    using period_type = typename traits::period_type;
    using time_point_type = typename traits::time_point_type;

    detail::replay_observer<T, Coordination> s;

public:
    explicit replay(Coordination cn, composite_subscription cs = composite_subscription())
        : s(count_type(), period_type(), cn, cs, composite_subscription{})
    {
    }

    replay(std::size_t count, Coordination cn, composite_subscription cs = composite_subscription())
        : s(count_type(std::move(count)), period_type(), cn, cs, composite_subscription{})
    {
    }

    replay(rxsc::scheduler::clock_type::duration period, Coordination cn, composite_subscription cs = composite_subscription())
        : s(count_type(), period_type(period), cn, cs, composite_subscription{})
    {
    }

    replay(std::size_t count, rxsc::scheduler::clock_type::duration period, Coordination cn, composite_subscription cs = composite_subscription())
        : s(count_type(count), period_type(period), cn, cs, composite_subscription{})
    {
    }

    bool has_observers() const {
        return s.has_observers();
    }

    std::list<T> get_values() const {
        return s.get_values();
    }

    subscriber<T> get_subscriber() const {
        return s.get_subscriber();
    }

    observable<T> get_observable() const {
        auto keepAlive = s;
        auto observable = make_observable_dynamic<T>([keepAlive](subscriber<T> o){
            for (auto&& value: keepAlive.get_values()) {
                o.on_next(value);
            }
            keepAlive.add(keepAlive.get_subscriber(), std::move(o));
        });
        return s.get_coordinator().in(observable);
    }
};

}

}

#endif
