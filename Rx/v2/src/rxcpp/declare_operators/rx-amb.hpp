// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

namespace rxcpp
{
namespace operators
{
namespace detail
{
template<class T, class Observable, class Coordination>
struct amb;
}
} // namespace operators

template <class T, class SO>
struct observable_member_t<rxcpp::observable<T, SO>, amb_tag>
{
    using Observable  = rxcpp::observable<T, SO>;
    using SourceValue = rxu::decay_t<T>;

    template <class Amb     = rxo::detail::amb<SourceValue, rxu::decay_t<Observable>, identity_one_worker>,
              class Enabled = rxu::enable_if_all_true_type_t<is_observable<SourceValue>, header_included_t<amb_tag, Amb>>,
              class Value   = rxu::value_type_t<Amb>,
              class Result  = observable<Value, Amb>>
    Result amb() const
    {
        return Result(Amb(*static_cast<const Observable*>(this), identity_current_thread()));
    }

    template <class Coordination,
              class Enabled       = rxu::enable_if_all_true_type_t<is_coordination<Coordination>, is_observable<SourceValue>, header_included_t<amb_tag, Coordination>>,
              class Amb           = rxo::detail::amb<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<Coordination>>,
              class Value         = rxu::value_type_t<Amb>,
              class Result        = observable<Value, Amb>>
    Result amb(Coordination&& cn) const
    {
        return Result(Amb(*static_cast<const Observable*>(this), std::forward<Coordination>(cn)));
    }

    template <class Value0,
              class... ValueN,
              class Enabled              = rxu::enable_if_all_true_type_t<all_observables<Value0, ValueN...>, header_included_t<amb_tag, Value0>>,
              class SourceValueAsObs     = observable<SourceValue>,
              class ObservableObservable = observable<SourceValueAsObs>,
              class Amb                  = typename rxu::defer_type<rxo::detail::amb, SourceValueAsObs, ObservableObservable, identity_one_worker>::type,
              class Value                = rxu::value_type_t<Amb>,
              class Result               = observable<Value, Amb>>
    Result amb(Value0&& v0, ValueN&&... vn) const
    {
        return Result(Amb(rxs::from(static_cast<const Observable*>(this)->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread()));
    }

    template <class Coordination,
              class Value0,
              class... ValueN,
              class Enabled              = rxu::enable_if_all_true_type_t<all_observables<Value0, ValueN...>, is_coordination<Coordination>, header_included_t<amb_tag, Coordination>>,
              class SourceValueAsObs     = observable<SourceValue>,
              class ObservableObservable = observable<SourceValueAsObs>,
              class Amb                  = typename rxu::defer_type<rxo::detail::amb, SourceValueAsObs, ObservableObservable, rxu::decay_t<Coordination>>::type,
              class Value                = rxu::value_type_t<Amb>,
              class Result               = observable<Value, Amb>>
    Result amb(Coordination&& cn, Value0&& v0, ValueN&&... vn) const
    {
        return Result(Amb(rxs::from(static_cast<const Observable*>(this)->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::forward<Coordination>(cn)));
    }

    template<typename... AN, typename = std::enable_if_t<!header_included_v<amb_tag, AN...>>>
    static auto amb(AN&&...)
    {
        return operator_declaration<amb_tag, AN...>::header_included();
    }

    template <typename... AN>
    auto member(AN&&...an) const
    {
        return amb(std::forward<AN>(an)...);
    }
};
}
