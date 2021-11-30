// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

namespace rxcpp
{
namespace operators
{
namespace detail
{
template <class Observable, class CollectionSelector, class ResultSelector, class Coordination>
struct flat_map;
}
}  // namespace operators

template<class Observable>
struct observable_member_t<Observable, flat_map_tag>
{
    template<class CollectionSelector,
             class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
             class SourceValue = rxu::value_type_t<Observable>,
             class CollectionType = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
             class ResultSelectorType = rxu::detail::take_at<1>,
             class Enabled = rxu::enable_if_all_true_type_t<
                 all_observables<Observable, CollectionType>>,
             class FlatMap = rxo::detail::flat_map<
                 rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, ResultSelectorType, identity_one_worker>,
             class CollectionValueType = rxu::value_type_t<CollectionType>,
             class Value = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
             class Result = observable<Value, FlatMap>>
    Result flat_map(CollectionSelector&& s)
    {
        return Result(FlatMap(*static_cast<Observable*>(this), std::forward<CollectionSelector>(s), ResultSelectorType(), identity_current_thread()));
    }

    template<class CollectionSelector, class Coordination,
             class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
             class SourceValue = rxu::value_type_t<Observable>,
             class CollectionType = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
             class ResultSelectorType = rxu::detail::take_at<1>,
             class Enabled = rxu::enable_if_all_true_type_t<
                 all_observables<Observable, CollectionType>,
                 is_coordination<Coordination>>,
             class FlatMap = rxo::detail::flat_map<
                 rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, ResultSelectorType, rxu::decay_t<
                     Coordination>>,
             class CollectionValueType = rxu::value_type_t<CollectionType>,
             class Value = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
             class Result = observable<Value, FlatMap>>
    Result flat_map(CollectionSelector&& s, Coordination&& cn)
    {
        return Result(FlatMap(*static_cast<Observable*>(this), std::forward<CollectionSelector>(s), ResultSelectorType(), std::forward<Coordination>(cn)));
    }

    template<class CollectionSelector, class ResultSelector,
             class IsCoordination = is_coordination<ResultSelector>,
             class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
             class SourceValue = rxu::value_type_t<Observable>,
             class CollectionType = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
             class Enabled = rxu::enable_if_all_true_type_t<
                 all_observables<Observable, CollectionType>,
                 rxu::negation<IsCoordination>>,
             class FlatMap = rxo::detail::flat_map<
                 rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, rxu::decay_t<ResultSelector>,
                 identity_one_worker>,
             class CollectionValueType = rxu::value_type_t<CollectionType>,
             class ResultSelectorType = rxu::decay_t<ResultSelector>,
             class Value = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
             class Result = observable<Value, FlatMap>>
    Result flat_map(CollectionSelector&& s, ResultSelector&& rs)
    {
        return Result(FlatMap(*static_cast<Observable*>(this), std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_current_thread()));
    }

    template<class CollectionSelector, class ResultSelector, class Coordination,
             class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
             class SourceValue = rxu::value_type_t<Observable>,
             class CollectionType = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
             class Enabled = rxu::enable_if_all_true_type_t<
                 all_observables<Observable, CollectionType>,
                 is_coordination<Coordination>>,
             class FlatMap = rxo::detail::flat_map<
                 rxu::decay_t<Observable>, rxu::decay_t<CollectionSelector>, rxu::decay_t<ResultSelector>, rxu::decay_t<
                     Coordination>>,
             class CollectionValueType = rxu::value_type_t<CollectionType>,
             class ResultSelectorType = rxu::decay_t<ResultSelector>,
             class Value = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
             class Result = observable<Value, FlatMap>>
    Result flat_map(CollectionSelector&& s, ResultSelector&& rs, Coordination&& cn)
    {
        return Result(FlatMap(*static_cast<Observable*>(this), std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), std::forward<Coordination>(cn)));
    }

    template<typename ...AN>
    auto merge_transform(AN&& ...an)
    {
        return flat_map(std::forward<AN>(an)...);
    }

    template<typename ...AN>
    auto member(AN&& ...an)
    {
        return flat_map(std::forward<AN>(an)...);
    }

};
}
