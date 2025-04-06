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

#pragma once

#include <src/filter/core/consistency.h>
#include <src/filter/core/test/measurements.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::core::test::filters
{
template <typename T>
struct UpdateInfo final
{
        T x;
        T x_stddev;
        T v;
        T v_stddev;

        std::optional<numerical::Matrix<2, 2, T>> predict_f;
        std::optional<numerical::Vector<2, T>> predict_x;
        std::optional<numerical::Matrix<2, 2, T>> predict_p;

        numerical::Vector<2, T> update_x;
        numerical::Matrix<2, 2, T> update_p;
};

template <typename T>
class Filter
{
public:
        virtual ~Filter() = default;

        virtual void reset() = 0;

        [[nodiscard]] virtual std::optional<UpdateInfo<T>> update(const Measurements<T>& m) = 0;

        [[nodiscard]] virtual const NormalizedSquared<T>& nees() const = 0;
};
}
