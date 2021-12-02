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

template <class T, class SO>
struct observable_member_t<rxcpp::observable<T, SO>, flat_map_tag>
{
    using Observable         = rxcpp::observable<T, SO>;
    using SourceValue        = rxu::decay_t<T>;
    using ResultSelectorType = rxu::detail::take_at<1>;

    template <class CollectionSelector,
              class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
              class CollectionType         = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
              class Enabled                = rxu::enable_if_all_true_type_t<is_observable<CollectionType>>,
              class FlatMap                = rxo::detail::flat_map<rxu::decay_t<Observable>, CollectionSelectorType, ResultSelectorType, identity_one_worker>,
              class CollectionValueType    = rxu::value_type_t<CollectionType>,
              class Value                  = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
              class Result                 = observable<Value, FlatMap>,
              class IsHeaderExist          = typename FlatMap::this_type>
    Result flat_map(CollectionSelector&& s) const
    {
        return Result(FlatMap(*static_cast<const Observable*>(this),
                              std::forward<CollectionSelector>(s),
                              ResultSelectorType(),
                              identity_current_thread()));
    }

    template <class CollectionSelector,
              class Coordination,
              class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
              class CollectionType         = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
              class Enabled                = rxu::enable_if_all_true_type_t<is_observable<CollectionType>, is_coordination<Coordination>>,
              class FlatMap                = rxo::detail::flat_map<rxu::decay_t<Observable>, CollectionSelectorType, ResultSelectorType, rxu::decay_t<Coordination>>,
              class CollectionValueType    = rxu::value_type_t<CollectionType>,
              class Value                  = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
              class Result                 = observable<Value, FlatMap>,
              class IsHeaderExist          = typename FlatMap::this_type>
    Result flat_map(CollectionSelector&& s, Coordination&& cn) const
    {
        return Result(FlatMap(*static_cast<const Observable*>(this),
                              std::forward<CollectionSelector>(s),
                              ResultSelectorType(),
                              std::forward<Coordination>(cn)));
    }

    template <class CollectionSelector,
              class ResultSelector,
              class IsCoordination         = is_coordination<ResultSelector>,
              class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
              class CollectionType         = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
              class Enabled                = rxu::enable_if_all_true_type_t<is_observable<CollectionType>, rxu::negation<IsCoordination>>,
              class FlatMap                = rxo::detail::flat_map<rxu::decay_t<Observable>, CollectionSelectorType, rxu::decay_t<ResultSelector>, identity_one_worker>,
              class CollectionValueType    = rxu::value_type_t<CollectionType>,
              class ResultSelectorType     = rxu::decay_t<ResultSelector>,
              class Value                  = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
              class Result                 = observable<Value, FlatMap>,
              class IsHeaderExist          = typename FlatMap::this_type>
    Result flat_map(CollectionSelector&& s, ResultSelector&& rs) const
    {
        return Result(FlatMap(*static_cast<const Observable*>(this),
                              std::forward<CollectionSelector>(s),
                              std::forward<ResultSelector>(rs),
                              identity_current_thread()));
    }

    template <class CollectionSelector,
              class ResultSelector,
              class Coordination,
              class CollectionSelectorType = rxu::decay_t<CollectionSelector>,
              class CollectionType         = rxu::result_of_t<CollectionSelectorType(SourceValue)>,
              class Enabled                = rxu::enable_if_all_true_type_t<is_observable<CollectionType>, is_coordination<Coordination>>,
              class FlatMap                = rxo::detail::flat_map<rxu::decay_t<Observable>, CollectionSelectorType, rxu::decay_t<ResultSelector>, rxu::decay_t<Coordination>>,
              class CollectionValueType    = rxu::value_type_t<CollectionType>,
              class ResultSelectorType     = rxu::decay_t<ResultSelector>,
              class Value                  = rxu::result_of_t<ResultSelectorType(SourceValue, CollectionValueType)>,
              class Result                 = observable<Value, FlatMap>,
              class IsHeaderExist          = typename FlatMap::this_type>
    Result flat_map(CollectionSelector&& s, ResultSelector&& rs, Coordination&& cn) const
    {
        return Result(FlatMap(*static_cast<const Observable*>(this),
                              std::forward<CollectionSelector>(s),
                              std::forward<ResultSelector>(rs),
                              std::forward<Coordination>(cn)));
    }

    template <typename... AN>
    static auto flat_map(AN&&...an)
    {
        return member_overload<flat_map_tag>::member(std::forward<AN>(an)...);
    }

    template <typename... AN>
    auto merge_transform(AN&&... an) const
    {
        return flat_map(std::forward<AN>(an)...);
    }

    template <typename... AN>
    auto member(AN&&... an) const
    {
        return flat_map(std::forward<AN>(an)...);
    }
};
}  // namespace rxcpp
