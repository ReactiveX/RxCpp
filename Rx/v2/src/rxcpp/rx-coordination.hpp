// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_COORDINATION_HPP)
#define RXCPP_RX_COORDINATION_HPP

#include "rx-includes.hpp"

namespace rxcpp {

struct tag_coordinator {};
struct coordinator_base {typedef tag_coordinator coordinator_tag;};
template<class T>
class is_coordinator
{
    template<class C>
    static typename C::coordinator_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_coordinator*>::value;
};

struct tag_coordination {};
struct coordination_base {typedef tag_coordination coordination_tag;};
template<class T>
class is_coordination
{
    template<class C>
    static typename C::coordination_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_coordination*>::value;
};

template<class Input, class Output>
class coordinator : public coordinator_base
{
    struct not_supported {typedef not_supported type;};
public:
    typedef Input input_type;
    typedef Output output_type;

    input_type input;
    output_type output;

    template<class Observable>
    struct get_observable
    {
        typedef decltype((*(input_type*)nullptr)((*(Observable*)nullptr))) type;
    };

    template<class Subscriber>
    struct get_subscriber
    {
        typedef decltype((*(output_type*)nullptr)((*(Subscriber*)nullptr))) type;
    };

    template<class T>
    struct get
    {
        typedef typename std::conditional<
            is_observable<T>::value, get_observable<T>, typename std::conditional<
            is_subscriber<T>::value, get_subscriber<T>, not_supported>::type>::type::type type;
    };

    coordinator(Input i, Output o) : input(i), output(o) {}

    const Input& get_input() const {
        return input;
    }

    const Output& get_output() const {
        return output;
    }

    template<class Observable>
    auto in(Observable o) const
        -> typename get_observable<Observable>::type {
        return input(std::move(o));
        static_assert(is_observable<Observable>::value, "can only synchronize observables");
    }

    template<class Subscriber>
    auto out(Subscriber s) const
        -> typename get_subscriber<Subscriber>::type {
        return output(std::move(s));
        static_assert(is_subscriber<Subscriber>::value, "can only synchronize subscribers");
    }
};

class identity_one_worker : public coordination_base
{
    rxsc::scheduler factory;

    class input_type
    {
        rxsc::worker controller;
        rxsc::scheduler factory;
    public:
        explicit input_type(rxsc::worker w)
            : controller(w)
            , factory(rxsc::make_same_worker(w))
        {
        }
        rxsc::worker get_worker() const {
            return controller;
        }
        rxsc::scheduler get_scheduler() const {
            return factory;
        }
        template<class Observable>
        Observable operator()(Observable o) const {
            return std::move(o);
            static_assert(is_observable<Observable>::value, "can only synchronize observables");
        }
    };

    class output_type
    {
        rxsc::worker controller;
        rxsc::scheduler factory;
    public:
        explicit output_type(rxsc::worker w)
            : controller(w)
            , factory(rxsc::make_same_worker(w))
        {
        }
        rxsc::worker get_worker() const {
            return controller;
        }
        rxsc::scheduler get_scheduler() const {
            return factory;
        }
        template<class Subscriber>
        Subscriber operator()(Subscriber s) const {
            return std::move(s);
            static_assert(is_subscriber<Subscriber>::value, "can only synchronize subscribers");
        }
    };

public:

    explicit identity_one_worker(rxsc::scheduler sc) : factory(sc) {}

    typedef coordinator<input_type, output_type> coordinator_type;

    coordinator_type create_coordinator(composite_subscription cs = composite_subscription()) const {
        auto w = factory.create_worker(std::move(cs));
        return coordinator_type(input_type(w), output_type(w));
    }
};

}

#endif
