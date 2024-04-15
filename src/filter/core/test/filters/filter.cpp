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
#include "ukf.h"

#include <src/com/error.h>
#include <src/filter/core/consistency.h>
#include <src/filter/core/test/measurements.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

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

template <typename Filter>
bool update_filter(Filter* const filter, const Measurements<typename Filter::Type>& m)
{
        ASSERT(filter);
        ASSERT(m.x || m.v);

        if (m.x)
        {
                if (m.v)
                {
                        filter->update_position_speed(m.x->value, m.x->variance, m.v->value, m.v->variance);
                        return true;
                }
                filter->update_position(m.x->value, m.x->variance);
                return true;
        }
        if (m.v)
        {
                filter->update_speed(m.v->value, m.v->variance);
                return true;
        }
        return false;
}

template <typename F>
class FilterImpl : public Filter<typename F::Type>
{
        F::Type init_v_;
        F::Type init_v_variance_;
        F::Type process_variance_;
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

                if (!last_time_)
                {
                        if (!m.x)
                        {
                                return std::nullopt;
                        }

                        last_time_ = m.time;

                        reset_filter(filter_.get(), m, init_v_, init_v_variance_);

                        return {
                                {.x = filter_->position(), .stddev = std::sqrt(filter_->position_p())}
                        };
                }

                const T dt = m.time - *last_time_;
                ASSERT(dt >= 0);
                last_time_ = m.time;

                filter_->predict(dt, process_variance_);

                if (!update_filter(filter_.get(), m))
                {
                        return std::nullopt;
                }

                nees_.add(
                        numerical::Vector<2, T>(m.true_x, m.true_v) - filter_->position_speed(),
                        filter_->position_speed_p());

                return {
                        {.x = filter_->position(), .stddev = std::sqrt(filter_->position_p())}
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
                const typename F::Type process_variance,
                std::unique_ptr<F>&& filter)
                : init_v_(init_v),
                  init_v_variance_(init_v_variance),
                  process_variance_(process_variance),
                  filter_(std::move(filter))
        {
        }
};
}

template <typename T>
std::unique_ptr<Filter<T>> create_ekf(const T init_v, const T init_v_variance, const T process_variance)
{
        return std::make_unique<FilterImpl<filters::FilterEkf<T, false>>>(
                init_v, init_v_variance, process_variance, filters::create_filter_ekf<T, false>());
}

template <typename T>
std::unique_ptr<Filter<T>> create_h_infinity(const T init_v, const T init_v_variance, const T process_variance)
{
        return std::make_unique<FilterImpl<filters::FilterEkf<T, true>>>(
                init_v, init_v_variance, process_variance, filters::create_filter_ekf<T, true>());
}

template <typename T>
std::unique_ptr<Filter<T>> create_ukf(const T init_v, const T init_v_variance, const T process_variance)
{
        return std::make_unique<FilterImpl<filters::FilterUkf<T>>>(
                init_v, init_v_variance, process_variance, filters::create_filter_ukf<T>());
}

#define INSTANTIATION(T)                                                \
        template std::unique_ptr<Filter<T>> create_ekf(T, T, T);        \
        template std::unique_ptr<Filter<T>> create_h_infinity(T, T, T); \
        template std::unique_ptr<Filter<T>> create_ukf(T, T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
