// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_SKIP_HPP)
#define CPPLINQ_LINQ_SKIP_HPP
#pragma once

#include <cstddef>

namespace cpplinq 
{
    template <class Collection>
    struct linq_skip
    {
    public:
        typedef typename Collection::cursor cursor;

        linq_skip(const Collection& c, std::size_t n) : c(c), n(n) {}

        cursor get_cursor() const {
            std::size_t rem = n;

            auto cur = c.get_cursor();
            while(rem-- && !cur.empty()) {
                cur.inc();
            }
            cur.forget();
            return cur;
        }

    private:
        Collection  c;
        std::size_t      n;
    };
}
#endif // !defined(CPPLINQ_LINQ_SKIP_HPP)


