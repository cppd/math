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

#include "../../consistency.h"
#include "../estimation.h"
#include "../measurement.h"
#include "../time_point.h"

#include <src/color/rgb8.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T>
class Process final
{
        std::string name_;
        color::RGB8 color_;
        T reset_dt_;
        std::optional<T> gate_;
        std::unique_ptr<ProcessFilter<T>> filter_;

        std::vector<TimePoint<2, T>> positions_;
        std::vector<TimePoint<2, T>> positions_p_;
        std::vector<TimePoint<1, T>> speeds_;
        std::vector<TimePoint<1, T>> speeds_p_;

        struct Nees final
        {
                NormalizedSquared<2, T> position;
                NormalizedSquared<1, T> speed;
                NormalizedSquared<1, T> angle;
                NormalizedSquared<1, T> angle_r;
        };

        std::optional<Nees> nees_;
        std::optional<T> last_time_;

        [[nodiscard]] std::string angle_string() const;

        void save(T time, const TrueData<2, T>& true_data);

        void check_time(T time) const;

        void update_position(const Measurement<2, T>& position, const Measurements<2, T>& m, T dt);

        [[nodiscard]] bool update_non_position(const Measurements<2, T>& m, T dt);

public:
        Process(std::string name,
                color::RGB8 color,
                T reset_dt,
                std::optional<T> gate,
                std::unique_ptr<ProcessFilter<T>>&& filter);

        void update(const Measurements<2, T>& m, const Estimation<T>& estimation);

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] color::RGB8 color() const;

        [[nodiscard]] std::string consistency_string() const;
        [[nodiscard]] const std::vector<TimePoint<2, T>>& positions() const;
        [[nodiscard]] const std::vector<TimePoint<2, T>>& positions_p() const;
        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds() const;
        [[nodiscard]] const std::vector<TimePoint<1, T>>& speeds_p() const;
};
}
