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

#include "../estimation.h"
#include "../measurement.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>
#include <vector>

namespace ns::filter::test::move
{
template <std::size_t N, typename T>
class Queue final
{
        static constexpr std::size_t SIZE = 20;

        T reset_dt_;
        T angle_estimation_variance_;
        std::optional<T> last_time_;
        T init_time_;
        Vector<2 * N, T> init_position_velocity_;
        Matrix<2 * N, 2 * N, T> init_position_velocity_p_;
        std::vector<Measurements<N, T>> measurements_;

public:
        Queue(T reset_dt, const T angle_estimation_variance)
                : reset_dt_(reset_dt),
                  angle_estimation_variance_(angle_estimation_variance)
        {
        }

        void update(const Measurements<2, T>& m, const Estimation<T>& estimation)
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

                if (!estimation.has_angle() || !(estimation.angle_p() <= angle_estimation_variance_))
                {
                        last_time_.reset();
                        measurements_.clear();
                        return;
                }

                last_time_ = m.time;

                if (measurements_.empty())
                {
                        init_time_ = m.time;
                        init_position_velocity_ = estimation.position_velocity();
                        init_position_velocity_p_ = estimation.position_velocity_p();
                        return;
                }

                measurements_.push_back(m);
        }

        [[nodiscard]] std::size_t empty() const
        {
                return measurements_.size() < SIZE;
        }

        [[nodiscard]] T init_time() const
        {
                ASSERT(!empty());

                return init_time_;
        }

        [[nodiscard]] const Vector<2 * N, T>& init_position_velocity() const
        {
                ASSERT(!empty());

                return init_position_velocity_;
        }

        [[nodiscard]] const Matrix<2 * N, 2 * N, T>& init_position_velocity_p() const
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
}
