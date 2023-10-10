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

#include "process_ukf.h"

#include "filter_ukf.h"
#include "update.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/name.h>

namespace ns::filter::test::filter::process
{
namespace
{
template <typename T>
constexpr T INIT_ANGLE = 0;
template <typename T>
constexpr T INIT_ANGLE_VARIANCE = square(degrees_to_radians(100.0));
}

template <typename T>
ProcessUkf<T>::ProcessUkf(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          angle_estimation_variance_(angle_estimation_variance),
          gate_(gate),
          filter_(create_filter_ukf(sigma_points_alpha, position_variance, angle_variance, angle_r_variance)),
          queue_(reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <typename T>
void ProcessUkf<T>::save(const T time, const TrueData<2, T>& true_data)
{
        positions_.push_back({.time = time, .point = filter_->position()});
        positions_p_.push_back({.time = time, .point = filter_->position_p().diagonal()});
        speeds_.push_back({.time = time, .point = Vector<1, T>(filter_->speed())});
        speeds_p_.push_back({.time = time, .point = Vector<1, T>(filter_->speed_p())});

        if (!nees_)
        {
                nees_.emplace();
        }
        nees_->position.add(true_data.position - filter_->position(), filter_->position_p());
        nees_->speed.add(true_data.speed - filter_->speed(), filter_->speed_p());
        nees_->angle.add(normalize_angle(true_data.angle - filter_->angle()), filter_->angle_p());
        nees_->angle_r.add(normalize_angle(true_data.angle_r - filter_->angle_r()), filter_->angle_r_p());
}

template <typename T>
void ProcessUkf<T>::check_time(const T time) const
{
        if (last_time_ && !(*last_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_time_) + " to " + to_string(time));
        }
}

template <typename T>
void ProcessUkf<T>::update(const Measurements<2, T>& m, const Estimation<T>& estimation)
{
        check_time(m.time);

        queue_.update(m, estimation);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                if (!m.position || queue_.empty())
                {
                        return;
                }

                ASSERT(queue_.measurements().back().time == m.time);
                LOG(name_ + "; " + estimation.description());
                update_filter(
                        queue_,
                        [&]()
                        {
                                filter_->reset(
                                        queue_.init_position_velocity(), queue_.init_position_velocity_p(),
                                        INIT_ANGLE<T>, INIT_ANGLE_VARIANCE<T>);
                        },
                        [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                        {
                                update_position(
                                        filter_.get(), position, measurements.acceleration, measurements.direction,
                                        measurements.speed, gate_, dt);
                        });
                last_time_ = m.time;
                return;
        }

        const T dt = m.time - *last_time_;

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return;
                }

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update_position(filter_.get(), position, m.acceleration, m.direction, m.speed, gate_, dt);

                LOG(to_string(m.time) + "; true angle = " + to_string(radians_to_degrees(m.true_data.angle)) + "; "
                    + angle_string());
        }
        else
        {
                if (!update_non_position(filter_.get(), m.acceleration, m.direction, m.speed, gate_, dt))
                {
                        return;
                }
        }

        last_time_ = m.time;

        save(m.time, m.true_data);
}

template <typename T>
const std::string& ProcessUkf<T>::name() const
{
        return name_;
}

template <typename T>
color::RGB8 ProcessUkf<T>::color() const
{
        return color_;
}

template <typename T>
std::string ProcessUkf<T>::angle_string() const
{
        std::string s;
        s += name_;
        s += "; angle = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
        s += "; angle speed = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_speed())));
        s += "; angle r = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_r())));
        return s;
}

template <typename T>
std::string ProcessUkf<T>::consistency_string() const
{
        if (!nees_)
        {
                return {};
        }

        const std::string name = std::string("Process<") + type_name<T>() + "> " + name_;
        std::string s;
        s += name + "; NEES position; " + nees_->position.check_string();
        s += '\n';
        s += name + "; NEES speed; " + nees_->speed.check_string();
        s += '\n';
        s += name + "; NEES angle; " + nees_->angle.check_string();
        s += '\n';
        s += name + "; NEES angle r; " + nees_->angle_r.check_string();
        return s;
}

template <typename T>
const std::vector<TimePoint<2, T>>& ProcessUkf<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<TimePoint<2, T>>& ProcessUkf<T>::positions_p() const
{
        return positions_p_;
}

template <typename T>
const std::vector<TimePoint<1, T>>& ProcessUkf<T>::speeds() const
{
        return speeds_;
}

template <typename T>
const std::vector<TimePoint<1, T>>& ProcessUkf<T>::speeds_p() const
{
        return speeds_p_;
}

template class ProcessUkf<float>;
template class ProcessUkf<double>;
template class ProcessUkf<long double>;
}