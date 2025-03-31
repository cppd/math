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

#include "filters.h"

#include "filter.h"
#include "noise_model.h"

#include "ekf/ekf.h"
#include "ukf/ukf.h"

#include <src/com/error.h>
#include <src/filter/core/consistency.h>
#include <src/filter/core/test/measurements.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <memory>
#include <optional>

namespace ns::filter::core::test::filters
{
namespace
{
template <typename T>
numerical::Vector<2, T> init_x(const Measurements<T>& m, const T init_v)
{
        ASSERT(m.x);
        const T x = m.x->value;
        const T v = m.v ? m.v->value : init_v;
        return {x, v};
}

template <typename T>
numerical::Vector<2, T> init_variance(const Measurements<T>& m, const T init_v_variance)
{
        ASSERT(m.x);
        const T x_variance = m.x->variance;
        const T v_variance = m.v ? m.v->variance : init_v_variance;
        return {x_variance, v_variance};
}

template <typename Filter>
UpdateInfo<typename Filter::Type> filter_reset(
        Filter* const filter,
        const Measurements<typename Filter::Type>& m,
        const typename Filter::Type init_v,
        const typename Filter::Type init_v_variance)
{
        using T = Filter::Type;

        ASSERT(filter);

        const numerical::Vector<2, T> x = init_x(m, init_v);
        const numerical::Vector<2, T> variance = init_variance(m, init_v_variance);

        const numerical::Matrix<2, 2, T> p{
                {variance[0],           0},
                {          0, variance[1]}
        };

        filter->reset(x, p);

        ASSERT(filter->position_speed() == x);
        ASSERT(filter->position_speed_p() == p);

        return {
                .x = x[0],
                .x_stddev = std::sqrt(variance[0]),
                .v = x[1],
                .v_stddev = std::sqrt(variance[1]),
                .f_predict = std::nullopt,
                .x_predict = std::nullopt,
                .p_predict = std::nullopt,
                .x_update = x,
                .p_update = p,
        };
}

template <typename Filter>
std::optional<numerical::Matrix<2, 2, typename Filter::Type>> filter_predict(
        Filter* const filter,
        const typename Filter::Type dt,
        const NoiseModel<typename Filter::Type>& noise_model,
        const typename Filter::Type& fading_memory_alpha)
{
        if constexpr (std::is_same_v<Filter, ukf::FilterUkf<typename Filter::Type>>)
        {
                filter->predict(dt, noise_model, fading_memory_alpha);
                return std::nullopt;
        }
        else
        {
                return filter->predict(dt, noise_model, fading_memory_alpha);
        }
}

template <typename Filter>
bool filter_update(
        Filter* const filter,
        const Measurements<typename Filter::Type>& m,
        const std::optional<typename Filter::Type> gate)
{
        ASSERT(filter);
        ASSERT(m.x || m.v);

        if (m.x)
        {
                if (m.v)
                {
                        filter->update_position_speed(m.x->value, m.x->variance, m.v->value, m.v->variance, gate);
                        return true;
                }
                filter->update_position(m.x->value, m.x->variance, gate);
                return true;
        }
        if (m.v)
        {
                filter->update_speed(m.v->value, m.v->variance, gate);
                return true;
        }
        return false;
}

template <typename F>
class FilterImpl : public Filter<typename F::Type>
{
        F::Type init_v_;
        F::Type init_v_variance_;
        NoiseModel<typename F::Type> noise_model_;
        F::Type fading_memory_alpha_;
        F::Type reset_dt_;
        std::optional<typename F::Type> gate_;
        std::unique_ptr<F> filter_;

        NormalizedSquared<typename F::Type> nees_;

        std::optional<typename F::Type> last_time_;

        void reset() override
        {
                last_time_.reset();
        }

        [[nodiscard]] std::optional<UpdateInfo<typename F::Type>> update(
                const Measurements<typename F::Type>& m) override
        {
                using T = typename F::Type;

                if (!(m.x || m.v))
                {
                        return std::nullopt;
                }

                if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
                {
                        if (!m.x)
                        {
                                return std::nullopt;
                        }
                        last_time_ = m.time;
                        return filter_reset(filter_.get(), m, init_v_, init_v_variance_);
                }

                const T dt = m.time - *last_time_;
                ASSERT(dt >= 0);
                last_time_ = m.time;

                const std::optional<numerical::Matrix<2, 2, T>> f_predict =
                        filter_predict(filter_.get(), dt, noise_model_, fading_memory_alpha_);

                const numerical::Vector<2, T> x_predict = filter_->position_speed();
                const numerical::Matrix<2, 2, T> p_predict = filter_->position_speed_p();

                if (!filter_update(filter_.get(), m, gate_))
                {
                        return std::nullopt;
                }

                nees_.add(
                        numerical::Vector<2, T>(m.true_x, m.true_v) - filter_->position_speed(),
                        filter_->position_speed_p());

                return {
                        {
                         .x = filter_->position(),
                         .x_stddev = std::sqrt(filter_->position_p()),
                         .v = filter_->speed(),
                         .v_stddev = std::sqrt(filter_->speed_p()),
                         .f_predict = f_predict,
                         .x_predict = x_predict,
                         .p_predict = p_predict,
                         .x_update = filter_->position_speed(),
                         .p_update = filter_->position_speed_p(),
                         }
                };
        }

        [[nodiscard]] const NormalizedSquared<typename F::Type>& nees() const override
        {
                return nees_;
        }

public:
        FilterImpl(
                const typename F::Type init_v,
                const typename F::Type init_v_variance,
                const NoiseModel<typename F::Type>& noise_model,
                const typename F::Type fading_memory_alpha,
                const F::Type reset_dt,
                const std::optional<typename F::Type> gate,
                std::unique_ptr<F>&& filter)
                : init_v_(init_v),
                  init_v_variance_(init_v_variance),
                  noise_model_(noise_model),
                  fading_memory_alpha_(fading_memory_alpha),
                  reset_dt_(reset_dt),
                  gate_(gate),
                  filter_(std::move(filter))
        {
        }
};
}

template <typename T>
std::unique_ptr<Filter<T>> create_ekf(
        const T init_v,
        const T init_v_variance,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha,
        const T reset_dt,
        const std::optional<T> gate)
{
        return std::make_unique<FilterImpl<ekf::FilterEkf<T, false>>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate,
                ekf::create_filter_ekf<T, false>());
}

template <typename T>
std::unique_ptr<Filter<T>> create_h_infinity(
        const T init_v,
        const T init_v_variance,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha,
        const T reset_dt,
        const std::optional<T> gate)
{
        return std::make_unique<FilterImpl<ekf::FilterEkf<T, true>>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate,
                ekf::create_filter_ekf<T, true>());
}

template <typename T>
std::unique_ptr<Filter<T>> create_ukf(
        const T init_v,
        const T init_v_variance,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha,
        const T reset_dt,
        const std::optional<T> gate)
{
        return std::make_unique<FilterImpl<ukf::FilterUkf<T>>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate, ukf::create_filter_ukf<T>());
}

#define INSTANTIATION(T)                                                                                           \
        template std::unique_ptr<Filter<T>> create_ekf(T, T, const NoiseModel<T>&, T, T, std::optional<T>);        \
        template std::unique_ptr<Filter<T>> create_h_infinity(T, T, const NoiseModel<T>&, T, T, std::optional<T>); \
        template std::unique_ptr<Filter<T>> create_ukf(T, T, const NoiseModel<T>&, T, T, std::optional<T>);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
