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

#include "acceleration.h"

#include "consistency.h"
#include "filter_0.h"
#include "filter_1.h"
#include "filter_ekf.h"
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

namespace ns::filter::filters::acceleration
{
namespace
{
template <typename T>
std::string measurement_description(const Measurements<2, T>& m)
{
        return to_string(m.time) + "; true angle = " + to_string(radians_to_degrees(m.true_data.angle));
}

template <typename T>
std::string filter_description(const Filter0<T>& filter)
{
        return "; angle = " + to_string(radians_to_degrees(normalize_angle(filter.angle())))
               + "; angle r = " + to_string(radians_to_degrees(normalize_angle(filter.angle_r())));
}

template <typename T, template <typename> typename Filter>
std::string filter_description(const Filter<T>& filter)
{
        return "; angle = " + to_string(radians_to_degrees(normalize_angle(filter.angle())))
               + "; angle speed = " + to_string(radians_to_degrees(normalize_angle(filter.angle_speed())))
               + "; angle r = " + to_string(radians_to_degrees(normalize_angle(filter.angle_r())));
}

template <typename T, template <typename> typename F>
class Acceleration final : public Filter<2, T>
{
        T reset_dt_;
        T angle_estimation_variance_;
        std::optional<T> gate_;
        Init<T> init_;
        T position_process_variance_;
        T angle_process_variance_;
        T angle_r_process_variance_;
        std::unique_ptr<F<T>> filter_;

        com::MeasurementQueue<2, T> queue_;

        std::optional<Nees<T>> nees_;
        std::optional<Nis<T>> nis_;

        std::optional<T> last_time_;

        void check_time(T time) const;

        void reset(const Measurements<2, T>& m);

        [[nodiscard]] bool update_filter(const Measurements<2, T>& m);

        [[nodiscard]] std::optional<UpdateInfo<2, T>> update(
                const Measurements<2, T>& m,
                const Estimation<2, T>& estimation) override;

        [[nodiscard]] std::string consistency_string() const override;

public:
        Acceleration(
                std::size_t measurement_queue_size,
                T reset_dt,
                T angle_estimation_variance,
                std::optional<T> gate,
                const Init<T>& init,
                T position_process_variance,
                T angle_process_variance,
                T angle_r_process_variance,
                std::unique_ptr<F<T>>&& filter);
};

template <typename T, template <typename> typename F>
Acceleration<T, F>::Acceleration(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T position_process_variance,
        const T angle_process_variance,
        const T angle_r_process_variance,
        std::unique_ptr<F<T>>&& filter)
        : reset_dt_(reset_dt),
          angle_estimation_variance_(angle_estimation_variance),
          gate_(gate),
          init_(init),
          position_process_variance_(position_process_variance),
          angle_process_variance_(angle_process_variance),
          angle_r_process_variance_(angle_r_process_variance),
          filter_(std::move(filter)),
          queue_(measurement_queue_size, reset_dt, angle_estimation_variance)
{
        ASSERT(filter_);
}

template <typename T, template <typename> typename F>
void Acceleration<T, F>::check_time(const T time) const
{
        if (last_time_ && !(*last_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_time_) + " to " + to_string(time));
        }
}

template <typename T, template <typename> typename F>
void Acceleration<T, F>::reset(const Measurements<2, T>& m)
{
        if (!m.position || queue_.empty())
        {
                return;
        }

        ASSERT(queue_.last_time() == m.time);

        queue_.update_filter(
                [&]()
                {
                        filter_->reset(queue_.init_position_velocity(), queue_.init_position_velocity_p(), init_);
                },
                [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                {
                        update_position(
                                filter_.get(), position, measurements.acceleration, measurements.direction,
                                measurements.speed, gate_, dt, position_process_variance_, angle_process_variance_,
                                angle_r_process_variance_, nis_);
                });

        last_time_ = m.time;
}

template <typename T, template <typename> typename F>
bool Acceleration<T, F>::update_filter(const Measurements<2, T>& m)
{
        ASSERT(last_time_);
        const T dt = m.time - *last_time_;

        if (m.position)
        {
                if (!m.position->variance)
                {
                        return {};
                }

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update_position(
                        filter_.get(), position, m.acceleration, m.direction, m.speed, gate_, dt,
                        position_process_variance_, angle_process_variance_, angle_r_process_variance_, nis_);

                LOG(measurement_description(m) + filter_description(*filter_));

                return true;
        }

        return update_non_position(
                filter_.get(), m.acceleration, m.direction, m.speed, gate_, dt, position_process_variance_,
                angle_process_variance_, angle_r_process_variance_, nis_);
}

template <typename T, template <typename> typename F>
std::optional<UpdateInfo<2, T>> Acceleration<T, F>::update(
        const Measurements<2, T>& m,
        const Estimation<2, T>& estimation)
{
        check_time(m.time);

        queue_.update(m, estimation);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                reset(m);
                return {};
        }

        if (!update_filter(m))
        {
                return {};
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
std::string Acceleration<T, F>::consistency_string() const
{
        return make_consistency_string(nees_, nis_);
}
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_acceleration_0(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const T position_process_variance,
        const T angle_process_variance,
        const T angle_r_process_variance)
{
        return std::make_unique<Acceleration<T, filters::acceleration::Filter0>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_process_variance,
                angle_process_variance, angle_r_process_variance, create_filter_0<T>(sigma_points_alpha));
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_acceleration_1(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const T position_process_variance,
        const T angle_process_variance,
        const T angle_r_process_variance)
{
        return std::make_unique<Acceleration<T, filters::acceleration::Filter1>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_process_variance,
                angle_process_variance, angle_r_process_variance, create_filter_1<T>(sigma_points_alpha));
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_acceleration_ekf(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T position_process_variance,
        const T angle_process_variance,
        const T angle_r_process_variance)
{
        return std::make_unique<Acceleration<T, filters::acceleration::FilterEkf>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_process_variance,
                angle_process_variance, angle_r_process_variance, create_filter_ekf<T>());
}

#define TEMPLATE(T)                                                               \
        template std::unique_ptr<Filter<2, T>> create_acceleration_0(             \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, T, T, T); \
        template std::unique_ptr<Filter<2, T>> create_acceleration_1(             \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, T, T, T); \
        template std::unique_ptr<Filter<2, T>> create_acceleration_ekf(           \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, T, T);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
