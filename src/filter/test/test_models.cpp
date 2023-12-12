/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../models.h"

#include <src/numerical/matrix.h>

#include <cstddef>

namespace ns::filter
{
namespace
{
template <std::size_t N, typename T>
[[nodiscard]] constexpr bool equal(const Matrix<N, N, T>& a, const Matrix<N, N, T>& b)
{
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = 0; c < N; ++c)
                {
                        if (a(r, c) == b(r, c))
                        {
                                continue;
                        }
                        return false;
                }
        }
        return true;
}

template <typename T>
struct Test final
{
        void test()
        {
                constexpr Matrix<2, 2, T> CWN_2{
                        {0.0208333333333333333339L, 0.0625L},
                        {                  0.0625L,   0.25L}
                };
                static_assert(equal(continuous_white_noise<2, T>(0.5, 0.5), CWN_2));

                constexpr Matrix<3, 3, T> CWN_3{
                        {0.000781250000000000000011L,               0.00390625L, 0.0104166666666666666669L},
                        {                0.00390625L, 0.0208333333333333333339L,                   0.0625L},
                        {  0.0104166666666666666669L,                   0.0625L,                     0.25L}
                };
                static_assert(equal(continuous_white_noise<3, T>(0.5, 0.5), CWN_3));

                constexpr Matrix<2, 2, T> DWN_2{
                        {0.0078125L, 0.03125L},
                        {  0.03125L,   0.125L}
                };
                static_assert(equal(discrete_white_noise<2, T>(0.5, 0.5), DWN_2));

                constexpr Matrix<3, 3, T> DWN_3{
                        {0.0078125, 0.03125, 0.0625},
                        {  0.03125,   0.125,   0.25},
                        {   0.0625,    0.25,    0.5}
                };
                static_assert(equal(discrete_white_noise<3, T>(0.5, 0.5), DWN_3));
        }
};

template void Test<float>::test();
template void Test<double>::test();
template void Test<long double>::test();
}
}
