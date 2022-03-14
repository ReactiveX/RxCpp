// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-group_by.hpp

    \brief Return an observable that emits grouped_observables, each of which corresponds to a unique key value and each of which emits those items from the source observable that share that key value.

    \tparam KeySelector      the type of the key extracting function
    \tparam MarbleSelector   the type of the element extracting function
    \tparam BinaryPredicate  the type of the key comparing function
    \tparam DurationSelector the type of the duration observable function

    \param  ks  a function that extracts the key for each item (optional)
    \param  ms  a function that extracts the return element for each item (optional)
    \param  p   a function that implements comparison of two keys (optional)

    \return  Observable that emits values of grouped_observable type, each of which corresponds to a unique key value and each of which emits those items from the source observable that share that key value.

    \sample
    \snippet group_by.cpp group_by full intro
    \snippet group_by.cpp group_by full sample
    \snippet output.txt group_by full sample

    \sample
    \snippet group_by.cpp group_by sample
    \snippet output.txt group_by sample
*/

#if !defined(RXCPP_OPERATORS_RX_GROUP_BY_HPP)
#define RXCPP_OPERATORS_RX_GROUP_BY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct group_by_invalid_arguments {};

template<class... AN>
struct group_by_invalid : public rxo::operator_base<group_by_invalid_arguments<AN...>> {
    using type = observable<group_by_invalid_arguments<AN...>, group_by_invalid<AN...>>;
};
template<class... AN>
using group_by_invalid_t = typename group_by_invalid<AN...>::type;

template<class T, class Selector>
struct is_group_by_selector_for {

    using selector_type = rxu::decay_t<Selector>;
    using source_value_type = T;

    struct tag_not_valid {};
    template<class CV, class CS>
    static auto check(int) -> decltype(std::declval<CS>()(std::declval<CV>()));
    template<class CV, class CS>
    static tag_not_valid check(...);

    using type = decltype(check<source_value_type, selector_type>(0));
    static const bool value = !rxcpp::is_same_v<type, tag_not_valid>;
};

template<class T, class Observable, class KeySelector, class MarbleSelector, class BinaryPredicate, class DurationSelector>
struct group_by_traits
{
    using source_value_type = T;
    using source_type = rxu::decay_t<Observable>;
    using key_selector_type = rxu::decay_t<KeySelector>;
    using marble_selector_type = rxu::decay_t<MarbleSelector>;
    using predicate_type = rxu::decay_t<BinaryPredicate>;
    using duration_selector_type = rxu::decay_t<DurationSelector>;

    static_assert(is_group_by_selector_for<source_value_type, key_selector_type>::value, "group_by KeySelector must be a function with the signature key_type(source_value_type)");

    using key_type = typename is_group_by_selector_for<source_value_type, key_selector_type>::type;

    static_assert(is_group_by_selector_for<source_value_type, marble_selector_type>::value, "group_by MarbleSelector must be a function with the signature marble_type(source_value_type)");

    using marble_type = typename is_group_by_selector_for<source_value_type, marble_selector_type>::type;

    using subject_type = rxsub::subject<marble_type>;

    using key_subscriber_map_type = std::map<key_type, typename subject_type::subscriber_type, predicate_type>;

    using grouped_observable_type = grouped_observable<key_type, marble_type>;
};

template<class T, class Observable, class KeySelector, class MarbleSelector, class BinaryPredicate, class DurationSelector>
struct group_by
{
    using traits_type = group_by_traits<T, Observable, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>;
    using key_selector_type = typename traits_type::key_selector_type;
    using marble_selector_type = typename traits_type::marble_selector_type;
    using marble_type = typename traits_type::marble_type;
    using predicate_type = typename traits_type::predicate_type;
    using duration_selector_type = typename traits_type::duration_selector_type;
    using subject_type = typename traits_type::subject_type;
    using key_type = typename traits_type::key_type;

