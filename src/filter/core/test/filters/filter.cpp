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

#include "filter.h"

#include "ekf.h"
#include "noise_model.h"
#include "ukf.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
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
template <typename Filter>
void reset_filter(
        Filter* const filter,
        const Measurements<typename Filter::Type>& m,
        const typename Filter::Type init_v,
        const typename Filter::Type init_v_variance)
{
        using T = Filter::Type;

        ASSERT(filter);
        ASSERT(m.x);

        const numerical::Vector<2, T> x(m.x->value, m.v ? m.v->value : init_v);

        const numerical::Matrix<2, 2, T> p{
                {m.x->variance,                                     0},
                {            0, m.v ? m.v->variance : init_v_variance}
        };

        filter->reset(x, p);
}

template <typename T>
class VarianceCorrection final
{
        std::optional<T> last_time_;
        T last_k_{1};

        [[nodiscard]] static T correction(const T dt)
        {
                return std::min(T{30}, 1 + power<3>(dt) / 10'000);
        }

public:
        [[nodiscard]] T update(const T time)
        {
                const T dt = time - last_time_.value_or(time);
                ASSERT(dt >= 0);
                const T k = (dt < 5) ? 1 : correction(dt);
                ASSERT(k >= 1);
                const T res = (last_k_ + k) / 2;
                last_time_ = time;
                last_k_ = res;
                return square(res);
        }
};

template <typename Filter>
bool update_filter(
        Filter* const filter,
        VarianceCorrection<typename Filter::Type>* const variance_correction,
        const Measurements<typename Filter::Type>& m,
        const std::optional<typename Filter::Type> gate)
{
        ASSERT(filter);
        ASSERT(variance_correction);
        ASSERT(m.x || m.v);

        if (m.x)
        {
                const auto xv = m.x->variance * variance_correction->update(m.time);
                if (m.v)
                {
                        filter->update_position_speed(m.x->value, xv, m.v->value, m.v->variance, gate);
                        return true;
                }
                filter->update_position(m.x->value, xv, gate);
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

        std::optional<VarianceCorrection<typename F::Type>> variance_correction_;

        NormalizedSquared<typename F::Type> nees_;

        std::optional<typename F::Type> last_time_;

        void reset() override
        {
                last_time_.reset();
                variance_correction_.reset();
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

                        reset_filter(filter_.get(), m, init_v_, init_v_variance_);
                        variance_correction_.emplace();

                        return {
                                {.x = filter_->position(),
                                 .x_stddev = std::sqrt(filter_->position_p()),
                                 .v = filter_->speed(),
                                 .v_stddev = std::sqrt(filter_->speed_p())}
                        };
                }

                const T dt = m.time - *last_time_;
                ASSERT(dt >= 0);
                last_time_ = m.time;

                filter_->predict(dt, noise_model_, fading_memory_alpha_);

                ASSERT(variance_correction_);
                if (!update_filter(filter_.get(), &*variance_correction_, m, gate_))
                {
                        return std::nullopt;
                }

                nees_.add(
                        numerical::Vector<2, T>(m.true_x, m.true_v) - filter_->position_speed(),
                        filter_->position_speed_p());

                return {
                        {.x = filter_->position(),
                         .x_stddev = std::sqrt(filter_->position_p()),
                         .v = filter_->speed(),
                         .v_stddev = std::sqrt(filter_->speed_p())}
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
        return std::make_unique<FilterImpl<filters::FilterEkf<T, false>>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate,
                filters::create_filter_ekf<T, false>());
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
        return std::make_unique<FilterImpl<filters::FilterEkf<T, true>>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate,
                filters::create_filter_ekf<T, true>());
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
        return std::make_unique<FilterImpl<filters::FilterUkf<T>>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate,
                filters::create_filter_ukf<T>());
}

#define INSTANTIATION(T)                                                                                           \
        template std::unique_ptr<Filter<T>> create_ekf(T, T, const NoiseModel<T>&, T, T, std::optional<T>);        \
        template std::unique_ptr<Filter<T>> create_h_infinity(T, T, const NoiseModel<T>&, T, T, std::optional<T>); \
        template std::unique_ptr<Filter<T>> create_ukf(T, T, const NoiseModel<T>&, T, T, std::optional<T>);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
