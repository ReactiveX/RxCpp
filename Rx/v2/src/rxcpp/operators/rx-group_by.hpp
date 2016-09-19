// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_GROUP_BY_HPP)
#define RXCPP_OPERATORS_RX_GROUP_BY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Selector>
struct is_group_by_selector_for {

    typedef rxu::decay_t<Selector> selector_type;
    typedef T source_value_type;

    struct tag_not_valid {};
    template<class CV, class CS>
    static auto check(int) -> decltype((*(CS*)nullptr)(*(CV*)nullptr));
    template<class CV, class CS>
    static tag_not_valid check(...);

    typedef decltype(check<source_value_type, selector_type>(0)) type;
    static const bool value = !std::is_same<type, tag_not_valid>::value;
};

template<class T, class Observable, class KeySelector, class MarbleSelector, class BinaryPredicate>
struct group_by_traits
{
    typedef T source_value_type;
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<KeySelector> key_selector_type;
    typedef rxu::decay_t<MarbleSelector> marble_selector_type;
    typedef rxu::decay_t<BinaryPredicate> predicate_type;

    static_assert(is_group_by_selector_for<source_value_type, key_selector_type>::value, "group_by KeySelector must be a function with the signature key_type(source_value_type)");

    typedef typename is_group_by_selector_for<source_value_type, key_selector_type>::type key_type;

    static_assert(is_group_by_selector_for<source_value_type, marble_selector_type>::value, "group_by MarbleSelector must be a function with the signature marble_type(source_value_type)");

    typedef typename is_group_by_selector_for<source_value_type, marble_selector_type>::type marble_type;

    typedef rxsub::subject<marble_type> subject_type;

    typedef std::map<key_type, typename subject_type::subscriber_type, predicate_type> key_subscriber_map_type;

    typedef grouped_observable<key_type, marble_type> grouped_observable_type;
};

template<class T, class Observable, class KeySelector, class MarbleSelector, class BinaryPredicate>
struct group_by
{
    typedef group_by_traits<T, Observable, KeySelector, MarbleSelector, BinaryPredicate> traits_type;
    typedef typename traits_type::key_selector_type key_selector_type;
    typedef typename traits_type::marble_selector_type marble_selector_type;
    typedef typename traits_type::marble_type marble_type;
    typedef typename traits_type::predicate_type predicate_type;
    typedef typename traits_type::subject_type subject_type;
    typedef typename traits_type::key_type key_type;

    typedef typename traits_type::key_subscriber_map_type group_map_type;
    typedef std::vector<typename composite_subscription::weak_subscription> bindings_type;

    struct group_by_state_type 
    {
        group_by_state_type(composite_subscription sl, predicate_type p) 
            : source_lifetime(sl)
            , groups(p)
            , observers(0) 
        {}
        composite_subscription source_lifetime;
        rxsc::worker worker;
        group_map_type groups;
        std::atomic<int> observers;
    };

    template<class Subscriber>
    static void stopsource(Subscriber&& dest, std::shared_ptr<group_by_state_type>& state) {
        ++state->observers;
        dest.add([state](){
            if (!state->source_lifetime.is_subscribed()) {
                return;
            }
            --state->observers;
            if (state->observers == 0) {
                state->source_lifetime.unsubscribe();
            }
        });
    }

    struct group_by_values
    {
        group_by_values(key_selector_type ks, marble_selector_type ms, predicate_type p)
            : keySelector(std::move(ks))
            , marbleSelector(std::move(ms))
            , predicate(std::move(p))
        {
        }
        mutable key_selector_type keySelector;
        mutable marble_selector_type marbleSelector;
        mutable predicate_type predicate;
    };

    group_by_values initial;

    group_by(key_selector_type ks, marble_selector_type ms, predicate_type p)
        : initial(std::move(ks), std::move(ms), std::move(p))
    {
    }

    struct group_by_observable : public rxs::source_base<marble_type>
    {
        mutable std::shared_ptr<group_by_state_type> state;
        subject_type subject;
        key_type key;

        group_by_observable(std::shared_ptr<group_by_state_type> st, subject_type s, key_type k)
            : state(std::move(st))
            , subject(std::move(s))
            , key(k)
        {
        }

        template<class Subscriber>
        void on_subscribe(Subscriber&& o) const {
            group_by::stopsource(o, state);
            subject.get_observable().subscribe(std::forward<Subscriber>(o));
        }

        key_type on_get_key() {
            return key;
        }
    };

    template<class Subscriber>
    struct group_by_observer : public group_by_values
    {
        typedef group_by_observer<Subscriber> this_type;
        typedef typename traits_type::grouped_observable_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;

