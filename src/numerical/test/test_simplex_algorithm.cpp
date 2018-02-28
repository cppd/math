/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "test_simplex_algorithm.h"

#include "com/log.h"
#include "com/print.h"
#include "com/types.h"
#include "numerical/simplex.h"

template <size_t N, size_t M, typename T>
void print_data(std::array<T, M>& b, std::array<std::array<T, N>, M>& a, T& v, std::array<T, N>& c)
{
        // Исключить вызов функции to_string для типа __float128
        if constexpr (std::is_floating_point_v<T>)
        {
                LOG("z = " + to_string(v) + " + " + to_string(c));
                for (int i = 0; i < 3; ++i)
                {
                        LOG(to_string(b[i]) + " + " + to_string(a[i]));
                }
        }
}

template <typename T>
void test_pivot()
{
        LOG(std::string("PIVOT ") + type_name<T>());

        std::array<T, 3> b{{30, 24, 36}};
        std::array<std::array<T, 3>, 3> a{{{{-1, -1, -3}}, {{-2, -2, -5}}, {{-4, -1, -2}}}};
        T v = 5;
        std::array<T, 3> c{{3, 1, 2}};

        pivot(b, a, v, c, 2, 0);

        print_data(b, a, v, c);

        if (!(b == std::array<T, 3>{{21, 6, 9}}))
        {
                error("b error");
        }

        if (!(a == std::array<std::array<T, 3>, 3>{{{{0.25, -0.75, -2.5}}, {{0.5, -1.5, -4}}, {{-0.25, -0.25, -0.5}}}}))
        {
                error("a error");
        }

        if (!(v == 32))
        {
                error("v error");
        }

        if (!(c == std::array<T, 3>{{-0.75, 0.25, 0.5}}))
        {
                error("c error");
        }
}

void test_simplex_algorithm()
{
        test_pivot<float>();
        test_pivot<double>();
        test_pivot<long double>();
        test_pivot<__float128>();
}
