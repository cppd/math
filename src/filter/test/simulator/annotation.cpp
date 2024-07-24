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

#include "config.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ns::filter::test::simulator
{
namespace
{
constexpr std::string_view DEGREE = "&#x00b0;";
constexpr std::string_view SIGMA = "&#x03c3;";

struct MeasurementInfo final
{
        bool position = false;
        bool speed = false;
        bool direction = false;
        bool acceleration = false;
};

template <std::size_t N, typename T>
MeasurementInfo measurement_info(const std::vector<filters::Measurements<N, T>>& measurements)
{
        MeasurementInfo res;

        for (const filters::Measurements<N, T>& m : measurements)
        {
                res.position = res.position || m.position.has_value();
                res.speed = res.speed || m.speed.has_value();
                res.direction = res.direction || m.direction.has_value();
                res.acceleration = res.acceleration || m.acceleration.has_value();
        }

        return res;
}

template <typename T>
void write_update_annotation(const Config<T>& config, const MeasurementInfo& measurement_info, std::ostringstream& oss)
{
        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / (config.measurement_dt * config.measurement_dt_count_position) << " Hz";

        if (measurement_info.speed)
        {
                oss << "<br>";
                oss << "speed: " << 1 / (config.measurement_dt * config.measurement_dt_count_speed) << " Hz";
        }

        if (measurement_info.direction)
        {
                oss << "<br>";
                oss << "direction: " << 1 / (config.measurement_dt * config.measurement_dt_count_direction) << " Hz";
        }

        if (measurement_info.acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << 1 / (config.measurement_dt * config.measurement_dt_count_acceleration)
                    << " Hz";
        }
}

template <typename T>
void write_bias_annotation(const Config<T>& config, const MeasurementInfo& measurement_info, std::ostringstream& oss)
{
        if (!measurement_info.direction && !measurement_info.acceleration)
        {
                return;
        }

        oss << "<br>";
        oss << "<br>";
        oss << "<b>bias</b>";
        oss << "<br>";
        oss << "direction drift: " << radians_to_degrees(config.angle_drift_per_hour) << " " << DEGREE << "/h";
        oss << "<br>";
        oss << "direction angle: " << radians_to_degrees(config.angle_r) << DEGREE;
}

template <typename T>
void write_sigma_annotation(const Config<T>& config, const MeasurementInfo& measurement_info, std::ostringstream& oss)
{
        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "position: " << std::sqrt(config.measurement_variance_position) << " m";

        if (measurement_info.speed)
        {
                oss << "<br>";
                oss << "speed: " << std::sqrt(config.measurement_variance_speed) << " m/s";
        }

        if (measurement_info.direction)
        {
                oss << "<br>";
                oss << "direction: " << radians_to_degrees(std::sqrt(config.measurement_variance_direction)) << DEGREE;
        }

        if (measurement_info.acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << std::sqrt(config.measurement_variance_acceleration) << " m/s<sup>2</sup>";
        }
}
}

template <std::size_t N, typename T>
std::string make_annotation(const Config<T>& config, const std::vector<filters::Measurements<N, T>>& measurements)
{
        const MeasurementInfo info = measurement_info(measurements);

        if (!info.position)
        {
                error("No position measurements");
        }

        std::ostringstream oss;

        write_update_annotation(config, info, oss);
        write_bias_annotation(config, info, oss);
        write_sigma_annotation(config, info, oss);

        return oss.str();
}

#define TEMPLATE(T) \
        template std::string make_annotation(const Config<T>&, const std::vector<filters::Measurements<2, T>>&);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