        dest_type dest;

        mutable std::shared_ptr<group_by_state_type> state;

        group_by_observer(composite_subscription l, dest_type d, group_by_values v)
            : group_by_values(v)
            , dest(std::move(d))
            , state(std::make_shared<group_by_state_type>(l, group_by_values::predicate))
        {
            group_by::stopsource(dest, state);
        }
        void on_next(T v) const {
            auto selectedKey = on_exception(
                [&](){
                    return this->keySelector(v);},
                [this](std::exception_ptr e){on_error(e);});
            if (selectedKey.empty()) {
                return;
            }
            auto g = state->groups.find(selectedKey.get());
            if (g == state->groups.end()) {
                if (!dest.is_subscribed()) {
                    return;
                }
                auto sub = subject_type();
                g = state->groups.insert(std::make_pair(selectedKey.get(), sub.get_subscriber())).first;
                dest.on_next(make_dynamic_grouped_observable<key_type, marble_type>(group_by_observable(state, sub, selectedKey.get())));
            }
            auto selectedMarble = on_exception(
                [&](){
                    return this->marbleSelector(v);},
                [this](std::exception_ptr e){on_error(e);});
            if (selectedMarble.empty()) {
                return;
            }
            g->second.on_next(std::move(selectedMarble.get()));
        }
        void on_error(std::exception_ptr e) const {
            for(auto& g : state->groups) {
                g.second.on_error(e);
            }
            dest.on_error(e);
        }
        void on_completed() const {
            for(auto& g : state->groups) {
                g.second.on_completed();
            }
            dest.on_completed();
        }

        static subscriber<T, observer_type> make(dest_type d, group_by_values v) {
            auto cs = composite_subscription();
            return make_subscriber<T>(cs, observer_type(this_type(cs, std::move(d), std::move(v))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(group_by_observer<Subscriber>::make(std::move(dest), initial)) {
        return      group_by_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class KeySelector, class MarbleSelector, class BinaryPredicate>
class group_by_factory
{
    typedef rxu::decay_t<KeySelector> key_selector_type;
    typedef rxu::decay_t<MarbleSelector> marble_selector_type;
    typedef rxu::decay_t<BinaryPredicate> predicate_type;
    key_selector_type keySelector;
    marble_selector_type marbleSelector;
    predicate_type predicate;
public:
    group_by_factory(key_selector_type ks, marble_selector_type ms, predicate_type p)
        : keySelector(std::move(ks))
        , marbleSelector(std::move(ms))
        , predicate(std::move(p))
    {
    }
    template<class Observable>
    struct group_by_factory_traits
    {
        typedef rxu::value_type_t<rxu::decay_t<Observable>> value_type;
        typedef detail::group_by_traits<value_type, Observable, KeySelector, MarbleSelector, BinaryPredicate> traits_type;
        typedef detail::group_by<value_type, Observable, KeySelector, MarbleSelector, BinaryPredicate> group_by_type;
    };
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<typename group_by_factory_traits<Observable>::traits_type::grouped_observable_type>(typename group_by_factory_traits<Observable>::group_by_type(std::move(keySelector), std::move(marbleSelector), std::move(predicate)))) {
        return      source.template lift<typename group_by_factory_traits<Observable>::traits_type::grouped_observable_type>(typename group_by_factory_traits<Observable>::group_by_type(std::move(keySelector), std::move(marbleSelector), std::move(predicate)));
    }
};

}

template<class KeySelector, class MarbleSelector, class BinaryPredicate>
inline auto group_by(KeySelector ks, MarbleSelector ms, BinaryPredicate p)
    ->      detail::group_by_factory<KeySelector, MarbleSelector, BinaryPredicate> {
    return  detail::group_by_factory<KeySelector, MarbleSelector, BinaryPredicate>(std::move(ks), std::move(ms), std::move(p));
}

template<class KeySelector, class MarbleSelector>
inline auto group_by(KeySelector ks, MarbleSelector ms)
    ->      detail::group_by_factory<KeySelector, MarbleSelector, rxu::less> {
    return  detail::group_by_factory<KeySelector, MarbleSelector, rxu::less>(std::move(ks), std::move(ms), rxu::less(), identity_current_thread());
}

template<class KeySelector>
inline auto group_by(KeySelector ks)
    ->      detail::group_by_factory<KeySelector, rxu::detail::take_at<0>, rxu::less> {
    return  detail::group_by_factory<KeySelector, rxu::detail::take_at<0>, rxu::less>(std::move(ks), rxu::take_at<0>(), rxu::less());
}

}

}

#endif

