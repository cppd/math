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

#include "consistency.h"
#include "filter_2.h"
#include "init.h"

#include <src/filter/filters/com/measurement_queue.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::speed
{
template <std::size_t N, typename T>
class Speed2 final : public Filter<N, T>
{
        T reset_dt_;
        std::optional<T> gate_;
        std::unique_ptr<Filter2<N, T>> filter_;
        Init<T> init_;

        com::MeasurementQueue<N, T> queue_;

        std::optional<Nees<N, T>> nees_;
        std::optional<Nis<N, T>> nis_;

        std::optional<T> last_time_;
        std::optional<T> last_position_time_;

        void check_time(T time) const;

        void reset(const Measurements<N, T>& m);

public:
        Speed2(std::size_t measurement_queue_size,
               T reset_dt,
               T angle_estimation_variance,
               std::optional<T> gate,
               T sigma_points_alpha,
               T position_variance,
               const Init<T>& init);

        [[nodiscard]] std::optional<UpdateInfo<N, T>> update(
                const Measurements<N, T>& m,
                const Estimation<N, T>& estimation) override;

        [[nodiscard]] std::string consistency_string() const override;
};
}
