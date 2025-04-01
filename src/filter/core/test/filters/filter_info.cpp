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

#include "filter_info.h"

#include "filter.h"
#include "noise_model.h"
#include "utility.h"

#include "info/info.h"

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
class FilterInfoImpl : public Filter<T>
{
        T init_v_;
        T init_v_variance_;
        NoiseModel<T> noise_model_;
        T fading_memory_alpha_;
        T reset_dt_;
        std::optional<T> gate_;
        std::unique_ptr<info::FilterInfo<T>> filter_;

        NormalizedSquared<T> nees_;

        std::optional<T> last_time_;

        void reset() override
        {
                last_time_.reset();
        }

        [[nodiscard]] bool init_update(const Measurements<T>& m)
        {
                if (!m.x)
                {
                        return false;
                }

                last_time_ = m.time;

                const numerical::Vector<2, T> x{0, 0};
                const numerical::Matrix<2, 2, T> i{
                        {0, 0},
                        {0, 0}
                };
                filter_->reset(x, i);

                if (m.v)
                {
                        filter_update(filter_.get(), m, gate_);
                }
                else
                {
                        Measurements<T> mv = m;
                        mv.v = {.value = init_v_, .variance = init_v_variance_};
                        filter_update(filter_.get(), mv, gate_);
                }
                return true;
        }

        [[nodiscard]] std::optional<UpdateInfo<T>> update(const Measurements<T>& m) override
        {
                if (!(m.x || m.v))
                {
                        return std::nullopt;
                }

                std::optional<numerical::Matrix<2, 2, T>> f_predict;
                std::optional<numerical::Vector<2, T>> x_predict;
                std::optional<numerical::Matrix<2, 2, T>> p_predict;

                if (!last_time_ || !(m.time - *last_time_ < reset_dt_))
                {
                        if (!init_update(m))
                        {
                                return std::nullopt;
                        }
                }
                else
                {
                        ASSERT(last_time_);
                        const T dt = m.time - *last_time_;
                        ASSERT(dt >= 0);
                        last_time_ = m.time;
                        f_predict = filter_->predict(dt, noise_model_, fading_memory_alpha_);
                        x_predict = filter_->position_speed();
                        p_predict = filter_->position_speed_p();
                        filter_update(filter_.get(), m, gate_);
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

        [[nodiscard]] const NormalizedSquared<T>& nees() const override
        {
                return nees_;
        }

public:
        FilterInfoImpl(
                const T init_v,
                const T init_v_variance,
                const NoiseModel<T>& noise_model,
                const T fading_memory_alpha,
                const T reset_dt,
                const std::optional<T> gate,
                std::unique_ptr<info::FilterInfo<T>>&& filter)
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
std::unique_ptr<Filter<T>> create_info(
        const T init_v,
        const T init_v_variance,
        const NoiseModel<T>& noise_model,
        const T fading_memory_alpha,
        const T reset_dt,
        const std::optional<T> gate)
{
        return std::make_unique<FilterInfoImpl<T>>(
                init_v, init_v_variance, noise_model, fading_memory_alpha, reset_dt, gate,
                info::create_filter_info<T>());
}

#define INSTANTIATION(T) \
        template std::unique_ptr<Filter<T>> create_info(T, T, const NoiseModel<T>&, T, T, std::optional<T>);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
