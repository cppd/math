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

#include "show_file.h"

#include "utility.h"

#include <src/com/type/limit.h>
#include <src/com/type/name.h>

#include <fstream>
#include <iomanip>
#include <string>

namespace ns::filter::test
{
namespace
{
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
}

template <std::size_t N, typename T>
void write_to_file(
        const std::vector<Vector<N, T>>& track_position,
        const std::vector<Vector<N, T>>& track_speed,
        const std::vector<Vector<N, T>>& measurement_angle,
        const std::vector<Vector<N, T>>& measurement_acceleration_x,
        const std::vector<Vector<N, T>>& measurement_acceleration_y,
        const std::vector<std::optional<Vector<N, T>>>& measurement_position,
        const std::vector<std::optional<Vector<N, T>>>& measurement_speed,
        const std::vector<std::optional<Vector<N, T>>>& filter_position,
        const std::vector<std::optional<Vector<N, T>>>& filter_speed,
        const std::vector<Vector<N, T>>& filter_process)
{
        std::ofstream file(test_file_path("filter_2d_" + replace_space(type_name<T>()) + ".txt"));
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::scientific;

        if (!track_position.empty())
        {
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

        if (!track_speed.empty())
        {
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

        if (!measurement_angle.empty())
        {
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

        if (!measurement_acceleration_x.empty())
        {
                file << '{';
                file << R"("name":"Measurement Acceleration X")";
                file << R"(, "mode":"lines+markers")";
                file << R"(, "line_color":"#000000")";
                file << R"(, "line_width":0.25)";
                file << R"(, "line_dash":None)";
                file << R"(, "marker_size":2)";
                file << "}\n";
                for (const auto& v : measurement_acceleration_x)
                {
                        write(file, v);
                }
        }

        if (!measurement_acceleration_y.empty())
        {
                file << '{';
                file << R"("name":"Measurement Acceleration Y")";
                file << R"(, "mode":"lines+markers")";
                file << R"(, "line_color":"#000000")";
                file << R"(, "line_width":0.25)";
                file << R"(, "line_dash":None)";
                file << R"(, "marker_size":2)";
                file << "}\n";
                for (const auto& v : measurement_acceleration_y)
                {
                        write(file, v);
                }
        }

        if (!measurement_position.empty())
        {
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

        if (!measurement_speed.empty())
        {
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

        if (!filter_position.empty())
        {
                file << '{';
                file << R"("name":"Filter Position")";
                file << R"(, "mode":"lines+markers")";
                file << R"(, "line_color":"#a00000")";
                file << R"(, "line_width":0.25)";
                file << R"(, "line_dash":None)";
                file << R"(, "marker_size":2)";
                file << "}\n";
                for (const auto& v : filter_position)
                {
                        write(file, v);
                }
        }

        if (!filter_speed.empty())
        {
                file << '{';
                file << R"("name":"Filter Speed")";
                file << R"(, "mode":"lines+markers")";
                file << R"(, "line_color":"#00a000")";
                file << R"(, "line_width":0.5)";
                file << R"(, "line_dash":None)";
                file << R"(, "marker_size":2)";
                file << "}\n";
                for (const auto& v : filter_speed)
                {
                        write(file, v);
                }
        }

        if (!filter_process.empty())
        {
                file << '{';
                file << R"("name":"Filter Process")";
                file << R"(, "mode":"lines+markers")";
                file << R"(, "line_color":"#00a000")";
                file << R"(, "line_width":0.5)";
                file << R"(, "line_dash":None)";
                file << R"(, "marker_size":2)";
                file << "}\n";
                for (const auto& v : filter_process)
                {
                        write(file, v);
                }
        }
}

#define TEMPLATE(T)                                                                                                   \
        template void write_to_file(                                                                                  \
                const std::vector<Vector<2, T>>&, const std::vector<Vector<2, T>>&, const std::vector<Vector<2, T>>&, \
                const std::vector<Vector<2, T>>&, const std::vector<Vector<2, T>>&,                                   \
                const std::vector<std::optional<Vector<2, T>>>&, const std::vector<std::optional<Vector<2, T>>>&,     \
                const std::vector<std::optional<Vector<2, T>>>&, const std::vector<std::optional<Vector<2, T>>>&,     \
                const std::vector<Vector<2, T>>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
