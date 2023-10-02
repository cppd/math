/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "write.h"

#include "converters.h"

#include "../utility/utility.h"

#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>

#include <fstream>
#include <iomanip>
#include <string>

namespace ns::filter::test::view
{
namespace
{
template <std::size_t N, typename T>
constexpr Vector<N, T> OFFSET = []()
{
        Vector<N, T> res(0);
        res[N - 1] = 1000;
        return res;
}();

std::string color_to_string(const color::RGB8 color)
{
        return "\"rgb(" + to_string(color.red()) + "," + to_string(color.green()) + "," + to_string(color.blue())
               + ")\"";
}

template <std::size_t N, typename T>
void write(std::ostream& os, const Vector<N, T>& v)
{
        static_assert(N > 0);
        os << '(';
        os << v[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                os << ", " << v[i];
        }
        os << ")\n";
}

template <std::size_t N, typename T>
void write(std::ostream& os, const std::optional<Vector<N, T>>& v)
{
        static_assert(N > 0);
        if (v)
        {
                write(os, *v);
                return;
        }
        os << "(None";
        for (std::size_t i = 1; i < N; ++i)
        {
                os << ", None";
        }
        os << ")\n";
}

template <std::size_t N, typename T>
void write_track_position(std::ostream& file, const std::vector<Vector<N, T>>& track_position)
{
        if (track_position.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":"Track Position")";
        file << R"(, "mode":"lines")";
        file << R"(, "line_color":"#0000ff")";
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":"dot")";
        file << R"(, "marker_size":None)";
        file << "}\n";
        for (const auto& v : track_position)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_track_speed(std::ostream& file, const std::vector<Vector<N, T>>& track_speed)
{
        if (track_speed.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":"Track Speed")";
        file << R"(, "mode":"lines")";
        file << R"(, "line_color":"#0000ff")";
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":"dot")";
        file << R"(, "marker_size":None)";
        file << "}\n";
        for (const auto& v : track_speed)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_measurement_angle(std::ostream& file, const std::vector<std::optional<Vector<N, T>>>& measurement_angle)
{
        if (measurement_angle.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":"Measurement Angle")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : measurement_angle)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_measurement_acceleration(
        std::ostream& file,
        const std::string_view name,
        const std::vector<std::optional<Vector<N, T>>>& measurement_acceleration)
{
        if (measurement_acceleration.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":"Measurement Acceleration)" << ' ' << name << '"';
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : measurement_acceleration)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_measurement_position(
        std::ostream& file,
        const std::vector<std::optional<Vector<N, T>>>& measurement_position)
{
        if (measurement_position.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":"Measurement Position")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : measurement_position)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_measurement_speed(std::ostream& file, const std::vector<std::optional<Vector<N, T>>>& measurement_speed)
{
        if (measurement_speed.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":"Measurement Speed")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : measurement_speed)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_filter_speed(
        std::ostream& file,
        const std::string& name,
        const color::RGB8 color,
        const std::vector<std::optional<Vector<N, T>>>& speed)
{
        if (speed.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":")" << name << " Speed\"";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(color);
        file << R"(, "line_width":0.5)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : speed)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_filter_speed_p(
        std::ostream& file,
        const std::string& name,
        const color::RGB8 color,
        const std::vector<std::optional<Vector<N, T>>>& speed_p)
{
        if (speed_p.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":")" << name << " Speed P\"";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(color);
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":1)";
        file << "}\n";
        for (const auto& v : speed_p)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_filter_position(
        std::ostream& file,
        const std::string& name,
        const color::RGB8 color,
        const std::vector<std::optional<Vector<N, T>>>& position)
{
        if (position.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":")" << name << " Position\"";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(color);
        file << R"(, "line_width":0.5)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : position)
        {
                write(file, v);
        }
}

template <std::size_t N, typename T>
void write_filter_position_p(
        std::ostream& file,
        const std::string_view filter_name,
        const std::string_view name,
        const color::RGB8 color,
        const std::vector<std::optional<Vector<N, T>>>& position_p)
{
        if (position_p.empty())
        {
                return;
        }

        file << '{';
        file << R"("name":")" << filter_name << " Position P " << name << '"';
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(color);
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":1)";
        file << "}\n";
        for (const auto& v : position_p)
        {
                write(file, v);
        }
}
}

template <std::size_t N, typename T>
void write_to_file(
        const std::string_view annotation,
        const std::vector<Measurements<N, T>>& measurements,
        const T interval,
        const std::vector<Filter<N, T>>& filters)
{
        std::ofstream file(utility::test_file_path("filter_2d_" + utility::replace_space(type_name<T>()) + ".txt"));
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::scientific;

        if (!annotation.empty())
        {
                file << '"' << annotation << "\"\n";
        }

        write_track_position(file, add_offset(track_position(measurements), OFFSET<N, T>));

        write_track_speed(file, track_speed(measurements));

        write_measurement_angle(file, angle_measurements(measurements, interval));

        write_measurement_acceleration(file, "X", acceleration_measurements<0>(measurements, interval));

        write_measurement_acceleration(file, "Y", acceleration_measurements<1>(measurements, interval));

        write_measurement_position(file, add_offset(position_measurements(measurements, interval), OFFSET<N, T>));

        write_measurement_speed(file, speed_measurements(measurements, interval));

        for (const Filter<N, T>& filter : filters)
        {
                write_filter_speed(
                        file, filter.name, filter.color, convert_speed(optional_value(filter.speed, interval)));

                write_filter_speed_p(
                        file, filter.name, filter.color, convert_speed_p(optional_value(filter.speed_p, interval)));

                write_filter_position(
                        file, filter.name, filter.color,
                        add_offset(convert_position(optional_value(filter.position, interval)), OFFSET<N, T>));

                write_filter_position_p(
                        file, filter.name, "X", filter.color,
                        convert_position_p<0>(optional_value(filter.position_p, interval)));

                write_filter_position_p(
                        file, filter.name, "Y", filter.color,
                        convert_position_p<1>(optional_value(filter.position_p, interval)));
        }
}

#define TEMPLATE(T)                  \
        template void write_to_file( \
                std::string_view, const std::vector<Measurements<2, T>>&, T, const std::vector<Filter<2, T>>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
