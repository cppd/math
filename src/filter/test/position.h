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
#include "position_filter.h"

#include "../consistency.h"

#include <src/color/rgb8.h>
#include <src/numerical/variance.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T>
class Position final
{
        std::string name_;
        color::RGB8 color_;
        T reset_dt_;
        std::unique_ptr<PositionFilter<T>> filter_;

        std::vector<Vector<3, T>> positions_;
        std::vector<Vector<2, T>> speeds_;
        std::vector<Vector<2, T>> speeds_p_;

        NormalizedSquared<2, T> nees_position_;
        NormalizedSquared<1, T> nees_speed_;

        numerical::MovingVariance<Vector<2, T>> variance_;
        unsigned long long update_count_{0};

        std::optional<T> last_filter_time_;
        std::optional<T> last_position_time_;

        void save(T time, const TrueData<2, T>& true_data);

        void check_time(T time) const;

public:
        Position(std::string name, color::RGB8 color, T reset_dt, std::unique_ptr<PositionFilter<T>>&& filter);

        void update_position(const Measurements<2, T>& m);
        void predict_update(const Measurements<2, T>& m);

        [[nodiscard]] const PositionFilter<T>* filter() const;

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] color::RGB8 color() const;

        [[nodiscard]] std::string consistency_string() const;
        [[nodiscard]] const std::vector<Vector<3, T>>& positions() const;
        [[nodiscard]] const std::vector<Vector<2, T>>& speeds() const;
        [[nodiscard]] const std::vector<Vector<2, T>>& speeds_p() const;
};
}
