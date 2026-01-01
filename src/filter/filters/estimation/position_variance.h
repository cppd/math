/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "moving_variance.h"

#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/filters/position/filter_2.h>
#include <src/filter/filters/position/init.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::estimation
{
template <std::size_t N, typename T>
class PositionVariance final
{
        const T reset_dt_;
        const position::Init<T> init_;
        const NoiseModel<T> noise_model_;
        const T fading_memory_alpha_;
        std::unique_ptr<position::Filter2<N, T>> filter_;

        MovingVariance<N, T> position_variance_;
        std::optional<numerical::Vector<N, T>> last_position_variance_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void check_time(T time) const;
        void update_position_variance(const Measurements<N, T>& m);

public:
        PositionVariance(
                T reset_dt,
                const NoiseModel<T>& noise_model,
                T fading_memory_alpha,
                const position::Init<T>& init);

        void update(const Measurements<N, T>& m);

        [[nodiscard]] std::string consistency_string() const;

        [[nodiscard]] const std::optional<numerical::Vector<N, T>>& last_position_variance() const;
};
}
