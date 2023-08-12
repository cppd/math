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

#include "measurement.h"
#include "point.h"
#include "position_filter.h"
#include "variance.h"

#include <src/color/rgb8.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T>
class PositionVariance final
{
        std::string name_;
        color::RGB8 color_;
        T reset_dt_;
        std::unique_ptr<PositionFilter<N, T>> filter_;

        std::vector<Point<N, T>> positions_;
        std::vector<Point<N, T>> positions_p_;
        std::vector<Point<1, T>> speeds_;
        std::vector<Point<1, T>> speeds_p_;

        MovingVariance<N, T> position_variance_;
        std::optional<Vector<N, T>> last_position_variance_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void save_results(T time);
        void check_time(T time) const;
        void update_position_variance(const Measurements<N, T>& m);

public:
        PositionVariance(
                std::string name,
                color::RGB8 color,
                T reset_dt,
                std::unique_ptr<PositionFilter<N, T>>&& filter);

        void update_position(const Measurements<N, T>& m);

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] color::RGB8 color() const;

        [[nodiscard]] const std::optional<Vector<N, T>>& last_position_variance() const;
        [[nodiscard]] std::string consistency_string() const;
};
}
