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

#include "position.h"

#include "filter_0.h"
#include "filter_1.h"
#include "filter_2.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/filters/position/consistency.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::position
{
namespace
{
template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
class Position final : public FilterPosition<N, T, ORDER>
{
        T reset_dt_;
        T linear_dt_;
        std::optional<T> gate_;
        Init<T> init_;
        NoiseModel<T> noise_model_;
        T fading_memory_alpha_;
        std::unique_ptr<F<N, T>> filter_;

        Nees<T> nees_;
        Nis<T> nis_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void check_time(T time) const;

        [[nodiscard]] UpdateInfoPosition<N, T, ORDER> update_info(
                T time,
                const auto& f_predict,
                const auto& x_predict,
                const auto& p_predict) const;

public:
        Position(
                T reset_dt,
                T linear_dt,
                std::optional<T> gate,
                const Init<T>& init,
                const NoiseModel<T>& noise_model,
                T fading_memory_alpha,
                std::unique_ptr<F<N, T>>&& filter);

        [[nodiscard]] std::optional<UpdateInfoPosition<N, T, ORDER>> update(const Measurements<N, T>& m) override;

        [[nodiscard]] std::string consistency_string() const override;

        [[nodiscard]] bool empty() const override;

        [[nodiscard]] numerical::Vector<N, T> position() const override;
        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const override;
        [[nodiscard]] numerical::Vector<N, T> velocity() const override;
        [[nodiscard]] numerical::Matrix<N, N, T> velocity_p() const override;
        [[nodiscard]] numerical::Vector<2 * N, T> position_velocity() const override;
        [[nodiscard]] numerical::Matrix<2 * N, 2 * N, T> position_velocity_p() const override;
        [[nodiscard]] T speed() const override;
        [[nodiscard]] T speed_p() const override;

        [[nodiscard]] numerical::Vector<N, T> x_to_position(
                const numerical::Vector<N * (1 + ORDER), T>& x) const override;
        [[nodiscard]] numerical::Vector<N, T> p_to_position_p(
                const numerical::Matrix<N * (1 + ORDER), N * (1 + ORDER), T>& p) const override;
        [[nodiscard]] T x_to_speed(const numerical::Vector<N * (1 + ORDER), T>& x) const override;
};

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
Position<N, T, ORDER, F>::Position(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha,
        std::unique_ptr<F<N, T>>&& filter)
        : reset_dt_(reset_dt),
          linear_dt_(linear_dt),
          gate_(gate),
          init_(init),
          noise_model_(noise_model),
          fading_memory_alpha_(fading_memory_alpha),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
void Position<N, T, ORDER, F>::check_time(const T time) const
{
        if (last_predict_time_ && !(*last_predict_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_predict_time_) + " to "
                      + to_string(time));
        }

        if (last_update_time_ && !(*last_update_time_ < time))
        {
                error("Measurement time does not increase; from " + to_string(*last_update_time_) + " to "
                      + to_string(time));
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
UpdateInfoPosition<N, T, ORDER> Position<N, T, ORDER, F>::update_info(
        const T time,
        const auto& f_predict,
        const auto& x_predict,
        const auto& p_predict) const
{
        T speed = 0;
        T speed_p = 0;

        if constexpr (!std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                speed = filter_->speed();
                speed_p = filter_->speed_p();
        }

        return [&]<std::size_t U>(const numerical::Vector<U, T>& x_update, const numerical::Matrix<U, U, T>& p_update)
        {
                const UpdateDetails<U, T> details{
                        .time = time,
                        .f_predict = f_predict,
                        .x_predict = x_predict,
                        .p_predict = p_predict,
                        .x_update = x_update,
                        .p_update = p_update,
                };

                return UpdateInfoPosition<N, T, ORDER>{
                        .position = filter_->position(),
                        .position_p = filter_->position_p().diagonal(),
                        .speed = speed,
                        .speed_p = speed_p,
                        .details = details,
                };
        }(filter_->x(), filter_->p());
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
std::optional<UpdateInfoPosition<N, T, ORDER>> Position<N, T, ORDER, F>::update(const Measurements<N, T>& m)
{
        check_time(m.time);

        if (!m.position || !m.position->variance)
        {
                return {};
        }

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
                {
                        filter_->reset(m.position->value, *m.position->variance);
                }
                else
                {
                        filter_->reset(m.position->value, *m.position->variance, init_);
                }

                last_predict_time_ = m.time;
                last_update_time_ = m.time;

                return update_info(m.time, std::nullopt, std::nullopt, std::nullopt);
        }

        const auto f_predict = filter_->predict(m.time - *last_predict_time_, noise_model_, fading_memory_alpha_);
        const auto x_predict = filter_->x();
        const auto p_predict = filter_->p();
        last_predict_time_ = m.time;

        update_nees(*filter_, m.true_data, nees_);

        const auto update = filter_->update(m.position->value, *m.position->variance, gate_);
        if (update.gate)
        {
                return update_info(m.time, f_predict, x_predict, p_predict);
        }
        const T update_dt = m.time - *last_update_time_;
        last_update_time_ = m.time;

        if (update_dt <= linear_dt_)
        {
                update_nis(update, nis_);
        }

        return update_info(m.time, f_predict, x_predict, p_predict);
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
[[nodiscard]] bool Position<N, T, ORDER, F>::empty() const
{
        return !last_predict_time_ || !last_update_time_;
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Vector<N, T> Position<N, T, ORDER, F>::position() const
{
        return filter_->position();
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Matrix<N, N, T> Position<N, T, ORDER, F>::position_p() const
{
        return filter_->position_p();
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Vector<N, T> Position<N, T, ORDER, F>::velocity() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("velocity is not supported");
        }
        else
        {
                return filter_->velocity();
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Matrix<N, N, T> Position<N, T, ORDER, F>::velocity_p() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("velocity_p is not supported");
        }
        else
        {
                return filter_->velocity_p();
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
numerical::Vector<2 * N, T> Position<N, T, ORDER, F>::position_velocity() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("position_velocity is not supported");
        }
        else
        {
                return filter_->position_velocity();
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
numerical::Matrix<2 * N, 2 * N, T> Position<N, T, ORDER, F>::position_velocity_p() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("position_velocity_p is not supported");
        }
        else
        {
                return filter_->position_velocity_p();
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
T Position<N, T, ORDER, F>::speed() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("speed is not supported");
        }
        else
        {
                return filter_->speed();
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
T Position<N, T, ORDER, F>::speed_p() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("speed_p is not supported");
        }
        else
        {
                return filter_->speed_p();
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
std::string Position<N, T, ORDER, F>::consistency_string() const
{
        return make_consistency_string(nees_, nis_);
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
numerical::Vector<N, T> Position<N, T, ORDER, F>::x_to_position(const numerical::Vector<N * (1 + ORDER), T>& x) const
{
        return filter_->x_to_position(x);
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
numerical::Vector<N, T> Position<N, T, ORDER, F>::p_to_position_p(
        const numerical::Matrix<N * (1 + ORDER), N * (1 + ORDER), T>& p) const
{
        return filter_->p_to_position_p(p);
}

template <std::size_t N, typename T, std::size_t ORDER, template <std::size_t, typename> typename F>
T Position<N, T, ORDER, F>::x_to_speed(const numerical::Vector<N * (1 + ORDER), T>& x) const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                error("speed is not supported");
        }
        else
        {
                return filter_->x_to_speed(x);
        }
}
}

template <std::size_t N, typename T>
std::unique_ptr<FilterPosition<N, T, 0>> create_position_0(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T theta,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha)
{
        return std::make_unique<Position<N, T, 0, Filter0>>(
                reset_dt, linear_dt, gate, init, noise_model, fading_memory_alpha, create_filter_0<N, T>(theta));
}

template <std::size_t N, typename T>
std::unique_ptr<FilterPosition<N, T, 1>> create_position_1(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T theta,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha)
{
        return std::make_unique<Position<N, T, 1, Filter1>>(
                reset_dt, linear_dt, gate, init, noise_model, fading_memory_alpha, create_filter_1<N, T>(theta));
}

template <std::size_t N, typename T>
std::unique_ptr<FilterPosition<N, T, 2>> create_position_2(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T theta,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha)
{
        return std::make_unique<Position<N, T, 2, Filter2>>(
                reset_dt, linear_dt, gate, init, noise_model, fading_memory_alpha, create_filter_2<N, T>(theta));
}

#define TEMPLATE(N, T)                                                               \
        template std::unique_ptr<FilterPosition<(N), T, 0>> create_position_0(       \
                T, T, std::optional<T>, const Init<T>&, T, const NoiseModel<T>&, T); \
        template std::unique_ptr<FilterPosition<(N), T, 1>> create_position_1(       \
                T, T, std::optional<T>, const Init<T>&, T, const NoiseModel<T>&, T); \
        template std::unique_ptr<FilterPosition<(N), T, 2>> create_position_2(       \
                T, T, std::optional<T>, const Init<T>&, T, const NoiseModel<T>&, T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
