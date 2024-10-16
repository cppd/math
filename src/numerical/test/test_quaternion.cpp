/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

namespace ns::numerical
{
namespace
{
template <typename T>
struct Test final
{
        static constexpr Quaternion<T> A{1, 2, 3, 4};
        static constexpr Quaternion<T> B{11, 12, 13, 14};

        static_assert(A == Quaternion<T>(1, {2, 3, 4}));
        static_assert(A == Quaternion<T>(Vector<4, T>(1, 2, 3, 4)));
        static_assert(A[0] == 1);
        static_assert(A[1] == 2);
        static_assert(A[2] == 3);
        static_assert(A[3] == 4);
        static_assert(A.data() == Vector<4, T>(1, 2, 3, 4));
        static_assert(A.imag() == Vector<3, T>(2, 3, 4));
        static_assert(A.conjugate() == Quaternion<T>(1, -2, -3, -4));
        static_assert(A * T{3} == Quaternion<T>(3, 6, 9, 12));
        static_assert(T{3} * A == Quaternion<T>(3, 6, 9, 12));
        static_assert(A / T{2} == Quaternion<T>(T{1} / 2, 1, T{3} / 2, 2));
        static_assert(A + B == Quaternion<T>(12, 14, 16, 18));
        static_assert(A - B == Quaternion<T>(-10, -10, -10, -10));
        static_assert(A * B == Quaternion<T>(-108, 24, 66, 48));
        static_assert(A * Vector<3, T>(11, 12, 13) == Quaternion<T>(-110, 2, 30, 4));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;
}
}
