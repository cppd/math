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

#include "position.h"

#include "filter_0.h"
#include "filter_1.h"
#include "filter_2.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/position/consistency.h>
#include <src/filter/utility/instantiation.h>
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
template <std::size_t N, typename T, template <std::size_t, typename> typename F>
class Position final : public FilterPosition<N, T>
{
        T reset_dt_;
        T linear_dt_;
        std::optional<T> gate_;
        Init<T> init_;
        T process_variance_;
        T fading_memory_alpha_;
        std::unique_ptr<F<N, T>> filter_;

        Nees<T> nees_;
        Nis<T> nis_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void check_time(T time) const;

        std::optional<UpdateInfo<N, T>> update_info() const;

public:
        Position(
                T reset_dt,
                T linear_dt,
                std::optional<T> gate,
                const Init<T>& init,
                T process_variance,
                T fading_memory_alpha,
                std::unique_ptr<F<N, T>>&& filter);

        [[nodiscard]] std::optional<UpdateInfo<N, T>> update(const Measurements<N, T>& m) override;
        [[nodiscard]] std::optional<UpdateInfo<N, T>> predict(const Measurements<N, T>& m) override;
        [[nodiscard]] std::string consistency_string() const override;

        [[nodiscard]] bool empty() const override;

        [[nodiscard]] numerical::Vector<N, T> position() const override;
        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const override;
        [[nodiscard]] numerical::Vector<N, T> velocity() const override;
        [[nodiscard]] numerical::Matrix<N, N, T> velocity_p() const override;
        [[nodiscard]] numerical::Vector<2 * N, T> position_velocity() const override;
        [[nodiscard]] numerical::Matrix<2 * N, 2 * N, T> position_velocity_p() const override;
};

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
Position<N, T, F>::Position(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T process_variance,
        const T fading_memory_alpha,
        std::unique_ptr<F<N, T>>&& filter)
        : reset_dt_(reset_dt),
          linear_dt_(linear_dt),
          gate_(gate),
          init_(init),
          process_variance_(process_variance),
          fading_memory_alpha_(fading_memory_alpha),
          filter_(std::move(filter))
{
        ASSERT(filter_);
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
void Position<N, T, F>::check_time(const T time) const
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
std::optional<UpdateInfo<N, T>> Position<N, T, F>::update_info() const
{
        if constexpr (std::is_same_v<F<N, T>, Filter0<N, T>>)
        {
                return {
                        {.position = filter_->position(),
                         .position_p = filter_->position_p().diagonal(),
                         .speed = 0,
                         .speed_p = 0}
                };
        }
        else
        {
                return {
                        {.position = filter_->position(),
                         .position_p = filter_->position_p().diagonal(),
                         .speed = filter_->speed(),
                         .speed_p = filter_->speed_p()}
                };
        }
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
std::optional<UpdateInfo<N, T>> Position<N, T, F>::update(const Measurements<N, T>& m)
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

                return update_info();
        }

        filter_->predict(m.time - *last_predict_time_, process_variance_, fading_memory_alpha_);
        last_predict_time_ = m.time;

        update_nees(*filter_, m.true_data, nees_);

        const auto update = filter_->update(m.position->value, *m.position->variance, gate_);
        if (update.gate)
        {
                return update_info();
        }
        const T update_dt = m.time - *last_update_time_;
        last_update_time_ = m.time;

        if (update_dt <= linear_dt_)
        {
                update_nis(update, nis_);
        }

        return update_info();
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
std::optional<UpdateInfo<N, T>> Position<N, T, F>::predict(const Measurements<N, T>& m)
{
        if (m.position)
        {
                error("Predict with position");
        }

        check_time(m.time);

        if (!last_predict_time_ || !last_update_time_ || !(m.time - *last_update_time_ < reset_dt_))
        {
                return {};
        }

        filter_->predict(m.time - *last_predict_time_, process_variance_, fading_memory_alpha_);
        last_predict_time_ = m.time;

        update_nees(*filter_, m.true_data, nees_);

        return update_info();
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
[[nodiscard]] bool Position<N, T, F>::empty() const
{
        return !last_predict_time_ || !last_update_time_;
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Vector<N, T> Position<N, T, F>::position() const
{
        return filter_->position();
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Matrix<N, N, T> Position<N, T, F>::position_p() const
{
        return filter_->position_p();
}

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Vector<N, T> Position<N, T, F>::velocity() const
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
[[nodiscard]] numerical::Matrix<N, N, T> Position<N, T, F>::velocity_p() const
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
numerical::Vector<2 * N, T> Position<N, T, F>::position_velocity() const
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
numerical::Matrix<2 * N, 2 * N, T> Position<N, T, F>::position_velocity_p() const
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

template <std::size_t N, typename T, template <std::size_t, typename> typename F>
std::string Position<N, T, F>::consistency_string() const
{
        return make_consistency_string(nees_, nis_);
}
}

template <std::size_t N, typename T>
std::unique_ptr<FilterPosition<N, T>> create_position_0(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T theta,
        const T process_variance,
        const T fading_memory_alpha)
{
        return std::make_unique<Position<N, T, Filter0>>(
                reset_dt, linear_dt, gate, init, process_variance, fading_memory_alpha, create_filter_0<N, T>(theta));
}

template <std::size_t N, typename T>
std::unique_ptr<FilterPosition<N, T>> create_position_1(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T theta,
        const T process_variance,
        const T fading_memory_alpha)
{
        return std::make_unique<Position<N, T, Filter1>>(
                reset_dt, linear_dt, gate, init, process_variance, fading_memory_alpha, create_filter_1<N, T>(theta));
}

template <std::size_t N, typename T>
std::unique_ptr<FilterPosition<N, T>> create_position_2(
        const T reset_dt,
        const T linear_dt,
        const std::optional<T> gate,
        const Init<T>& init,
        const T theta,
        const T process_variance,
        const T fading_memory_alpha)
{
        return std::make_unique<Position<N, T, Filter2>>(
                reset_dt, linear_dt, gate, init, process_variance, fading_memory_alpha, create_filter_2<N, T>(theta));
}

#define TEMPLATE(N, T)                                                      \
        template std::unique_ptr<FilterPosition<(N), T>> create_position_0( \
                T, T, std::optional<T>, const Init<T>&, T, T, T);           \
        template std::unique_ptr<FilterPosition<(N), T>> create_position_1( \
                T, T, std::optional<T>, const Init<T>&, T, T, T);           \
        template std::unique_ptr<FilterPosition<(N), T>> create_position_2( \
                T, T, std::optional<T>, const Init<T>&, T, T, T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
