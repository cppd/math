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

#include "direction.h"

#include "consistency.h"
#include "filter_1_0.h"
#include "filter_1_1.h"
#include "filter_2_1.h"
#include "init.h"
#include "update.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/filter/filters/com/measurement_queue.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::direction
{
namespace
{
template <typename T>
std::string measurement_description(const Measurements<2, T>& m)
{
        return to_string(m.time) + "; true angle = "
               + to_string(radians_to_degrees(normalize_angle(m.true_data.angle + m.true_data.angle_r)));
}

template <typename T>
std::string filter_description(const Filter10<T>& filter)
{
        return "; angle = " + to_string(radians_to_degrees(normalize_angle(filter.angle())));
}

template <typename T, template <typename> typename Filter>
std::string filter_description(const Filter<T>& filter)
{
        return "; angle = " + to_string(radians_to_degrees(normalize_angle(filter.angle())))
               + "; angle speed = " + to_string(radians_to_degrees(normalize_angle(filter.angle_speed())));
}

template <typename T, template <typename> typename F>
class Direction final : public Filter<2, T>
{
        T reset_dt_;
        T angle_estimation_variance_;
        std::optional<T> gate_;
        Init<T> init_;
        T position_process_variance_;
        T angle_process_variance_;
        T fading_memory_alpha_;
        std::unique_ptr<F<T>> filter_;

        com::MeasurementQueue<2, T> queue_;

        std::optional<Nees<T>> nees_;
        std::optional<Nis<T>> nis_;

        std::optional<T> last_time_;
        std::optional<T> last_position_time_;

        void check_time(T time) const;

        void reset(const Measurements<2, T>& m);

        [[nodiscard]] bool update_filter(const Measurements<2, T>& m, const Estimation<2, T>& estimation);

        [[nodiscard]] std::optional<UpdateInfo<2, T>> update(
                const Measurements<2, T>& m,
                const Estimation<2, T>& estimation) override;

        [[nodiscard]] std::string consistency_string() const override;

public:
        Direction(
                std::size_t measurement_queue_size,
                T reset_dt,
                T angle_estimation_variance,
                std::optional<T> gate,
                const Init<T>& init,
                T position_process_variance,
                T angle_process_variance,
                T fading_memory_alpha,
                std::unique_ptr<F<T>>&& filter);
};

template <typename T, template <typename> typename F>
Direction<T, F>::Direction(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T position_process_variance,
        const T angle_process_variance,
        const T fading_memory_alpha,
        std::unique_ptr<F<T>>&& filter)
        : reset_dt_(reset_dt),
          angle_estimation_variance_(angle_estimation_variance),
          gate_(gate),
          init_(init),
          position_process_variance_(position_process_variance),
          angle_process_variance_(angle_process_variance),
          fading_memory_alpha_(fading_memory_alpha),
          filter_(std::move(filter)),
          queue_(measurement_queue_size, reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <typename T, template <typename> typename F>
void Direction<T, F>::check_time(const T time) const
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

template <typename T, template <typename> typename F>
void Direction<T, F>::reset(const Measurements<2, T>& m)
{
        if (!m.position || queue_.empty())
        {
                return;
        }

        ASSERT(queue_.last_time() == m.time);

        queue_.update_filter(
                [&]
                {
                        filter_->reset(queue_.init_position_velocity(), queue_.init_position_velocity_p(), init_);
                },
                [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                {
                        update_position(
                                filter_.get(), position, measurements.direction, measurements.speed, gate_, dt,
                                position_process_variance_, angle_process_variance_, fading_memory_alpha_, nis_);
                });

        last_time_ = m.time;
        last_position_time_ = m.time;
}

template <typename T, template <typename> typename F>
bool Direction<T, F>::update_filter(const Measurements<2, T>& m, const Estimation<2, T>& estimation)
{
        ASSERT(last_time_);
        const T dt = m.time - *last_time_;

        const std::optional<Measurement<1, T>> direction =
                m.direction && estimation.angle_variance_less_than(angle_estimation_variance_)
                        ? m.direction
                        : std::nullopt;

        if (m.position)
        {
                ASSERT(m.position->variance);

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update_position(
                        filter_.get(), position, direction, m.speed, gate_, dt, position_process_variance_,
                        angle_process_variance_, fading_memory_alpha_, nis_);

                LOG(measurement_description(m) + filter_description(*filter_));

                return true;
        }

        if (direction || m.speed)
        {
                update_non_position(
                        filter_.get(), direction, m.speed, gate_, dt, position_process_variance_,
                        angle_process_variance_, fading_memory_alpha_, nis_);
                return true;
        }

        return false;
}

template <typename T, template <typename> typename F>
std::optional<UpdateInfo<2, T>> Direction<T, F>::update(const Measurements<2, T>& m, const Estimation<2, T>& estimation)
{
        if (!((m.position && m.position->variance) || m.direction || m.speed))
        {
                return {};
        }

        check_time(m.time);

        queue_.update(m, estimation);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                reset(m);
                return {};
        }

        if (!m.position && last_position_time_ && !(m.time - *last_position_time_ < reset_dt_))
        {
                return {};
        }

        if (!update_filter(m, estimation))
        {
                return {};
        }

        if (m.position)
        {
                last_position_time_ = m.time;
        }

        last_time_ = m.time;

        update_nees(*filter_, m.true_data, nees_);

        return {
                {.position = filter_->position(),
                 .position_p = filter_->position_p().diagonal(),
                 .speed = filter_->speed(),
                 .speed_p = filter_->speed_p()}
        };
}

template <typename T, template <typename> typename F>
std::string Direction<T, F>::consistency_string() const
{
        return make_consistency_string(nees_, nis_);
}
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_direction_1_0(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const T position_process_variance,
        const T angle_process_variance,
        const T fading_memory_alpha)
{
        return std::make_unique<Direction<T, Filter10>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_process_variance,
                angle_process_variance, fading_memory_alpha, create_filter_1_0<T>(sigma_points_alpha));
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_direction_1_1(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const T position_process_variance,
        const T angle_process_variance,
        const T fading_memory_alpha)
{
        return std::make_unique<Direction<T, Filter11>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_process_variance,
                angle_process_variance, fading_memory_alpha, create_filter_1_1<T>(sigma_points_alpha));
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_direction_2_1(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const T position_process_variance,
        const T angle_process_variance,
        const T fading_memory_alpha)
{
        return std::make_unique<Direction<T, Filter21>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_process_variance,
                angle_process_variance, fading_memory_alpha, create_filter_2_1<T>(sigma_points_alpha));
}

#define TEMPLATE(T)                                                               \
        template std::unique_ptr<Filter<2, T>> create_direction_1_0(              \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, T, T, T); \
        template std::unique_ptr<Filter<2, T>> create_direction_1_1(              \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, T, T, T); \
        template std::unique_ptr<Filter<2, T>> create_direction_2_1(              \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, T, T, T);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
