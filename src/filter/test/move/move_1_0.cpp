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

#include "move_1_0.h"

#include "update.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/type/name.h>

namespace ns::filter::test
{
namespace
{
template <typename T>
constexpr T INIT_ANGLE = 0;
template <typename T>
constexpr T INIT_ANGLE_VARIANCE = square(degrees_to_radians(100.0));
}

template <typename T>
Move10<T>::Move10(
        std::string name,
        const color::RGB8 color,
        const T reset_dt,
        const T angle_p,
        const std::optional<T> gate,
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance)
        : name_(std::move(name)),
          color_(color),
          reset_dt_(reset_dt),
          angle_p_(angle_p),
          gate_(gate),
          filter_(create_move_filter_1_0(sigma_points_alpha, position_variance, angle_variance))
{
        ASSERT(filter_);
}

template <typename T>
void Move10<T>::save(const T time, const TrueData<2, T>& true_data)
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
void Move10<T>::check_time(const T time) const
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
void Move10<T>::update(const Measurements<2, T>& m, const Estimation<T>& estimation)
{
        check_time(m.time);

        if (!m.position)
        {
                return;
        }

        const bool has_angle = estimation.has_position() && (estimation.position_angle_p() <= angle_p_);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                if (m.position && has_angle)
                {
                        LOG(name_ + "; " + estimation.position_description());

                        filter_->reset(
                                estimation.position_velocity_acceleration(),
                                estimation.position_velocity_acceleration_p(), INIT_ANGLE<T>, INIT_ANGLE_VARIANCE<T>);

                        last_time_ = m.time;
                }
                return;
        }

        if (!m.position && last_position_time_ && !(m.time - *last_position_time_ < reset_dt_))
        {
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
const std::string& Move10<T>::name() const
{
        return name_;
}

template <typename T>
color::RGB8 Move10<T>::color() const
{
        return color_;
}

template <typename T>
std::string Move10<T>::angle_string() const
{
        std::string s;
        s += name_;
        s += "; angle = " + to_string(radians_to_degrees(normalize_angle(filter_->angle())));
        return s;
}

template <typename T>
std::string Move10<T>::consistency_string() const
{
        if (!nees_)
        {
                return {};
        }

        const std::string name = std::string("Move<") + type_name<T>() + "> " + name_;

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
const std::vector<TimePoint<2, T>>& Move10<T>::positions() const
{
        return positions_;
}

template <typename T>
const std::vector<TimePoint<2, T>>& Move10<T>::positions_p() const
{
        return positions_p_;
}

template <typename T>
const std::vector<TimePoint<1, T>>& Move10<T>::speeds() const
{
        return speeds_;
}

template <typename T>
const std::vector<TimePoint<1, T>>& Move10<T>::speeds_p() const
{
        return speeds_p_;
}

template class Move10<float>;
template class Move10<double>;
template class Move10<long double>;
}
