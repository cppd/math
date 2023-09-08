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

#include "position_filter.h"

#include "../../consistency.h"
#include "../measurement.h"
#include "../time_point.h"

#include <src/color/rgb8.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test::position
{
template <std::size_t N, typename T>
class Position final
{
        std::string name_;
        color::RGB8 color_;
        T reset_dt_;
        T linear_dt_;
        std::optional<T> gate_;
        std::unique_ptr<PositionFilter<N, T>> filter_;

        std::vector<TimePoint<N, T>> positions_;
        std::vector<TimePoint<N, T>> positions_p_;
        std::vector<TimePoint<1, T>> speeds_;
        std::vector<TimePoint<1, T>> speeds_p_;

        NormalizedSquared<N, T> nees_position_;
        NormalizedSquared<1, T> nees_speed_;
        NormalizedSquared<1, T> nis_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void save_results(T time);
        void add_nees_checks(const TrueData<N, T>& true_data);

        void check_time(T time) const;

public:
        Position(
                std::string name,
                color::RGB8 color,
                T reset_dt,
                T linear_dt,
                std::optional<T> gate,
                std::unique_ptr<PositionFilter<N, T>>&& filter);

        void update_position(const Measurements<N, T>& m);
        void predict_update(const Measurements<N, T>& m);

        [[nodiscard]] bool empty() const;

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] color::RGB8 color() const;

        [[nodiscard]] Vector<N, T> velocity() const;
        [[nodiscard]] Matrix<N, N, T> velocity_p() const;
        [[nodiscard]] Vector<3 * N, T> position_velocity_acceleration() const;
        [[nodiscard]] Matrix<3 * N, 3 * N, T> position_velocity_acceleration_p() const;

        [[nodiscard]] std::string consistency_string() const;
        [[nodiscard]] const std::vector<TimePoint<N, T>>& positions() const;
        [[nodiscard]] const std::vector<TimePoint<N, T>>& positions_p() const;
        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds() const;
        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds_p() const;
};
}
