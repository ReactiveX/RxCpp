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
              class All   = rxo::detail::all<SourceValue, rxu::decay_t<Predicate>>,
              class Value = rxu::value_type_t<All>>
    auto all(Predicate&& p) const
    {
        return static_cast<const Observable*>(this)->template lift<Value>(All(std::forward<Predicate>(p)));
    }

    template <typename... AN>
    static auto all(AN&&...an)
    {
        return member_overload<all_tag>::include_header_assert(std::forward<AN>(an)...);
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

    template <class IsEmpty = rxo::detail::all<SourceValue, rxu::decay_t<Predicate>>,
              class Value   = rxu::value_type_t<IsEmpty>>
    auto is_empty() const
    {
        return static_cast<const Observable*>(this)->template lift<Value>(IsEmpty([](const SourceValue&) { return false; }));
    }

    template <typename... AN>
    static auto is_empty(AN&&...an)
    {
        return member_overload<is_empty_tag>::include_header_assert(std::forward<AN>(an)...);
    }

    auto member() const
    {
        return is_empty();
    }
};
}  // namespace rxcpp
