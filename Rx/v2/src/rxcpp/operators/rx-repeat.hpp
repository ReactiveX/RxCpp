// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-repeat.hpp

    \brief Repeat this observable for the given number of times or infinitely.

    \tparam Count  the type of the counter (optional).

    \param t  the number of times the source observable items are repeated (optional). If not specified, infinitely repeats the source observable.

    \return  An observable that repeats the sequence of items emitted by the source observable for t times.

    \sample
    \snippet repeat.cpp repeat count sample
    \snippet output.txt repeat count sample

    If the source observable calls on_error, repeat stops:
    \snippet repeat.cpp repeat error sample
    \snippet output.txt repeat error sample
*/

#if !defined(RXCPP_OPERATORS_RX_REPEAT_HPP)
#define RXCPP_OPERATORS_RX_REPEAT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct repeat_invalid_arguments {};

template<class... AN>
struct repeat_invalid : public rxo::operator_base<repeat_invalid_arguments<AN...>> {
    using type = observable<repeat_invalid_arguments<AN...>, repeat_invalid<AN...>>;
};
template<class... AN>
using repeat_invalid_t = typename repeat_invalid<AN...>::type;

// Contain repeat variations in a namespace
namespace repeat {
  // Structure to perform general repeat operations on state
  template <class ValuesType, class Subscriber, class T>
  struct state_type : public std::enable_shared_from_this<state_type<ValuesType, Subscriber, T>>,
                      public ValuesType {

    typedef Subscriber output_type;
    state_type(const ValuesType& i, const output_type& oarg)
      : ValuesType(i),
        source_lifetime(composite_subscription::empty()),
        out(oarg) {
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
                                // If we are completed already, emit nothing, but
                                // don't terminate yet. Termination will be called only
                                // on on_completed. This behavior may need to be revised
                                // to terminate immideately
                                if (!state->completed_predicate()) {
                                  state->out.on_next(t);
                                }
                              },
                              // on_error
                              [state](std::exception_ptr e) {
                                state->out.on_error(e);
                              },
                              // on_completed
                              [state]() {
                                state->on_completed();
                                // Use specialized predicate for finite/infinte case
                                if (state->completed_predicate()) {
                                  state->out.on_completed();
                                } else {
                                  state->do_subscribe();
                                }
                              }
                              );
    }
          
    composite_subscription source_lifetime;
    output_type out;
    composite_subscription::weak_subscription lifetime_token;
  };

  // Finite repeat case (explicitely limited with the number of times)
  template <class T, class Observable, class Count>
  struct finite : public operator_base<T> {
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<Count> count_type;

    struct values {
      values(source_type s, count_type t)
        : source(std::move(s)),
          remaining_(std::move(t)) {
      }

      inline bool completed_predicate() {
        return remaining_ <= 0;
      }
      
      inline void on_completed() {
        // Decrement counter and check if it's zero
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
      // start the first iteration
      state->do_subscribe();
    }

  private:
    values initial_;
  };

  // Infinite repeat case
  template <class T, class Observable>
  struct infinite : public operator_base<T> {
    typedef rxu::decay_t<Observable> source_type;
    
    struct values {
      values(source_type s)
        : source(std::move(s)) {
      }

      static inline bool completed_predicate() {
        // Infinite repeat never completes
        return false;
      }

      static inline void on_completed() {
        // Infinite repeat never completes
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

/*! @copydoc rx-repeat.hpp
*/
template<class... AN>
auto repeat(AN&&... an)
->     operator_factory<repeat_tag, AN...> {
    return operator_factory<repeat_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<repeat_tag> {
  template<class Observable,
           class Enabled = rxu::enable_if_all_true_type_t<is_observable<Observable>>,
           class SourceValue = rxu::value_type_t<Observable>,
           class Repeat = rxo::detail::repeat::infinite<SourceValue, rxu::decay_t<Observable>>,
           class Value = rxu::value_type_t<Repeat>,
           class Result = observable<Value, Repeat>>
  static Result member(Observable&& o) {
    return Result(Repeat(std::forward<Observable>(o)));
  }

  template<class Observable,
           class Count,
           class Enabled = rxu::enable_if_all_true_type_t<is_observable<Observable>>,
           class SourceValue = rxu::value_type_t<Observable>,
           class Repeat = rxo::detail::repeat::finite<SourceValue, rxu::decay_t<Observable>, rxu::decay_t<Count>>,
           class Value = rxu::value_type_t<Repeat>,
           class Result = observable<Value, Repeat>>
  static Result member(Observable&& o, Count&& c) {
    return Result(Repeat(std::forward<Observable>(o), std::forward<Count>(c)));
  }

  template<class... AN>
  static operators::detail::repeat_invalid_t<AN...> member(AN...) {
    std::terminate();
    return {};
    static_assert(sizeof...(AN) == 10000, "repeat takes (optional Count)");
  }
};

}

#endif
