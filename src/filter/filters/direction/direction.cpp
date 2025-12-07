/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/filter/filters/com/measurement_queue.h>
#include <src/filter/filters/estimation.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::direction
{
namespace
{
template <typename T>
struct Standing final
{
        static constexpr T SPEED_LIMIT{0.1};
        static constexpr T VELOCITY_MAGNITUDE = 0.001;
        static constexpr numerical::Vector<2, T> VELOCITY_DEFAULT{0.001, 0.001};
        static constexpr numerical::Vector<2, T> VELOCITY_VARIANCE{square(0.1), square(0.1)};
        static constexpr T FADING_MEMORY_ALPHA{1};
        static constexpr DiscreteNoiseModel<T> NOISE_MODEL_POSITION{.variance{0}};
        static constexpr DiscreteNoiseModel<T> NOISE_MODEL_ANGLE{.variance{0}};
};

template <typename T>
std::string measurement_description(const Measurements<2, T>& m)
{
        return to_string(m.time)
               + "; true angle = " + to_string(radians_to_degrees(wrap_angle(m.true_data.angle + m.true_data.angle_r)));
}

template <typename T>
std::string filter_description(const Filter10<T>& filter)
{
        return "; angle = " + to_string(radians_to_degrees(wrap_angle(filter.angle())));
}

template <typename T, template <typename> typename Filter>
std::string filter_description(const Filter<T>& filter)
{
        return "; angle = " + to_string(radians_to_degrees(wrap_angle(filter.angle())))
               + "; angle speed = " + to_string(radians_to_degrees(wrap_angle(filter.angle_speed())));
}

template <typename T, template <typename> typename F>
class Direction final : public Filter<2, T>
{
        T reset_dt_;
        std::optional<T> gate_;
        Init<T> init_;
        NoiseModel<T> position_noise_model_;
        NoiseModel<T> angle_noise_model_;
        T fading_memory_alpha_;
        std::unique_ptr<F<T>> filter_;

        com::MeasurementQueue<2, T> queue_;

        Nees<T> nees_;
        Nis<T> nis_;

        std::optional<T> last_time_;
        std::optional<T> last_speed_;

        std::optional<numerical::Vector<2, T>> standing_velocity_;
        bool standing_{false};

        void check_time(T time) const;

        void update_standing(const Measurements<2, T>& m);
        void update_standing_velocity();

        void reset();

        void update_filter(const Measurements<2, T>& m);

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
                const NoiseModel<T>& position_noise_model,
                const NoiseModel<T>& angle_noise_model,
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
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha,
        std::unique_ptr<F<T>>&& filter)
        : reset_dt_(reset_dt),
          gate_(gate),
          init_(init),
          position_noise_model_(position_noise_model),
          angle_noise_model_(angle_noise_model),
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
}

template <typename T, template <typename> typename F>
void Direction<T, F>::update_standing(const Measurements<2, T>& m)
{
        if (!m.speed)
        {
                return;
        }

        const T speed = m.speed->value[0];
        if (last_speed_)
        {
                standing_ = (*last_speed_ < Standing<T>::SPEED_LIMIT) && (speed < Standing<T>::SPEED_LIMIT);
        }
        last_speed_ = speed;
}

template <typename T, template <typename> typename F>
void Direction<T, F>::update_standing_velocity()
{
        if (standing_)
        {
                if (!standing_velocity_)
                {
                        standing_velocity_ = Standing<T>::VELOCITY_MAGNITUDE * filter_->velocity().normalized();
                        if (*standing_velocity_ == numerical::Vector<2, T>(0, 0) || !is_finite(*standing_velocity_))
                        {
                                standing_velocity_ = Standing<T>::VELOCITY_DEFAULT;
                        }
                }
        }
        else
        {
                standing_velocity_.reset();
        }
}

template <typename T, template <typename> typename F>
void Direction<T, F>::reset()
{
        queue_.update_filter(
                [&]
                {
                        filter_->reset(queue_.init_position_velocity(), queue_.init_position_velocity_p(), init_);
                },
                [&](const Measurement<2, T>& position, const Measurements<2, T>& measurements, const T dt)
                {
                        update_position(
                                filter_.get(), position, measurements.direction, measurements.speed, gate_, dt,
                                position_noise_model_, angle_noise_model_, fading_memory_alpha_, nis_);
                });
}

template <typename T, template <typename> typename F>
void Direction<T, F>::update_filter(const Measurements<2, T>& m)
{
        ASSERT(last_time_);
        const T dt = m.time - *last_time_;

        if (standing_)
        {
                ASSERT(standing_velocity_);
                update_velocity<T>(
                        filter_.get(), {.value = *standing_velocity_, .variance = Standing<T>::VELOCITY_VARIANCE},
                        gate_, dt, Standing<T>::NOISE_MODEL_POSITION, Standing<T>::NOISE_MODEL_ANGLE,
                        Standing<T>::FADING_MEMORY_ALPHA, nis_);

                return;
        }

        if (m.position)
        {
                ASSERT(m.position->variance);

                const Measurement<2, T> position = {.value = m.position->value, .variance = *m.position->variance};

                update_position(
                        filter_.get(), position, m.direction, m.speed, gate_, dt, position_noise_model_,
                        angle_noise_model_, fading_memory_alpha_, nis_);

                LOG(measurement_description(m) + filter_description(*filter_));

                return;
        }

        ASSERT(m.direction || m.speed);

        update_non_position(
                filter_.get(), m.direction, m.speed, gate_, dt, position_noise_model_, angle_noise_model_,
                fading_memory_alpha_, nis_);
}

template <typename T, template <typename> typename F>
std::optional<UpdateInfo<2, T>> Direction<T, F>::update(const Measurements<2, T>& m, const Estimation<2, T>& estimation)
{
        if (!((m.position && m.position->variance) || m.direction || m.speed))
        {
                return {};
        }

        check_time(m.time);

        update_standing(m);

        queue_.update(m, estimation);

        if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
        {
                if (!(m.position && m.position->variance))
                {
                        return {};
                }
                if (!queue_.empty())
                {
                        ASSERT(queue_.last_time() == m.time);
                        reset();
                        last_time_ = m.time;
                }
                return {
                        {.position = estimation.position(),
                         .position_p = estimation.position_p().diagonal(),
                         .speed = estimation.speed(),
                         .speed_p = estimation.speed_p()}
                };
        }

        update_standing_velocity();

        update_filter(m);

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
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha)
{
        return std::make_unique<Direction<T, Filter10>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_noise_model,
                angle_noise_model, fading_memory_alpha, create_filter_1_0<T>(sigma_points_alpha));
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_direction_1_1(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha)
{
        return std::make_unique<Direction<T, Filter11>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_noise_model,
                angle_noise_model, fading_memory_alpha, create_filter_1_1<T>(sigma_points_alpha));
}

template <typename T>
std::unique_ptr<Filter<2, T>> create_direction_2_1(
        const std::size_t measurement_queue_size,
        const T reset_dt,
        const T angle_estimation_variance,
        const std::optional<T> gate,
        const Init<T>& init,
        const T sigma_points_alpha,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha)
{
        return std::make_unique<Direction<T, Filter21>>(
                measurement_queue_size, reset_dt, angle_estimation_variance, gate, init, position_noise_model,
                angle_noise_model, fading_memory_alpha, create_filter_2_1<T>(sigma_points_alpha));
}

#define TEMPLATE(T)                                                                                                 \
        template std::unique_ptr<Filter<2, T>> create_direction_1_0(                                                \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, const NoiseModel<T>&, const NoiseModel<T>&, \
                T);                                                                                                 \
        template std::unique_ptr<Filter<2, T>> create_direction_1_1(                                                \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, const NoiseModel<T>&, const NoiseModel<T>&, \
                T);                                                                                                 \
        template std::unique_ptr<Filter<2, T>> create_direction_2_1(                                                \
                std::size_t, T, T, std::optional<T>, const Init<T>&, T, const NoiseModel<T>&, const NoiseModel<T>&, \
                T);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
