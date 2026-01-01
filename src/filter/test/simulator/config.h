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

#pragma once

#include <src/com/conversion.h>
#include <src/com/exponent.h>

#include <cstddef>

namespace ns::filter::test::simulator
{
template <typename T>
struct Config final
{
        std::size_t count = 8000;

        T speed_min = kph_to_mps(-30.0);
        T speed_max = kph_to_mps(130.0);
        T speed_clamp_min = kph_to_mps(0.0);
        T speed_clamp_max = kph_to_mps(100.0);
        T speed_variance = square(0.1);
        T velocity_magnitude_period = 110;

        T angle = degrees_to_radians(-170.0);
        T angle_drift_per_hour = degrees_to_radians(-360.0);
        T angle_r = degrees_to_radians(30.0);

        T measurement_dt = 0.1L;
        unsigned measurement_dt_count_acceleration = 1;
        unsigned measurement_dt_count_direction = 1;
        unsigned measurement_dt_count_position = 10;
        unsigned measurement_dt_count_speed = 10;

        T measurement_variance_acceleration = square(1.0);
        T measurement_variance_direction = square(degrees_to_radians(2.0));
        T measurement_variance_position = square(25.0);
        T measurement_variance_speed = square(0.2);

        T bad_measurement_position = 1000;
        T bad_measurement_position_probability = T{0} / 20;
};
}
