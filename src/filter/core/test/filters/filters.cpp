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
#include "utility.h"

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
        ASSERT(m.position);
        const T x = m.position->value;
        const T v = m.speed ? m.speed->value : init_v;
        return {x, v};
}

template <typename T>
numerical::Vector<2, T> init_variance(const Measurements<T>& m, const T init_v_variance)
{
        ASSERT(m.position);
        const T x_variance = m.position->variance;
        const T v_variance = m.speed ? m.speed->variance : init_v_variance;
        return {x_variance, v_variance};
}

template <typename Filter>
void filter_reset(
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
}

template <typename Filter>
PredictInfo<typename Filter::Type> filter_predict(
        Filter* const filter,
        const typename Filter::Type dt,
        const NoiseModel<typename Filter::Type>& noise_model,
        const typename Filter::Type& fading_memory_alpha)
{
        if constexpr (std::is_same_v<Filter, ukf::FilterUkf<typename Filter::Type>>)
        {
                filter->predict(dt, noise_model, fading_memory_alpha);
                return {};
        }
        else
        {
                PredictInfo<typename Filter::Type> predict;
                predict.f = filter->predict(dt, noise_model, fading_memory_alpha);
                predict.x = filter->position_speed();
                predict.p = filter->position_speed_p();
                return predict;
        }
}

template <typename F>
class Impl final : public Filter<typename F::Type>
{
        using T = F::Type;

        T init_v_;
        T init_v_variance_;
        NoiseModel<T> noise_model_;
        T fading_memory_alpha_;
        T reset_dt_;
        std::optional<T> gate_;
        std::unique_ptr<F> filter_;

        NormalizedSquared<T> nees_;

        std::optional<T> last_time_;

        void reset() override
        {
                last_time_.reset();
        }

        [[nodiscard]] std::optional<UpdateInfo<T>> update(const Measurements<T>& m) override
        {
                if (!(m.position || m.speed))
                {
                        return std::nullopt;
                }

                if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
                {
                        if (!m.position)
                        {
                                return std::nullopt;
                        }
                        last_time_ = m.time;
                        filter_reset(filter_.get(), m, init_v_, init_v_variance_);
                        return make_update_info({}, *filter_);
                }

                const T dt = m.time - *last_time_;
                ASSERT(dt >= 0);
                last_time_ = m.time;

                const PredictInfo<T> predict = filter_predict(filter_.get(), dt, noise_model_, fading_memory_alpha_);

                if (!filter_update(filter_.get(), m, gate_))
                {
                        return std::nullopt;
                }

                nees_.add(
                        numerical::Vector<2, T>(m.true_position, m.true_speed) - filter_->position_speed(),
                        filter_->position_speed_p());

                return make_update_info(predict, *filter_);
        }

        [[nodiscard]] const NormalizedSquared<T>& nees() const override
        {
                return nees_;
        }

public:
        Impl(std::unique_ptr<F>&& filter,
             const T init_v,
             const T init_v_variance,
             const NoiseModel<T>& noise_model,
             const T fading_memory_alpha,
             const T reset_dt,
             const std::optional<T> gate)
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

template <typename F, typename... T>
auto create(std::unique_ptr<F>&& filter, T&&... ts)
{
        return std::make_unique<Impl<F>>(std::move(filter), std::forward<T>(ts)...);
}
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
        return create(
                ekf::create_filter_ekf<T, false>(), init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt,
                gate);
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
        return create(
                ekf::create_filter_ekf<T, true>(), init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt,
                gate);
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
        return create(
                ukf::create_filter_ukf<T>(), init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate);
}

#define INSTANTIATION(T)                                                                                           \
        template std::unique_ptr<Filter<T>> create_ekf(T, T, const NoiseModel<T>&, T, T, std::optional<T>);        \
        template std::unique_ptr<Filter<T>> create_h_infinity(T, T, const NoiseModel<T>&, T, T, std::optional<T>); \
        template std::unique_ptr<Filter<T>> create_ukf(T, T, const NoiseModel<T>&, T, T, std::optional<T>);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
