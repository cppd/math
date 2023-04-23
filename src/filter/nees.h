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
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <vector>

namespace ns::filter
{
// Average of normalized (state) estimation error squared (NEES)
template <std::size_t N, typename T>
T nees_average(
        const std::vector<Vector<N, T>>& values,
        const std::vector<Vector<N, T>>& estimates,
        const std::vector<Matrix<N, N, T>>& covariances)
{
        if (values.size() != estimates.size() || values.size() != covariances.size())
        {
                error("NEES data size error");
        }

        T sum = 0;
        for (std::size_t i = 0; i < values.size(); ++i)
        {
                const Vector<N, T> x = values[i] - estimates[i];
                const T nees = dot(x * covariances[i].inversed(), x);
                sum += nees;
        }

        return sum / values.size();
}
}
