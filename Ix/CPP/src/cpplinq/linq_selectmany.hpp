// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#include "util.hpp"
#include "linq_cursor.hpp"

#include <type_traits>

namespace cpplinq
{
    namespace detail
    {
        struct default_select_many_selector
        {
            template <class T1, class T2>
            auto operator()(T1&& t1, T2&& t2) const
                -> decltype(std::forward<T2>(t2))
            {
                return std::forward<T2>(t2);
            }
        };
    }

    namespace detail 
    {
        template <typename Fn, typename Arg>
        struct resolve_select_many_fn_return_type
        {
            typedef decltype(std::declval<Fn>()(std::declval<Arg>())) value;
        };

        template <typename TCol>
        struct value_collection_adapter
        {
            value_collection_adapter(const TCol& col)
                : _collection(col){}

            value_collection_adapter(const value_collection_adapter& src)
                : _collection(src._collection) {}

            value_collection_adapter(value_collection_adapter && src)
                : _collection(std::move(src._collection)) {}

            const TCol& get() const
            {
                return _collection;
            }

            TCol& get()
            {
                return _collection;
            }

        private:
            TCol _collection;
        };

        template<typename TCol>
        struct collection_store_type 
        {
            typedef typename std::remove_reference<TCol>::type                         collection_type;
            typedef std::reference_wrapper<collection_type>                            reference_store_type;
            typedef value_collection_adapter<collection_type>                          value_store_type;

            typedef typename std::conditional<std::is_reference<TCol>::value, reference_store_type, value_store_type>::type    store;
        };
    }

    // cur<T> -> (T -> cur<element_type>) -> cur<element_type>
    template <class Container1, class Fn, class Fn2>
    class linq_select_many
    {
        template <class T> static T instance(); // for type inference

        Container1      c1;
        Fn              fn;
        Fn2             fn2;

        typedef typename Container1::cursor Cur1;
        typedef decltype(from(instance<Fn>()(instance<Cur1>().get()))) Container2;
        typedef typename Container2::cursor Cur2;

        typedef typename detail::resolve_select_many_fn_return_type<Fn, typename Cur1::element_type>::value inner_collection;

    public:
        class cursor
        {
        public:
            typedef typename util::min_cursor_category<typename Cur1::cursor_category,
                                                       typename Cur2::cursor_category,
                                                       forward_cursor_tag>::type
                cursor_category;
            typedef typename Cur2::reference_type reference_type;
            typedef typename Cur2::element_type element_type;

            typedef detail::collection_store_type<inner_collection>     collection_store_type;
            typedef typename collection_store_type::store               collection_store;
            typedef std::shared_ptr<collection_store>                   collection_store_ptr;

        private:
            // TODO: we need to lazy eval somehow, but this feels wrong.
            Cur1                                cur1;
            dynamic_cursor<reference_type>      cur2;
            Fn                                  fn;
            Fn2                                 fn2;
            collection_store_ptr                store;

        public:
            cursor(Cur1 cur1, const Fn& fn, const Fn2& fn2)
                : cur1(std::move(cur1)), fn(fn), fn2(fn2)
            {
                if (!cur1.empty())
                {
                    store = std::make_shared<collection_store>(fn(cur1.get()));
                    cur2 = from(store->get()).get_cursor();
                }
            }

            bool empty() const 
            {
                return cur2.empty();
            }

            void inc() 
            {
                cur2.inc();
                thunk();
            }

            reference_type get() const 
            {
                return fn2(cur1.get(), cur2.get());
            }

        private:
            void thunk() 
            {
                // refill cur2
                while (cur2.empty() && !cur1.empty()) {
                    cur1.inc();
                    if (cur1.empty()) 
                        break;

                    store = std::make_shared<collection_store>(fn(cur1.get()));
                    cur2 = from(store->get()).get_cursor();
                }
            }
        };

        linq_select_many(Container1 c1, Fn fn, Fn2 fn2) 
        : c1(std::move(c1)), fn(std::move(fn)), fn2(std::move(fn2))
        {
        }

        cursor get_cursor() const
        {
            return cursor(c1.get_cursor(), fn, fn2);
        }
    };
}



