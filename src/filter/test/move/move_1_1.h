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

#include "filter_1_1.h"
#include "move.h"

#include "../../consistency.h"
#include "../estimation.h"
#include "../measurement.h"
#include "../queue.h"
#include "../time_point.h"

#include <src/color/rgb8.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test::move
{
template <typename T>
class Move11 final : public Move<T>
{
        std::string name_;
        color::RGB8 color_;
        T reset_dt_;
        T angle_estimation_variance_;
        std::optional<T> gate_;
        std::unique_ptr<Filter11<T>> filter_;

        Queue<2, T> queue_;

        std::vector<TimePoint<2, T>> positions_;
        std::vector<TimePoint<2, T>> positions_p_;
        std::vector<TimePoint<1, T>> speeds_;
        std::vector<TimePoint<1, T>> speeds_p_;

        struct Nees final
        {
                NormalizedSquared<2, T> position;
                NormalizedSquared<1, T> speed;
                NormalizedSquared<1, T> angle;
        };

        std::optional<Nees> nees_;

        std::optional<T> last_time_;
        std::optional<T> last_position_time_;

        [[nodiscard]] std::string angle_string() const;

        void save(T time, const TrueData<2, T>& true_data);

        void check_time(T time) const;

public:
        Move11(std::string name,
               color::RGB8 color,
               T reset_dt,
               T angle_estimation_variance,
               std::optional<T> gate,
               T sigma_points_alpha,
               T position_variance,
               T angle_variance);

        void update(const Measurements<2, T>& m, const Estimation<T>& estimation) override;

        [[nodiscard]] const std::string& name() const override;
        [[nodiscard]] color::RGB8 color() const override;

        [[nodiscard]] std::string consistency_string() const override;
        [[nodiscard]] const std::vector<TimePoint<2, T>>& positions() const override;
        [[nodiscard]] const std::vector<TimePoint<2, T>>& positions_p() const override;
        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds() const override;
        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds_p() const override;
};
}
