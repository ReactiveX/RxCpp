// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_GROUPED_OBSERVABLE_HPP)
#define RXCPP_RX_GROUPED_OBSERVABLE_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace detail {

template<class K, class Source>
struct has_on_get_key_for
{
    struct not_void {};
    template<class CS>
    static auto check(int) -> decltype((*(CS*)nullptr).on_get_key());
    template<class CS>
    static not_void check(...);

    typedef decltype(check<Source>(0)) detail_result;
    static const bool value = std::is_same<detail_result, typename std::decay<K>::type>::value;
};

}

template<class K, class T>
class dynamic_grouped_observable
    : public rxs::source_base<T>
{
public:
    typedef typename std::decay<K>::type key_type;
    typedef tag_dynamic_grouped_observable dynamic_observable_tag;

private:
    struct state_type
        : public std::enable_shared_from_this<state_type>
    {
        typedef std::function<void(subscriber<T>)> onsubscribe_type;
        typedef std::function<key_type()> ongetkey_type;

        onsubscribe_type on_subscribe;
        ongetkey_type on_get_key;
    };
    std::shared_ptr<state_type> state;

    template<class U, class V>
    void construct(const dynamic_grouped_observable<U, V>& o, const tag_dynamic_grouped_observable&) {
        state = o.state;
    }

    template<class U, class V>
    void construct(dynamic_grouped_observable<U, V>&& o, const tag_dynamic_grouped_observable&) {
        state = std::move(o.state);
    }

    template<class SO>
    void construct(SO&& source, const rxs::tag_source&) {
        auto so = std::make_shared<typename std::decay<SO>::type>(std::forward<SO>(source));
        state->on_subscribe = [so](subscriber<T> o) mutable {
            so->on_subscribe(std::move(o));
        };
        state->on_get_key = [so]() mutable {
            return so->on_get_key();
        };
    }

public:

    dynamic_grouped_observable()
    {
    }

    template<class SOF>
    explicit dynamic_grouped_observable(SOF&& sof)
        : state(std::make_shared<state_type>())
    {
        construct(std::forward<SOF>(sof),
                  typename std::conditional<is_dynamic_grouped_observable<SOF>::value, tag_dynamic_grouped_observable, rxs::tag_source>::type());
    }

    template<class SF, class CF>
    dynamic_grouped_observable(SF&& sf, CF&& cf)
        : state(std::make_shared<state_type>())
    {
        state->on_subscribe = std::forward<SF>(sf);
        state->on_connect = std::forward<CF>(cf);
    }

    void on_subscribe(subscriber<T> o) const {
        state->on_subscribe(std::move(o));
    }

    template<class Subscriber>
    typename std::enable_if<!std::is_same<typename std::decay<Subscriber>::type, observer<T>>::value, void>::type
    on_subscribe(Subscriber&& o) const {
        auto so = std::make_shared<typename std::decay<Subscriber>::type>(std::forward<Subscriber>(o));
        state->on_subscribe(
            make_subscriber<T>(
                *so,
            // on_next
                [so](T t){
                    so->on_next(t);
                },
            // on_error
                [so](std::exception_ptr e){
                    so->on_error(e);
                },
            // on_completed
                [so](){
                    so->on_completed();
                }).
            as_dynamic());
    }

    key_type on_get_key() const {
        return state->on_get_key();
    }
};

template<class K, class T, class Source>
grouped_observable<K, T> make_dynamic_grouped_observable(Source&& s) {
    return grouped_observable<K, T>(dynamic_grouped_observable<K, T>(std::forward<Source>(s)));
}



template<class K, class T, class SourceOperator>
class grouped_observable
    : public observable<T, SourceOperator>
{
    typedef grouped_observable<K, T, SourceOperator> this_type;
    typedef observable<T, SourceOperator> base_type;
    typedef typename std::decay<SourceOperator>::type source_operator_type;

    static_assert(detail::has_on_get_key_for<K, source_operator_type>::value, "inner must have on_get_key method key_type()");

public:
    typedef typename std::decay<K>::type key_type;
    typedef tag_grouped_observable observable_tag;

    grouped_observable()
    {
    }

    explicit grouped_observable(const SourceOperator& o)
        : base_type(o)
    {
    }
    explicit grouped_observable(SourceOperator&& o)
        : base_type(std::move(o))
    {
    }

    // implicit conversion between observables of the same value_type
    template<class SO>
    grouped_observable(const grouped_observable<K, T, SO>& o)
        : base_type(o)
    {}
    // implicit conversion between observables of the same value_type
    template<class SO>
    grouped_observable(grouped_observable<K, T, SO>&& o)
        : base_type(std::move(o))
    {}

    ///
    /// performs type-forgetting conversion to a new grouped_observable
    ///
    grouped_observable<K, T> as_dynamic() const {
        return *this;
    }

    key_type get_key() const {
        return base_type::source_operator.on_get_key();
    }
};


}

//
// support range() >> filter() >> subscribe() syntax
// '>>' is spelled 'stream'
//
template<class T, class SourceOperator, class OperatorFactory>
auto operator >> (const rxcpp::grouped_observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}

//
// support range() | filter() | subscribe() syntax
// '|' is spelled 'pipe'
//
template<class T, class SourceOperator, class OperatorFactory>
auto operator | (const rxcpp::grouped_observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}

#endif
