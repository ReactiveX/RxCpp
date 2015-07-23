// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_SELECT_HPP)
#define CPPLINQ_LINQ_SELECT_HPP
#pragma once

#include <cstddef>

namespace cpplinq 
{
    template <class Collection, class Selector>
    class linq_select
    {
        typedef typename Collection::cursor 
            inner_cursor;
    public:
        struct cursor {
            typedef typename util::result_of<Selector(typename inner_cursor::element_type)>::type
                reference_type;
            typedef typename std::remove_reference<reference_type>::type
                element_type;
            typedef typename inner_cursor::cursor_category
                cursor_category;
            
            cursor(const inner_cursor& cur, Selector sel) : cur(cur), sel(std::move(sel)) {}

            void forget() { cur.forget(); }
            bool empty() const { return cur.empty(); }
            void inc() { cur.inc(); }
            reference_type get() const { return sel(cur.get()); }

            bool atbegin() const { return cur.atbegin(); }
            void dec() { cur.dec(); }

            void skip(std::size_t n) { cur.skip(n); }
            std::size_t position() const { return cur.position(); }
            std::size_t size() const { return cur.size(); }
        private:
            inner_cursor    cur;
            Selector        sel;
        };

        linq_select(const Collection& c, Selector sel) : c(c), sel(sel) {}

        cursor get_cursor() const { return cursor(c.get_cursor(), sel); }

    private:
        Collection c;
        Selector sel;
    };

}

#endif // defined(CPPLINQ_LINQ_SELECT_HPP)
