// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-retry.hpp

    \brief Retry this observable for the given number of times.

    \tparam Count the type of the counter (optional)

    \param t  the number of retries (optional) If not specified, infinitely retries the source observable. Specifying returns immediately without subscribing

    \return  An observable that mirrors the source observable, resubscribing to it if it calls on_error up to a specified number of retries.

    \sample
    \snippet retry.cpp retry count sample
    \snippet output.txt retry count sample
*/

#if !defined(RXCPP_OPERATORS_RX_RETRY_HPP)
#define RXCPP_OPERATORS_RX_RETRY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct retry_invalid_arguments {};

template<class... AN>
struct retry_invalid : public rxo::operator_base<retry_invalid_arguments<AN...>> {
    using type = observable<retry_invalid_arguments<AN...>, retry_invalid<AN...>>;
};
template<class... AN>
using retry_invalid_t = typename retry_invalid<AN...>::type;

// Contain retry variations in a namespace
namespace retry {
  // Structure to perform general retry operations on state
  template <class ValuesType, class Subscriber, class T>
  struct state_type : public std::enable_shared_from_this<state_type<ValuesType, Subscriber, T>>,
                      public ValuesType {
    typedef Subscriber output_type;
    state_type(const ValuesType& i, const output_type& oarg)
      : ValuesType(i)
      , source_lifetime(composite_subscription::empty())
      , out(oarg) {
    }

    void do_subscribe() {
      auto state = this->shared_from_this();

      state->out.remove(state->lifetime_token);
      state->source_lifetime.unsubscribe();

      state->source_lifetime = composite_subscription();
      state->lifetime_token = state->out.add(state->source_lifetime);


      state->source.subscribe(
                              state->out,
                              state->source_lifetime,
                              // on_next
                              [state](T t) {
                                state->out.on_next(t);
                              },
                              // on_error
                              [state](std::exception_ptr e) {
                                state->update();
                                // Use specialized predicate for finite/infinte case
                                if (state->completed_predicate()) {
                                  state->out.on_error(e);                                  
                                } else {
                                  state->do_subscribe();
                                }
                              },
                              // on_completed
                              [state]() {
                                // JEP: never appeears to be called?
                                state->out.on_completed();
                              }
                              );
    }
    
    composite_subscription source_lifetime;
    output_type out;
    composite_subscription::weak_subscription lifetime_token;
  };

  // Finite retry case (explicitly limited with the number of times)
  template<class T, class Observable, class Count>
  struct finite : public operator_base<T> {
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Count> count_type;

    struct values {
      values(source_type s, count_type t)
        : source(std::move(s)), remaining_(std::move(t)) {
        if (remaining_ != 0) ++remaining_;
      }
      
      inline bool completed_predicate() const {
        // Return true if we are completed
        return remaining_ <= 0;
      }
      
      inline void update() {
        // Decrement counter
        --remaining_;
      }

      source_type source;

    private:
      count_type remaining_;
    };

    finite(source_type s, count_type t)
      : initial_(std::move(s), std::move(t)) {
    }

    template<class Subscriber>
    void on_subscribe(const Subscriber& s) const {
      typedef state_type<values, Subscriber, T> state_t;
      // take a copy of the values for each subscription
      auto state = std::make_shared<state_t>(initial_, s);
      if (initial_.completed_predicate()) {
        // Should we call it here in retry?
        state->out.on_completed();
      } else {
        // start the first iteration
        state->do_subscribe();
      }
    }

  private:
    values initial_;
  };

  // Infinite retry case
  template<class T, class Observable>
  struct infinite : public operator_base<T> {
    typedef rxu::decay_t<Observable> source_type;

    struct values {
      values(source_type s)
        : source(std::move(s)) {
      }
      
      static inline bool completed_predicate() {
        // Infinite retry never stops ignoring errors
        return false;
      }
      
      static inline void update() {
        // Infinite retry does not need to update state
      }

      source_type source;
    };

    infinite(source_type s) : initial_(std::move(s)) {
    }

    template<class Subscriber>
    void on_subscribe(const Subscriber& s) const {
      typedef state_type<values, Subscriber, T> state_t;
      // take a copy of the values for each subscription
      auto state = std::make_shared<state_t>(initial_, s);
      // start the first iteration
      state->do_subscribe();
    }

  private:
    values initial_;
  };

}
}

/*! @copydoc rx-retry.hpp
*/
template<class... AN>
auto retry(AN&&... an)
->     operator_factory<retry_tag, AN...> {
    return operator_factory<retry_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}    

}

template<> 
struct member_overload<retry_tag>
{
  template<class Observable,
           class Enabled = rxu::enable_if_all_true_type_t<is_observable<Observable>>,
           class SourceValue = rxu::value_type_t<Observable>,
           class Retry = rxo::detail::retry::infinite<SourceValue, rxu::decay_t<Observable>>,
           class Value = rxu::value_type_t<Retry>,
           class Result = observable<Value, Retry>
           >
  static Result member(Observable&& o) {
    return Result(Retry(std::forward<Observable>(o)));
  }

  template<class Observable,
           class Count,
           class Enabled = rxu::enable_if_all_true_type_t<is_observable<Observable>>,
           class SourceValue = rxu::value_type_t<Observable>,
           class Retry = rxo::detail::retry::finite<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<Count>>,
           class Value = rxu::value_type_t<Retry>,
           class Result = observable<Value, Retry>
           >
  static Result member(Observable&& o, Count&& c) {
    return Result(Retry(std::forward<Observable>(o), std::forward<Count>(c)));
  }

  template<class... AN>
  static operators::detail::retry_invalid_t<AN...> member(const AN&...) {
    std::terminate();
    return {};
    static_assert(sizeof...(AN) == 10000, "retry takes (optional Count)");
  } 
};
    
}

#endif
