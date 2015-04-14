// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_LAST_HPP)
#define CPPLINQ_LINQ_LAST_HPP
#pragma once

namespace cpplinq { 

    template <class Cursor>
    typename Cursor::element_type
        linq_last_(Cursor c, onepass_cursor_tag)
    {
        if (c.empty()) { throw std::logic_error("last() out of bounds"); }
        typename Cursor::element_type elem = c.get();
        for(;;) {
            c.inc();
            if (c.empty()) break;
            elem = c.get();
        }
        return elem;
    }

    // TODO: bidirectional iterator in constant time

    template <class Cursor>
    typename Cursor::reference_type
        linq_last_(Cursor c, forward_cursor_tag)
    {
        if (c.empty()) { throw std::logic_error("last() out of bounds"); }
        Cursor best = c;
        for(;;) {
            c.inc();
            if (c.empty()) break;
            best = c;
        }
        return best.get();
    }

    template <class Cursor>
    typename Cursor::reference_type
        linq_last_(Cursor c, random_access_cursor_tag)
    {
        if (c.empty()) { throw std::logic_error("last() out of bounds"); }
        c.skip(c.size()-1);
        return c.get();
    }

    template <class Cursor>
    typename Cursor::element_type
        linq_last_or_default_(Cursor c, onepass_cursor_tag)
    {
        typename Cursor::element_type elem;
        while(!c.empty()) {
            elem = c.get();
            c.inc();
        }
        return elem;
    }

    template <class Cursor>
    typename Cursor::element_type
        linq_last_or_default_(Cursor c, forward_cursor_tag)
    {
        if (c.empty()) { throw std::logic_error("last() out of bounds"); }
        Cursor best = c;
        for(;;) {
            c.inc();
            if (c.empty()) break;
            best = c;
        }
        return best.get();
    }

    template <class Cursor>
    typename Cursor::element_type
        linq_last_or_default_(Cursor c, random_access_cursor_tag)
    {
        if (c.empty()) { return typename Cursor::element_type(); }
        c.skip(c.size()-1);
        return c.get();
    }

}

#endif // CPPLINQ_LINQ_LAST_HPP
