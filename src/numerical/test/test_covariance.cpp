/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/covariance.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <vector>

namespace ns::numerical
{
namespace
{
template <typename T>
void test_equal(const T a, const T b)
{
        if (a == b)
        {
                return;
        }
        error(to_string(a) + " is not equal to " + to_string(b));
}

template <typename T>
void test_simple()
{
        const std::vector<Vector<2, T>> data{
                {5, 10},
                {7, 11},
                {4,  8},
                {3,  6},
                {6, 10}
        };

        const Matrix<2, 2, T> m = covariance_matrix_simple(data);

        test_equal(m[0, 0], T{10});
        test_equal(m[0, 1], T{12});
        test_equal(m[1, 0], T{0});
        test_equal(m[1, 1], T{16});
}

template <typename T>
void test_full()
{
        const std::vector<Vector<2, T>> data{
                {5, 10},
                {7, 11},
                {4,  8},
                {3,  6},
                {6, 10}
        };

        const Matrix<2, 2, T> m = covariance_matrix_full(data);

        test_equal(m[0, 0], T{2.0L});
        test_equal(m[0, 1], T{2.4L});
        test_equal(m[1, 0], T{2.4L});
        test_equal(m[1, 1], T{3.2L});
}

template <typename T>
void test()
{
        test_simple<T>();
        test_full<T>();
}

void test_covariance()
{
        test<float>();
        test<double>();
        test<long double>();
}

TEST_SMALL("Covariance", test_covariance)
}
}
