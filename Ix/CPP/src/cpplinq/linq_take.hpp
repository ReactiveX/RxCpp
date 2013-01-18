// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_TAKE_HPP)
#define CPPLINQ_LINQ_TAKE_HPP
#pragma once

namespace cpplinq 
{
    template <class InnerCursor>
    struct linq_take_cursor
    {
        typedef typename InnerCursor::element_type element_type;
        typedef typename InnerCursor::reference_type reference_type;
        typedef typename InnerCursor::cursor_category cursor_category;

        linq_take_cursor(const InnerCursor& cur, size_t rem) : cur(cur), rem(rem) {}

        void forget() { cur.forget(); }
        bool empty() const { return cur.empty() || rem == 0; }
        void inc() { cur.inc(); --rem; }
        reference_type get() const { return cur.get(); }

        bool atbegin() const { return cur.atbegin(); }
        void dec() { cur.dec(); --rem; }

        void skip(size_t n) { cur.skip(n); rem -= n; }
        size_t position() const { return cur.position(); }
        size_t size() const { return cur.size(); }
            
    private:
        InnerCursor cur;
        size_t      rem;
    };

    namespace detail {
        template <class Collection>
        linq_take_cursor<typename Collection::cursor> 
            take_get_cursor_(
                const Collection& c,
                size_t n,
                onepass_cursor_tag
                )
        {
            return linq_take_cursor<typename Collection::cursor>(c.get_cursor(), n);
        }

        template <class Collection>
        typename Collection::cursor
            take_get_cursor_(
                const Collection& c,
                size_t n,
                random_access_cursor_tag
                )
        {
            auto cur = c.get_cursor();
            if (cur.size() > n) {
                cur.truncate(n);
            }
            return std::move(cur);
        }
    }

    template <class Collection>
    struct linq_take
    {
        typedef typename std::conditional<
                util::less_or_equal_cursor_category<
                    random_access_cursor_tag,
                    typename Collection::cursor::cursor_category>::value,
                typename Collection::cursor,
                linq_take_cursor<typename Collection::cursor>>::type
            cursor;

        linq_take(const Collection& c, size_t n) : c(c), n(n) {}

        cursor get_cursor() const {
            return detail::take_get_cursor_(c, n, typename Collection::cursor::cursor_category());
        }

        Collection  c;
        size_t      n;
    };

    template <class Collection>
    auto get_cursor(
            const linq_take<Collection>& take
            )
    -> decltype(get_cursor_(take, typename Collection::cursor::cursor_category()))
    {
        return get_cursor_(take, typename Collection::cursor::cursor_category());
    }


}
#endif // !defined(CPPLINQ_LINQ_TAKE_HPP)

