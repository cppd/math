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

#pragma once

#include "process_filter.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>

namespace ns::filter::test
{
template <typename T>
std::unique_ptr<ProcessFilter<T>> create_process_filter_ukf(
        T position_variance,
        T angle_variance,
        T angle_r_variance,
        const Vector<9, T>& x,
        const Matrix<9, 9, T>& p);
}
