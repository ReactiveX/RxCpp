// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_WHERE_HPP)
#define CPPLINQ_LINQ_WHERE_HPP
#pragma once

namespace cpplinq 
{
    template <class Collection, class Predicate>
    class linq_where
    {
        typedef typename Collection::cursor 
            inner_cursor;
    public:
        struct cursor {
            typedef typename util::min_iterator_category<
                    bidirectional_cursor_tag,
                    typename inner_cursor::cursor_category>::type
                cursor_category;
            typedef typename inner_cursor::element_type
                element_type;
            typedef typename inner_cursor::reference_type
                reference_type;

            cursor(const inner_cursor& cur, const Predicate& p) : cur(cur), pred(p)
            {
                if (!cur.empty() && !pred(cur.get())) {
                    this->inc();
                }
            }

            void forget() { cur.forget(); }
            bool empty() const { return cur.empty(); }
            void inc() { 
                for (;;) {
                    cur.inc();
                    if (cur.empty() || pred(cur.get())) break;
                }
            }
            reference_type get() const { 
                return cur.get();
            }

            bool atbegin() const { return atbegin(cur); }
            void dec() {
                for (;;) {
                    cur.dec();
                    if (pred(cur.get())) break;
                }
            }
        private:
            inner_cursor cur;
            Predicate pred;
        };

        linq_where(const Collection& c, Predicate pred) : c(c), pred(pred) {}

        cursor get_cursor() const { 
            return cursor(c.get_cursor(), pred); 
        }

    private:
        Collection c;
        Predicate   pred;
    };
}

#endif // !defined(CPPLINQ_LINQ_WHERE_HPP)

