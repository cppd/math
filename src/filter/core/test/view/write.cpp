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

#include "write.h"

#include <src/color/rgb8.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/filter/core/test/measurements.h>
#include <src/filter/utility/files.h>

#include <fstream>
#include <iomanip>
#include <ios>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ns::filter::core::test::view
{
namespace
{
std::string color_to_string(const color::RGB8 color)
{
        return "\"rgb(" + to_string(color.red()) + "," + to_string(color.green()) + "," + to_string(color.blue())
               + ")\"";
}

template <typename T>
std::unordered_map<T, Measurements<T>> measurement_time_map(const std::vector<Measurements<T>>& measurements)
{
        std::unordered_map<T, Measurements<T>> res;
        for (const Measurements<T>& m : measurements)
        {
                res[m.time] = m;
        }
        return res;
}

template <typename T>
const Measurements<T>& measurements_at_time(const std::unordered_map<T, Measurements<T>>& map, const T time)
{
        const auto iter = map.find(time);
        if (iter != map.cend())
        {
                return iter->second;
        }
        error("Failed to find measurements at time " + to_string(time));
}

template <typename T>
void write_measurements(std::ofstream& file, const std::vector<Measurements<T>>& measurements)
{
        file << '{';
        file << R"("name":"Track")";
        file << R"(, "mode":"lines")";
        file << R"(, "line_color":"#0000ff")";
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":"dot")";
        file << R"(, "marker_size":None)";
        file << "}\n";
        for (const Measurements<T>& m : measurements)
        {
                file << "(" << m.time << ", " << m.true_x << ")\n";
        }

        file << '{';
        file << R"("name":"Measurements")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";
        for (const Measurements<T>& m : measurements)
        {
                file << "(" << m.time << ", " << m.x << ")\n";
        }
}

template <typename T>
void write_filter(std::ofstream& file, const std::unordered_map<T, Measurements<T>> time_map, const Filter<T>& filter)
{
        file << '{';
        file << R"("name":")" << filter.name << "\"";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(filter.color);
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";
        for (const Point<T>& f : filter.points)
        {
                file << "(" << f.time << ", " << f.x << ")\n";
        }

        file << '{';
        file << R"s("name":")s" << filter.name << " P\"";
        file << R"s(, "mode":"lines")s";
        file << R"s(, "line_color":"rgba(128,128,0,0.5)")s";
        file << R"s(, "fill_color":"rgba(180,180,0,0.15)")s";
        file << R"s(, "line_width":1)s";
        file << R"s(, "line_dash":"dot")s";
        file << R"s(, "marker_size":None)s";
        file << "}\n";
        for (const Point<T>& f : filter.points)
        {
                const T true_x = measurements_at_time(time_map, f.time).true_x;
                file << "(" << f.time << ", " << true_x << ", " << f.stddev << ")\n";
        }
}

template <typename T>
void write_filters(
        std::ofstream& file,
        const std::vector<Measurements<T>>& measurements,
        const std::vector<Filter<T>>& filters)
{
        const std::unordered_map<T, Measurements<T>> time_map = measurement_time_map(measurements);
        for (const Filter<T>& filter : filters)
        {
                write_filter(file, time_map, filter);
        }
}
}

template <typename T>
void write(
        const std::string_view& name,
        const std::vector<Measurements<T>>& measurements,
        const std::vector<Filter<T>>& filters)
{
        std::ofstream file(utility::test_file_path(
                "filter_1d_" + utility::replace_space(to_lower(name)) + "_" + utility::replace_space(type_name<T>())
                + ".txt"));
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::scientific;

        write_measurements(file, measurements);

        write_filters(file, measurements, filters);
}

#define INSTANTIATION(T)     \
        template void write( \
                const std::string_view&, const std::vector<Measurements<T>>&, const std::vector<Filter<T>>&);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
