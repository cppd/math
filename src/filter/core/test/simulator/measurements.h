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

#pragma once

#include <src/filter/core/test/measurements.h>

#include <optional>
#include <vector>

namespace ns::filter::core::test::simulator
{
template <typename T>
struct MeasurementConfig final
{
        T position_reset_interval;
        T reset_min_time;
        T reset_max_time;
        T speed_factor;
};

template <typename T>
class VarianceCorrection final
{
        std::optional<T> last_time_;
        T last_k_{1};

public:
        void correct(Measurements<T>* m);
};

template <typename T>
struct SimulatorMeasurements final
{
        MeasurementConfig<T> config;
        std::vector<Measurements<T>> measurements;
};

template <typename T>
SimulatorMeasurements<T> prepare_measurements(const std::vector<Measurements<T>>& measurements);
}
