/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "measurements.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>
#include <src/filter/core/test/measurements.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <vector>

namespace ns::filter::core::test::simulator
{
namespace
{
template <typename T>
MeasurementConfig<T> measurement_config()
{
        MeasurementConfig<T> res;
        res.position_reset_interval = 2;
        res.reset_min_time = 226;
        res.reset_max_time = res.reset_min_time + 60;
        res.speed_factor = 1;
        return res;
}

template <typename T>
T random_sign(const T v, PCG& engine)
{
        return std::bernoulli_distribution(0.5)(engine) ? v : -v;
}

template <typename T>
std::vector<Measurements<T>> reset_position_measurements(
        std::vector<Measurements<T>> measurements,
        const MeasurementConfig<T>& config)
{
        if (measurements.empty())
        {
                return measurements;
        }

        auto iter = measurements.begin();
        T next_time = iter->time + config.position_reset_interval;

        while (++iter != measurements.end())
        {
                if (iter->time < next_time)
                {
                        iter->position.reset();
                }
                else
                {
                        next_time = iter->time + config.position_reset_interval;
                }

                if (iter->time >= config.reset_min_time && iter->time < config.reset_max_time)
                {
                        iter->position.reset();
                }
        }

        return measurements;
}

template <typename T>
class BadMeasurements final
{
        static constexpr T X = 2000;
        static constexpr T X_AFTER_RESET = 500;
        static constexpr T V = 30;
        static constexpr T PROBABILITY = T{1} / 20;
        static constexpr int COUNT_AFTER_RESET = 2;

        const MeasurementConfig<T>* const config_;

        PCG engine_;
        int count_after_reset_ = 0;

        void update_position(Measurements<T>& m)
        {
                if (!m.position)
                {
                        return;
                }

                if (m.time >= config_->reset_max_time && count_after_reset_ < COUNT_AFTER_RESET)
                {
                        ++count_after_reset_;
                        m.position->value += random_sign(X_AFTER_RESET, engine_);
                }
                else if (std::bernoulli_distribution(PROBABILITY)(engine_))
                {
                        m.position->value += random_sign(X, engine_);
                }
        }

        void update_speed(Measurements<T>& m)
        {
                if (!m.speed)
                {
                        return;
                }

                m.speed->value *= config_->speed_factor;
                if (std::bernoulli_distribution(PROBABILITY)(engine_))
                {
                        m.speed->value += V;
                }
        }

public:
        explicit BadMeasurements(const MeasurementConfig<T>* const config)
                : config_(config)
        {
                ASSERT(config_);
        }

        void update(Measurements<T>& m)
        {
                update_position(m);
                update_speed(m);
        }
};

template <typename T>
std::vector<Measurements<T>> add_bad_measurements(
        const std::vector<Measurements<T>>& measurements,
        const MeasurementConfig<T>& config)
{
        BadMeasurements<T> bm(&config);

        std::vector<Measurements<T>> res(measurements);
        for (Measurements<T>& m : std::ranges::drop_view(res, 5))
        {
                bm.update(m);
        }
        return res;
}

template <typename T>
class VarianceCorrectionImpl final : public VarianceCorrection<T>
{
        std::optional<T> last_time_;
        T last_k_;

        void reset() override
        {
                last_time_.reset();
                last_k_ = 1;
        }

        void correct(Measurements<T>* const m) override
        {
                if (!m->position)
                {
                        return;
                }

                const auto correction = [](const T dt)
                {
                        return std::min(T{30}, 1 + power<3>(dt) / 10'000);
                };

                const T dt = last_time_ ? (m->time - *last_time_) : 0;
                ASSERT(dt >= 0);
                const T k = (dt < 5) ? 1 : correction(dt);
                ASSERT(k >= 1);
                const T res = (last_k_ + k) / 2;
                last_time_ = m->time;
                last_k_ = res;

                m->position->variance *= square(res);
        }

public:
        VarianceCorrectionImpl()
        {
                reset();
        }
};
}

template <typename T>
SimulatorMeasurements<T> prepare_measurements(const std::vector<Measurements<T>>& measurements)
{
        const MeasurementConfig<T> config = measurement_config<T>();

        const std::vector<Measurements<T>> v = reset_position_measurements(measurements, config);

        return {
                .variance_correction = std::make_unique<VarianceCorrectionImpl<T>>(),
                .config = config,
                .measurements = add_bad_measurements(v, config),
        };
}

#define INSTANTIATION(T)                      \
        template class VarianceCorrection<T>; \
        template SimulatorMeasurements<T> prepare_measurements(const std::vector<Measurements<T>>&);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
