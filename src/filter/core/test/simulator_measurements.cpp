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

#include "simulator_measurements.h"

#include "measurements.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>

#include <algorithm>
#include <random>
#include <ranges>
#include <vector>

namespace ns::filter::core::test
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
        const std::vector<Measurements<T>>& measurements,
        const MeasurementConfig<T>& config)
{
        if (measurements.empty())
        {
                return {};
        }

        std::vector<Measurements<T>> res(measurements);
        auto iter = res.begin();
        T next_time = iter->time + config.position_reset_interval;

        while (++iter != res.end())
        {
                if (iter->time < next_time)
                {
                        iter->x.reset();
                }
                else
                {
                        next_time = iter->time + config.position_reset_interval;
                }

                if (iter->time >= config.reset_min_time && iter->time < config.reset_max_time)
                {
                        iter->x.reset();
                }
        }

        return res;
}

template <typename T>
std::vector<Measurements<T>> add_bad_measurements(
        const std::vector<Measurements<T>>& measurements,
        const MeasurementConfig<T>& config)
{
        constexpr T X = 2000;
        constexpr T X_AFTER_RESET = 500;
        constexpr T V = 30;
        constexpr T PROBABILITY = T{1} / 20;
        constexpr int COUNT_AFTER_RESET = 2;

        PCG engine;

        int count_after_reset = 0;

        const auto x = [&](Measurements<T>& m)
        {
                if (!m.x)
                {
                        return;
                }
                if (m.time >= config.reset_max_time && count_after_reset < COUNT_AFTER_RESET)
                {
                        ++count_after_reset;
                        m.x->value += random_sign(X_AFTER_RESET, engine);
                }
                else if (std::bernoulli_distribution(PROBABILITY)(engine))
                {
                        m.x->value += random_sign(X, engine);
                }
        };

        const auto v = [&](Measurements<T>& m)
        {
                if (!m.v)
                {
                        return;
                }
                m.v->value *= config.speed_factor;
                if (std::bernoulli_distribution(PROBABILITY)(engine))
                {
                        m.v->value += V;
                }
        };

        std::vector<Measurements<T>> res(measurements);
        for (Measurements<T>& m : std::ranges::drop_view(res, 5))
        {
                x(m);
                v(m);
        }
        return res;
}
}

template <typename T>
void VarianceCorrection<T>::correct(Measurements<T>* const m)
{
        if (!m->x)
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

        m->x->variance *= square(res);
}

template <typename T>
SimulatorMeasurements<T> prepare_measurements(const std::vector<Measurements<T>>& measurements)
{
        const MeasurementConfig<T> config = measurement_config<T>();
        const std::vector<Measurements<T>> v = reset_position_measurements(measurements, config);
        return {.config = config, .measurements = add_bad_measurements(v, config)};
}

#define INSTANTIATION(T)                      \
        template class VarianceCorrection<T>; \
        template SimulatorMeasurements<T> prepare_measurements(const std::vector<Measurements<T>>&);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}