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

/*
Yaakov Bar-Shalom, X.-Rong Li, Thiagalingam Kirubarajan.
Estimation with Applications To Tracking and Navigation.
John Wiley & Sons, 2001.

5.4 CONSISTENCY OF STATE ESTIMATORS
*/

/*
Roger R Labbe Jr.
Kalman and Bayesian Filters in Python.

8.7 Evaluating Filter Performance
*/

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <string>

namespace ns::filter
{
// Average of normalized (state) estimation error squared (NEES)
template <std::size_t N, typename T>
class NeesAverage final
{
        static_assert(N >= 1);
        static_assert(std::is_floating_point_v<T>);

        long double sum_ = 0;
        std::size_t count_ = 0;

public:
        void add(const Vector<N, T>& value, const Vector<N, T>& estimate, const Matrix<N, N, T>& covariance)
        {
                const Vector<N, T> x = value - estimate;
                const T nees = dot(x * covariance.inversed(), x);
                sum_ += nees;
                ++count_;
        }

        void add(const T value, const T estimate, const T variance)
                requires (N == 1)
        {
                add(Vector<1, T>(value), Vector<1, T>(estimate), Matrix<1, 1, T>{{variance}});
        }

        [[nodiscard]] T average() const
        {
                if (!(count_ > 0))
                {
                        error("No data to compute NEES average");
                }
                return sum_ / count_;
        }

        [[nodiscard]] static constexpr T max()
        {
                return N;
        }

        [[nodiscard]] std::string check_string() const
        {
                const T a = average();
                return std::string("NEES average <") + type_name<T>() + "> = " + to_string(a) + "; " + to_string(N)
                       + " degree" + (N > 1 ? "s" : "") + " of freedom; check " + (a <= max() ? "passed" : "failed");
        }
};
}
