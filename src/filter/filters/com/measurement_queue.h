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
#include <src/com/print.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/measurement.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <deque>

namespace ns::filter::filters::com
{
template <std::size_t N, typename T>
class MeasurementQueue final
{
        struct Init final
        {
                numerical::Vector<2 * N, T> position_velocity;
                numerical::Matrix<2 * N, 2 * N, T> position_velocity_p;
        };

        const std::size_t size_;
        const T reset_dt_;
        const T angle_estimation_variance_;

        std::deque<Init> inits_;
        std::deque<Measurements<N, T>> measurements_;

public:
        MeasurementQueue(const std::size_t size, const T reset_dt, const T angle_estimation_variance)
                : size_(size),
                  reset_dt_(reset_dt),
                  angle_estimation_variance_(angle_estimation_variance)
        {
                if (size_ < 2)
                {
                        error("Measurement queue size " + to_string(size_) + " must be greater than or equal to 2");
                }
        }

        void update(const Measurements<N, T>& m, const Estimation<N, T>& estimation)
        {
                if (!m.position || !m.position->variance)
                {
                        return;
                }

                if (!measurements_.empty() && !(m.time - measurements_.back().time < reset_dt_))
                {
                        inits_.clear();
                        measurements_.clear();
                        return;
                }

                if (!estimation.angle_variance_less_than(angle_estimation_variance_))
                {
                        inits_.clear();
                        measurements_.clear();
                        return;
                }

                inits_.push_back(
                        {.position_velocity = estimation.position_velocity(),
                         .position_velocity_p = estimation.position_velocity_p()});

                measurements_.push_back(m);
        }

        [[nodiscard]] std::size_t empty() const
        {
                ASSERT(inits_.size() == measurements_.size());

                return measurements_.size() < size_;
        }

        [[nodiscard]] T init_time() const
        {
                ASSERT(!empty());

                return measurements_.front().time;
        }

        [[nodiscard]] const numerical::Vector<2 * N, T>& init_position_velocity() const
        {
                ASSERT(!empty());

                return inits_.front().position_velocity;
        }

        [[nodiscard]] const numerical::Matrix<2 * N, 2 * N, T>& init_position_velocity_p() const
        {
                ASSERT(!empty());

                return inits_.front().position_velocity_p;
        }

        [[nodiscard]] const std::deque<Measurements<N, T>>& measurements() const
        {
                ASSERT(!empty());

                return measurements_;
        }
};

template <std::size_t N, typename T, typename Init, typename Update>
void update_filter(const MeasurementQueue<N, T>& queue, const Init init, const Update update)
{
        if (queue.empty())
        {
                error("Measurement queue is empty");
        }

        init();

        T last_time = queue.init_time();

        const auto& measurements = queue.measurements();
        auto iter = measurements.cbegin();
        ASSERT(iter != measurements.cend());
        for (++iter; iter != measurements.cend(); ++iter)
        {
                const Measurements<N, T>& m = *iter;

                ASSERT(m.position);
                ASSERT(m.position->variance);

                const T dt = m.time - last_time;
                last_time = m.time;

                const Measurement<N, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update(position, m, dt);
        }
}
}