    using group_map_type = typename traits_type::key_subscriber_map_type;
    using bindings_type = std::vector<typename composite_subscription::weak_subscription>;

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
        group_by_values(key_selector_type ks, marble_selector_type ms, predicate_type p, duration_selector_type ds)
            : keySelector(std::move(ks))
            , marbleSelector(std::move(ms))
            , predicate(std::move(p))
            , durationSelector(std::move(ds))
        {
        }
        mutable key_selector_type keySelector;
        mutable marble_selector_type marbleSelector;
        mutable predicate_type predicate;
        mutable duration_selector_type durationSelector;
    };

    group_by_values initial;

    group_by(key_selector_type ks, marble_selector_type ms, predicate_type p, duration_selector_type ds)
        : initial(std::move(ks), std::move(ms), std::move(p), std::move(ds))
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
        using this_type = group_by_observer<Subscriber>;
        using value_type = typename traits_type::grouped_observable_type;
        using dest_type = rxu::decay_t<Subscriber>;
        using observer_type = observer<T, this_type>;

        dest_type dest;

        mutable std::shared_ptr<group_by_state_type> state;

        group_by_observer(composite_subscription l, dest_type d, group_by_values v)
            : group_by_values(v)
            , dest(std::move(d))
            , state(std::make_shared<group_by_state_type>(l, group_by_values::predicate))
        {
            group_by::stopsource(dest, state);
        }

        template<typename U>
        void on_next(U&& v) const {
            auto selectedKey = on_exception(
                [&](){
                    return this->keySelector(v);},
                [this](rxu::error_ptr e){on_error(e);});
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
                auto obs = make_dynamic_grouped_observable<key_type, marble_type>(group_by_observable(state, sub, selectedKey.get()));
                auto durationObs = on_exception(
                    [&](){
                        return this->durationSelector(obs);},
                    [this](rxu::error_ptr e){on_error(e);});
                if (durationObs.empty()) {
                    return;
                }

                dest.on_next(obs);
                composite_subscription duration_sub;
                auto ssub = state->source_lifetime.add(duration_sub);

                auto expire_state = state;
                auto expire_dest = g->second;
                auto expire = [=]() {
                    auto g = expire_state->groups.find(selectedKey.get());
                    if (g != expire_state->groups.end()) {
                        expire_state->groups.erase(g);
                        expire_dest.on_completed();
                    }
                    expire_state->source_lifetime.remove(ssub);
                };
                auto robs = durationObs.get().take(1);
                duration_sub.add(robs.subscribe(
                    [](const typename decltype(robs)::value_type &){},
                    [=](rxu::error_ptr) {expire();},
                    [=](){expire();}
                ));
            }
            on_exception_no_return([&]()
                                   {
                                       g->second.on_next(this->marbleSelector(std::forward<U>(v)));
                                   },
                                   [this](rxu::error_ptr e) { on_error(e); });
        }
        void on_error(rxu::error_ptr e) const {
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

template<class KeySelector, class MarbleSelector, class BinaryPredicate, class DurationSelector>
class group_by_factory
{
    using key_selector_type = rxu::decay_t<KeySelector>;
    using marble_selector_type = rxu::decay_t<MarbleSelector>;
    using predicate_type = rxu::decay_t<BinaryPredicate>;
    using duration_selector_type = rxu::decay_t<DurationSelector>;
    key_selector_type keySelector;
    marble_selector_type marbleSelector;
    predicate_type predicate;
    duration_selector_type durationSelector;
public:
    group_by_factory(key_selector_type ks, marble_selector_type ms, predicate_type p, duration_selector_type ds)
        : keySelector(std::move(ks))
        , marbleSelector(std::move(ms))
        , predicate(std::move(p))
        , durationSelector(std::move(ds))
    {
    }
    template<class Observable>
    struct group_by_factory_traits
    {
        using value_type = rxu::value_type_t <rxu::decay_t<Observable>>;
        using traits_type = detail::group_by_traits<value_type, Observable, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>;
        using group_by_type = detail::group_by<value_type, Observable, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>;
    };
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<typename group_by_factory_traits<Observable>::traits_type::grouped_observable_type>(typename group_by_factory_traits<Observable>::group_by_type(std::move(keySelector), std::move(marbleSelector), std::move(predicate), std::move(durationSelector)))) {
        return      source.template lift<typename group_by_factory_traits<Observable>::traits_type::grouped_observable_type>(typename group_by_factory_traits<Observable>::group_by_type(std::move(keySelector), std::move(marbleSelector), std::move(predicate), std::move(durationSelector)));
    }
};

}

