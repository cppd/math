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

#pragma once

#include "filter_1_0.h"
#include "init.h"

#include "../../consistency.h"
#include "../estimation.h"
#include "../filter.h"
#include "../measurement.h"
#include "../utility/measurement_queue.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::direction
{
template <typename T>
class Direction10 final : public Filter<2, T>
{
        T reset_dt_;
        T angle_estimation_variance_;
        std::optional<T> gate_;
        std::unique_ptr<Filter10<T>> filter_;
        Init<T> init_;

        utility::MeasurementQueue<2, T> queue_;

        struct Nees final
        {
                NormalizedSquared<2, T> position;
                NormalizedSquared<1, T> speed;
                NormalizedSquared<1, T> angle;
        };

        std::optional<Nees> nees_;

        std::optional<T> last_time_;
        std::optional<T> last_position_time_;

        void save(const TrueData<2, T>& true_data);

        void check_time(T time) const;

        void reset(const Measurements<2, T>& m);

public:
        Direction10(
                std::size_t measurement_queue_size,
                T reset_dt,
                T angle_estimation_variance,
                std::optional<T> gate,
                T sigma_points_alpha,
                T position_variance,
                T angle_variance,
                const Init<T>& init);

        [[nodiscard]] std::optional<UpdateInfo<2, T>> update(
                const Measurements<2, T>& m,
                const Estimation<2, T>& estimation) override;

        [[nodiscard]] std::string consistency_string() const override;
};
}
