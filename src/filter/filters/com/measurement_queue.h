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

#include <src/com/error.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/measurement.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace ns::filter::filters::com
{
template <std::size_t N, typename T>
class MeasurementQueue final
{
        std::size_t size_;
        T reset_dt_;
        T angle_estimation_variance_;
        std::optional<T> last_time_;
        T init_time_;
        numerical::Vector<2 * N, T> init_position_velocity_;
        numerical::Matrix<2 * N, 2 * N, T> init_position_velocity_p_;
        std::vector<Measurements<N, T>> measurements_;

public:
        MeasurementQueue(const std::size_t size, const T reset_dt, const T angle_estimation_variance)
                : size_(size),
                  reset_dt_(reset_dt),
                  angle_estimation_variance_(angle_estimation_variance)
        {
        }

        void update(const Measurements<N, T>& m, const Estimation<N, T>& estimation)
        {
                if (!m.position || !m.position->variance)
                {
                        return;
                }

                if (last_time_ && !(m.time - *last_time_ < reset_dt_))
                {
                        last_time_.reset();
                        measurements_.clear();
                        return;
                }

                if (!(estimation.angle_variance_less_than(angle_estimation_variance_)))
                {
                        last_time_.reset();
                        measurements_.clear();
                        return;
                }

                const bool init = !last_time_;

                last_time_ = m.time;

                if (!init)
                {
                        measurements_.push_back(m);
                        return;
                }

                ASSERT(measurements_.empty());
                init_time_ = m.time;
                init_position_velocity_ = estimation.position_velocity();
                init_position_velocity_p_ = estimation.position_velocity_p();
        }

        [[nodiscard]] std::size_t empty() const
        {
                return measurements_.size() < size_;
        }

        [[nodiscard]] T init_time() const
        {
                ASSERT(!empty());

                return init_time_;
        }

        [[nodiscard]] const numerical::Vector<2 * N, T>& init_position_velocity() const
        {
                ASSERT(!empty());

                return init_position_velocity_;
        }

        [[nodiscard]] const numerical::Matrix<2 * N, 2 * N, T>& init_position_velocity_p() const
        {
                ASSERT(!empty());

                return init_position_velocity_p_;
        }

        [[nodiscard]] const std::vector<Measurements<N, T>>& measurements() const
        {
                ASSERT(!empty());

                return measurements_;
        }
};

template <std::size_t N, typename T, typename Init, typename Update>
void update_filter(const MeasurementQueue<N, T>& queue, const Init init, const Update update)
{
        ASSERT(!queue.empty());

        init();

        T last_time = queue.init_time();
        for (const Measurements<N, T>& m : queue.measurements())
        {
                ASSERT(m.position);
                ASSERT(m.position->variance);

                const T dt = m.time - last_time;
                last_time = m.time;

                const Measurement<N, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update(position, m, dt);
        }
}
}