/*! @copydoc rx-group_by.hpp
*/
template<class... AN>
auto group_by(AN&&... an) 
    ->     operator_factory<group_by_tag, AN...> {
    return operator_factory<group_by_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<> 
struct member_overload<group_by_tag>
{
    template<class Observable, class KeySelector, class MarbleSelector, class BinaryPredicate, class DurationSelector,
        class SourceValue = rxu::value_type_t<Observable>,
        class Traits = rxo::detail::group_by_traits<SourceValue, rxu::decay_t<Observable>, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>,
        class GroupBy = rxo::detail::group_by<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<KeySelector>, rxu::decay_t<MarbleSelector>, rxu::decay_t<BinaryPredicate>, rxu::decay_t<DurationSelector>>,
        class Value = typename Traits::grouped_observable_type>
    static auto member(Observable&& o, KeySelector&& ks, MarbleSelector&& ms, BinaryPredicate&& p, DurationSelector&& ds)
        -> decltype(o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), std::forward<MarbleSelector>(ms), std::forward<BinaryPredicate>(p), std::forward<DurationSelector>(ds)))) {
        return      o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), std::forward<MarbleSelector>(ms), std::forward<BinaryPredicate>(p), std::forward<DurationSelector>(ds)));
    }

    template<class Observable, class KeySelector, class MarbleSelector, class BinaryPredicate,
        class DurationSelector=rxu::ret<observable<int, rxs::detail::never<int>>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Traits = rxo::detail::group_by_traits<SourceValue, rxu::decay_t<Observable>, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>,
        class GroupBy = rxo::detail::group_by<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<KeySelector>, rxu::decay_t<MarbleSelector>, rxu::decay_t<BinaryPredicate>, rxu::decay_t<DurationSelector>>,
        class Value = typename Traits::grouped_observable_type>
    static auto member(Observable&& o, KeySelector&& ks, MarbleSelector&& ms, BinaryPredicate&& p)
        -> decltype(o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), std::forward<MarbleSelector>(ms), std::forward<BinaryPredicate>(p), rxu::ret<observable<int, rxs::detail::never<int>>>()))) {
        return      o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), std::forward<MarbleSelector>(ms), std::forward<BinaryPredicate>(p), rxu::ret<observable<int, rxs::detail::never<int>>>()));
    }

    template<class Observable, class KeySelector, class MarbleSelector,
        class BinaryPredicate=rxu::less, 
        class DurationSelector=rxu::ret<observable<int, rxs::detail::never<int>>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Traits = rxo::detail::group_by_traits<SourceValue, rxu::decay_t<Observable>, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>,
        class GroupBy = rxo::detail::group_by<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<KeySelector>, rxu::decay_t<MarbleSelector>, rxu::decay_t<BinaryPredicate>, rxu::decay_t<DurationSelector>>,
        class Value = typename Traits::grouped_observable_type>
    static auto member(Observable&& o, KeySelector&& ks, MarbleSelector&& ms)
        -> decltype(o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), std::forward<MarbleSelector>(ms), rxu::less(), rxu::ret<observable<int, rxs::detail::never<int>>>()))) {
        return      o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), std::forward<MarbleSelector>(ms), rxu::less(), rxu::ret<observable<int, rxs::detail::never<int>>>()));
    }


    template<class Observable, class KeySelector, 
        class MarbleSelector=rxu::detail::take_at<0>, 
        class BinaryPredicate=rxu::less, 
        class DurationSelector=rxu::ret<observable<int, rxs::detail::never<int>>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Traits = rxo::detail::group_by_traits<SourceValue, rxu::decay_t<Observable>, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>,
        class GroupBy = rxo::detail::group_by<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<KeySelector>, rxu::decay_t<MarbleSelector>, rxu::decay_t<BinaryPredicate>, rxu::decay_t<DurationSelector>>,
        class Value = typename Traits::grouped_observable_type>
    static auto member(Observable&& o, KeySelector&& ks)
        -> decltype(o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), rxu::detail::take_at<0>(), rxu::less(), rxu::ret<observable<int, rxs::detail::never<int>>>()))) {
        return      o.template lift<Value>(GroupBy(std::forward<KeySelector>(ks), rxu::detail::take_at<0>(), rxu::less(), rxu::ret<observable<int, rxs::detail::never<int>>>()));
    }

    template<class Observable, 
        class KeySelector=rxu::detail::take_at<0>, 
        class MarbleSelector=rxu::detail::take_at<0>, 
        class BinaryPredicate=rxu::less, 
        class DurationSelector=rxu::ret<observable<int, rxs::detail::never<int>>>,
        class Enabled = rxu::enable_if_all_true_type_t<
            all_observables<Observable>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Traits = rxo::detail::group_by_traits<SourceValue, rxu::decay_t<Observable>, KeySelector, MarbleSelector, BinaryPredicate, DurationSelector>,
        class GroupBy = rxo::detail::group_by<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<KeySelector>, rxu::decay_t<MarbleSelector>, rxu::decay_t<BinaryPredicate>, rxu::decay_t<DurationSelector>>,
        class Value = typename Traits::grouped_observable_type>
    static auto member(Observable&& o)
        -> decltype(o.template lift<Value>(GroupBy(rxu::detail::take_at<0>(), rxu::detail::take_at<0>(), rxu::less(), rxu::ret<observable<int, rxs::detail::never<int>>>()))) {
        return      o.template lift<Value>(GroupBy(rxu::detail::take_at<0>(), rxu::detail::take_at<0>(), rxu::less(), rxu::ret<observable<int, rxs::detail::never<int>>>()));
    }

    template<class... AN>
    static operators::detail::group_by_invalid_t<AN...> member(const AN&...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "group_by takes (optional KeySelector, optional MarbleSelector, optional BinaryKeyPredicate, optional DurationSelector), KeySelector takes (Observable::value_type) -> KeyValue, MarbleSelector takes (Observable::value_type) -> MarbleValue, BinaryKeyPredicate takes (KeyValue, KeyValue) -> bool, DurationSelector takes (Observable::value_type) -> Observable");
    }

};

}

#endif

