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

#include "direction_1_1.h"

#include "update.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/type/name.h>

namespace ns::filter::test::filter::direction
{
namespace
{
template <typename T>
constexpr T INIT_ANGLE = 0;
template <typename T>
constexpr T INIT_ANGLE_VARIANCE = square(degrees_to_radians(100.0));
}

template <typename T>
Direction11<T>::Direction11(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          angle_estimation_variance_(angle_estimation_variance),
          gate_(gate),
          filter_(create_filter_1_1(sigma_points_alpha, position_variance, angle_variance)),
          queue_(reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <typename T>
void Direction11<T>::save(const T time, const TrueData<2, T>& true_data)
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
        nees_->angle.add(normalize_angle(true_data.angle + true_data.angle_r - filter_->angle()), filter_->angle_p());
}

template <typename T>
void Direction11<T>::check_time(const T time) const
{
        if (last_time_ && !(*last_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_time_) + " to " + to_string(time));
        }

        if (last_position_time_ && !(*last_position_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_position_time_) + " to "
                      + to_string(time));
        }
}

template <typename T>
void Direction11<T>::reset(const Measurements<2, T>& m, const Estimation<T>& estimation)
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
                                queue_.init_position_velocity(), queue_.init_position_velocity_p(), INIT_ANGLE<T>,
                                INIT_ANGLE_VARIANCE<T>);
                },
                [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                {
                        update_position(filter_.get(), position, measurements.direction, measurements.speed, gate_, dt);
                });

        last_time_ = m.time;
        last_position_time_ = m.time;
}

template <typename T>
void Direction11<T>::update(const Measurements<2, T>& m, const Estimation<T>& estimation)
{
        check_time(m.time);

        queue_.update(m, estimation);

        if (!m.position)
        {
                return;
        }

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                reset(m, estimation);
                return;
        }

        if (!m.position && last_position_time_ && !(m.time - *last_position_time_ < reset_dt_))
        {
                return;
        }

        const T dt = m.time - *last_time_;
        const bool has_angle = estimation.has_angle() && (estimation.angle_p() <= angle_estimation_variance_);

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return;
                }

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};
                const std::optional<Measurement<1, T>> direction = has_angle ? m.direction : std::nullopt;
                update_position(filter_.get(), position, direction, m.speed, gate_, dt);

                last_position_time_ = m.time;

                LOG(to_string(m.time) + "; true angle = "
                    + to_string(radians_to_degrees(normalize_angle(m.true_data.angle + m.true_data.angle_r))) + "; "
                    + angle_string());
        }
        else
        {
                const std::optional<Measurement<1, T>> direction = has_angle ? m.direction : std::nullopt;
                if (!update_non_position(filter_.get(), direction, m.speed, gate_, dt))
                {
                        return;
                }
        }

        last_time_ = m.time;

        save(m.time, m.true_data);
}

template <typename T>
const std::string& Direction11<T>::name() const
{
        return name_;
}

template <typename T>
color::RGB8 Direction11<T>::color() const
{
        return color_;
}

template <typename T>
std::string Direction11<T>::angle_string() const
{
        std::string s;
        s += name_;
        s += "; angle = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
        s += "; angle speed = " + to_string(radians_to_degrees(normalize_angle(filter_->angle_speed())));
        return s;
}

template <typename T>
std::string Direction11<T>::consistency_string() const
{
        if (!nees_)
        {
                return {};
        }

        const std::string name = std::string("Direction<") + type_name<T>() + "> " + name_;

        std::string s;

        const auto new_line = [&]()
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += name;
        };

        new_line();
        s += "; NEES position; " + nees_->position.check_string();

        new_line();
        s += "; NEES speed; " + nees_->speed.check_string();

        new_line();
        s += "; NEES angle; " + nees_->angle.check_string();

        return s;
}

template <typename T>
const std::vector<TimePoint<2, T>>& Direction11<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<TimePoint<2, T>>& Direction11<T>::positions_p() const
{
        return positions_p_;
}

template <typename T>
const std::vector<TimePoint<1, T>>& Direction11<T>::speeds() const
{
        return speeds_;
}

template <typename T>
const std::vector<TimePoint<1, T>>& Direction11<T>::speeds_p() const
{
        return speeds_p_;
}

template class Direction11<float>;
template class Direction11<double>;
template class Direction11<long double>;
}
