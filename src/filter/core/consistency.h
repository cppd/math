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
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <string>

namespace ns::filter::core
{
// Average of normalized (state) estimation error squared (NEES).
// Average of normalized innovation squared (NIS).
template <std::size_t N, typename T>
class NormalizedSquared final
{
        static_assert(N >= 1);
        static_assert(std::is_floating_point_v<T>);

        long double sum_ = 0;
        unsigned long long count_ = 0;

        [[nodiscard]] unsigned long long degrees_of_freedom() const
        {
                return count_ * N;
        }

public:
        void add(const numerical::Vector<N, T>& difference, const numerical::Matrix<N, N, T>& covariance)
        {
                sum_ += dot(difference * covariance.inversed(), difference);
                ++count_;
        }

        void add(const T difference, const T variance)
                requires (N == 1)
        {
                add(numerical::Vector<1, T>(difference), numerical::Matrix<1, 1, T>{{variance}});
        }

        void add(const T normalized_squared)
        {
                sum_ += normalized_squared;
                ++count_;
        }

        [[nodiscard]] bool empty() const
        {
                return count_ == 0;
        }

        [[nodiscard]] T average() const
        {
                if (!empty())
                {
                        return sum_ / degrees_of_freedom();
                }
                error("No data to compute normalized squared average");
        }

        [[nodiscard]] std::string check_string() const
        {
                return to_string(average()) + "; DOF = " + to_string(degrees_of_freedom());
        }
};
}
