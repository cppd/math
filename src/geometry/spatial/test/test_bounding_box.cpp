/*
Copyright (C) 2017-2021 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../bounding_box.h"

namespace ns::geometry::spatial::test
{
namespace
{
template <typename T>
struct Test
{
        static constexpr BoundingBox<4, T> BOX{Vector<4, T>(1, -2, 3, -4), Vector<4, T>(-5, 6, -7, 8)};
        static_assert(BOX.min() == Vector<4, T>(-5, -2, -7, -4));
        static_assert(BOX.max() == Vector<4, T>(1, 6, 3, 8));
        static_assert(BOX.diagonal() == Vector<4, T>(6, 8, 10, 12));
        static_assert(BOX.center() == Vector<4, T>(-2, 2, -2, 2));
        static_assert(BOX.volume() == 5760);
        static_assert(BOX.surface() == 2736);

        static constexpr BoundingBox<4, T> BOX_MERGE_1 = []
        {
                BoundingBox<4, T> b(BOX);
                b.merge(Vector<4, T>(5, -5, 5, -5));
                return b;
        }();
        static_assert(BOX_MERGE_1.min() == Vector<4, T>(-5, -5, -7, -5));
        static_assert(BOX_MERGE_1.max() == Vector<4, T>(5, 6, 5, 8));

        static constexpr BoundingBox<4, T> BOX_MERGE_2 = []
        {
                BoundingBox<4, T> b(BOX);
                b.merge(BoundingBox<4, T>(Vector<4, T>(4, -3, 2, -1), Vector<4, T>(-4, 5, -6, 7)));
                return b;
        }();
        static_assert(BOX_MERGE_2.min() == Vector<4, T>(-5, -3, -7, -4));
        static_assert(BOX_MERGE_2.max() == Vector<4, T>(4, 6, 3, 8));

        static constexpr BoundingBox<4, T> BOX_POINT{Vector<4, T>(1, -2, 3, -4)};
        static_assert(BOX_POINT.min() == Vector<4, T>(1, -2, 3, -4));
        static_assert(BOX_POINT.max() == Vector<4, T>(1, -2, 3, -4));

        static constexpr BoundingBox<4, T> BOX_ARRAY{
                std::array{Vector<4, T>(1, -2, 3, -4), Vector<4, T>(-5, 6, -7, 8)}};
        static_assert(BOX_ARRAY.min() == Vector<4, T>(-5, -2, -7, -4));
        static_assert(BOX_ARRAY.max() == Vector<4, T>(1, 6, 3, 8));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;
}
}
