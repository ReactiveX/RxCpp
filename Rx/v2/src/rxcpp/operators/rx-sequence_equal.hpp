// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SEQUENCE_EQUAL_HPP)
#define RXCPP_OPERATORS_RX_SEQUENCE_EQUAL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class OtherObservable, class BinaryPredicate, class Coordination>
struct sequence_equal : public operator_base<bool>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<OtherObservable> other_source_type;
    typedef typename other_source_type::value_type other_source_value_type;
    typedef rxu::decay_t<BinaryPredicate> predicate_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

    struct values {
        values(source_type s, other_source_type t, predicate_type pred, coordination_type sf)
                : source(std::move(s))
                , other(std::move(t))
                , pred(std::move(pred))
                , coordination(std::move(sf))
        {
        }

        source_type source;
        other_source_type other;
        predicate_type pred;
        coordination_type coordination;
    };

    values initial;

    sequence_equal(source_type s, other_source_type t, predicate_type pred, coordination_type sf)
        : initial(std::move(s), std::move(t), std::move(pred), std::move(sf))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber s) const {

        typedef Subscriber output_type;

        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(const values& vals, coordinator_type coor, const output_type& o)
                : values(vals)
                , coordinator(std::move(coor))
                , out(o)
                , source_completed(false)
                , other_completed(false)
            {
                out.add(other_lifetime);
                out.add(source_lifetime);
            }

            composite_subscription other_lifetime;
            composite_subscription source_lifetime;
            coordinator_type coordinator;
            output_type out;
            
            mutable std::list<source_value_type> source_values;
            mutable std::list<other_source_value_type> other_values;
            mutable bool source_completed;
            mutable bool other_completed;
        };

        auto coordinator = initial.coordination.create_coordinator();
        auto state = std::make_shared<state_type>(initial, std::move(coordinator), std::move(s));

        auto other = on_exception(
            [&](){ return state->coordinator.in(state->other); },
            state->out);
        if (other.empty()) {
            return;
        }

        auto source = on_exception(
            [&](){ return state->coordinator.in(state->source); },
            state->out);
        if (source.empty()) {
            return;
        }

        auto check_equal = [state]() {
            if(!state->source_values.empty() && !state->other_values.empty()) {
                auto x = std::move(state->source_values.front());
                state->source_values.pop_front();

                auto y = std::move(state->other_values.front());
                state->other_values.pop_front();

                if (!state->pred(x, y)) {
                    state->out.on_next(false);
                    state->out.on_completed();
                }
            } else {
                if((!state->source_values.empty() && state->other_completed) ||
                   (!state->other_values.empty() && state->source_completed)) {
                    state->out.on_next(false);
                    state->out.on_completed();
                }
            }
        };
        
        auto check_complete = [state]() {
            if(state->source_completed && state->other_completed) {
                state->out.on_next(state->source_values.empty() && state->other_values.empty());
                state->out.on_completed();
            }
        };

        auto sinkOther = make_subscriber<other_source_value_type>(
            state->out,
            state->other_lifetime,
            // on_next
            [state, check_equal](other_source_value_type t) {
                auto& values = state->other_values;
                values.push_back(t);
                check_equal();
            },
            // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
            },
            // on_completed
            [state, check_complete]() {
                auto& completed = state->other_completed;
                completed = true;
                check_complete();
            }
        );

        auto selectedSinkOther = on_exception(
            [&](){ return state->coordinator.out(sinkOther); },
            state->out);
        if (selectedSinkOther.empty()) {
            return;
        }
        other->subscribe(std::move(selectedSinkOther.get()));

        source.get().subscribe(
            state->source_lifetime,
            // on_next
            [state, check_equal](source_value_type t) {
                auto& values = state->source_values;
                values.push_back(t);
                check_equal();
            },
            // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
            },
            // on_completed
            [state, check_complete]() {
                auto& completed = state->source_completed;
                completed = true;
                check_complete();
            }
        );        
    }
};

template<class OtherObservable, class BinaryPredicate, class Coordination>
class sequence_equal_factory
{
    typedef rxu::decay_t<OtherObservable> other_source_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef rxu::decay_t<BinaryPredicate> predicate_type;
    
    other_source_type other_source;
    coordination_type coordination;
    predicate_type pred;
    
public:
    sequence_equal_factory(other_source_type t, predicate_type p, coordination_type sf)
        : other_source(std::move(t))
        , coordination(std::move(sf))
        , pred(std::move(p))
    {
    }
    
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<bool, sequence_equal<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, other_source_type, BinaryPredicate, Coordination>> {
        return  observable<bool, sequence_equal<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, other_source_type, BinaryPredicate, Coordination>>(
                                 sequence_equal<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, other_source_type, BinaryPredicate, Coordination>(std::forward<Observable>(source), other_source, pred, coordination));
    }
};

}

template<class OtherObservable>
inline auto sequence_equal(OtherObservable&& t)
    ->      detail::sequence_equal_factory<OtherObservable, rxu::equal_to<>, identity_one_worker> {
    return  detail::sequence_equal_factory<OtherObservable, rxu::equal_to<>, identity_one_worker>(std::forward<OtherObservable>(t), rxu::equal_to<>(), identity_current_thread());
}

template<class OtherObservable, class BinaryPredicate, class Check = typename std::enable_if<!is_coordination<BinaryPredicate>::value>::type>
inline auto sequence_equal(OtherObservable&& t, BinaryPredicate&& pred)
    ->      detail::sequence_equal_factory<OtherObservable, BinaryPredicate, identity_one_worker> {
    return  detail::sequence_equal_factory<OtherObservable, BinaryPredicate, identity_one_worker>(std::forward<OtherObservable>(t), std::forward<BinaryPredicate>(pred), identity_current_thread());
}

template<class OtherObservable, class Coordination, class Check = typename std::enable_if<is_coordination<Coordination>::value>::type>
inline auto sequence_equal(OtherObservable&& t, Coordination&& cn)
    ->      detail::sequence_equal_factory<OtherObservable, rxu::equal_to<>, Coordination> {
    return  detail::sequence_equal_factory<OtherObservable, rxu::equal_to<>, Coordination>(std::forward<OtherObservable>(t), rxu::equal_to<>(), std::forward<Coordination>(cn));
}

template<class OtherObservable, class BinaryPredicate, class Coordination>
inline auto sequence_equal(OtherObservable&& t, BinaryPredicate&& pred, Coordination&& cn)
    ->      detail::sequence_equal_factory<OtherObservable, BinaryPredicate, Coordination> {
    return  detail::sequence_equal_factory<OtherObservable, BinaryPredicate, Coordination>(std::forward<OtherObservable>(t), std::forward<BinaryPredicate>(pred), std::forward<Coordination>(cn));
}

}

}

#endif
