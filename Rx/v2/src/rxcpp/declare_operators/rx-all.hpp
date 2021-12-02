// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

namespace rxcpp
{
namespace operators
{
namespace detail
{
template <class T, class Predicate>
struct all;
}
}  // namespace operators

template <class T, class SO>
struct observable_member_t<rxcpp::observable<T, SO>, all_tag>
{
    using Observable         = rxcpp::observable<T, SO>;
    using SourceValue        = rxu::decay_t<T>;
    
    template <class Predicate,
              class All           = rxo::detail::all<SourceValue, rxu::decay_t<Predicate>>,
              class IsHeaderExist = typename All::source_value_type>
    auto all(Predicate&& p) const
    {
        return static_cast<const Observable*>(this)->template lift<rxu::value_type_t<All>>(All(std::forward<Predicate>(p)));
    }

    template <typename... AN>
    static auto all(AN&&...)
    {
        return all_tag::include_header<std::false_type>{};
    }

    template <typename... AN>
    auto member(AN&&... an) const
    {
        return all(std::forward<AN>(an)...);
    }
};

template <class T, class SO>
struct observable_member_t<rxcpp::observable<T, SO>, is_empty_tag>
{
    using Observable  = rxcpp::observable<T, SO>;
    using SourceValue = rxu::decay_t<T>;
    using Predicate   = std::function<bool(const SourceValue&)>;

    template <class IsEmpty       = rxo::detail::all<SourceValue, rxu::decay_t<Predicate>>,
              class IsHeaderExist = typename IsEmpty::source_value_type>
    auto is_empty() const
    {
        return static_cast<const Observable*>(this)->template lift<rxu::value_type_t<IsEmpty>>(IsEmpty([](const SourceValue&) { return false; }));
    }

    template <typename... AN>
    static auto is_empty(AN&&...)
    {
        return is_empty_tag::include_header<std::false_type>{};
    }

    auto member() const
    {
        return is_empty();
    }
};
}  // namespace rxcpp
