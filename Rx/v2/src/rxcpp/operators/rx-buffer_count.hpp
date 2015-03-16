// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_BUFFER_COUNT_HPP)
#define RXCPP_OPERATORS_RX_BUFFER_COUNT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {


template<class T>
struct buffer_count
{
    typedef rxu::decay_t<T> source_value_type;
    struct buffer_count_values
    {
        buffer_count_values(int c, int s)
            : count(c)
            , skip(s)
        {
        }
        int count;
        int skip;
    };

    buffer_count_values initial;

    buffer_count(int count, int skip)
        : initial(count, skip)
    {
    }

    template<class Subscriber>
    struct buffer_count_observer : public buffer_count_values
    {
        typedef buffer_count_observer<Subscriber> this_type;
        typedef std::vector<T> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        mutable int cursor;
        mutable std::deque<value_type> chunks;

        buffer_count_observer(dest_type d, buffer_count_values v)
            : buffer_count_values(v)
            , dest(std::move(d))
            , cursor(0)
        {
        }
        void on_next(T v) const {
            if (cursor++ % this->skip == 0) {
                chunks.emplace_back();
            }
            for(auto& chunk : chunks) {
                chunk.push_back(v);
            }
            while (!chunks.empty() && int(chunks.front().size()) == this->count) {
                dest.on_next(std::move(chunks.front()));
                chunks.pop_front();
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            auto done = on_exception(
                [&](){
                    while (!chunks.empty()) {
                        dest.on_next(std::move(chunks.front()));
                        chunks.pop_front();
                    }
                    return true;
                },
                dest);
            if (done.empty()) {
                return;
            }
            dest.on_completed();
        }

        static subscriber<T, observer<T, this_type>> make(dest_type d, buffer_count_values v) {
            auto cs = d.get_subscription();
            return make_subscriber<T>(std::move(cs), this_type(std::move(d), std::move(v)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(buffer_count_observer<Subscriber>::make(std::move(dest), initial)) {
        return      buffer_count_observer<Subscriber>::make(std::move(dest), initial);
    }
};

class buffer_count_factory
{
    int count;
    int skip;
public:
    buffer_count_factory(int c, int s) : count(c), skip(s) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<std::vector<rxu::value_type_t<rxu::decay_t<Observable>>>>(buffer_count<rxu::value_type_t<rxu::decay_t<Observable>>>(count, skip))) {
        return      source.template lift<std::vector<rxu::value_type_t<rxu::decay_t<Observable>>>>(buffer_count<rxu::value_type_t<rxu::decay_t<Observable>>>(count, skip));
    }
};

}

inline auto buffer(int count)
    ->      detail::buffer_count_factory {
    return  detail::buffer_count_factory(count, count);
}
inline auto buffer(int count, int skip)
    ->      detail::buffer_count_factory {
    return  detail::buffer_count_factory(count, skip);
}

}

}

#endif
