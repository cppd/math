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

#include "write.h"

#include <src/color/rgb8.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/filter/core/test/measurements.h>
#include <src/settings/directory.h>

#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ns::filter::core::test::view
{
namespace
{
constexpr std::string_view SIGMA = "&#x03c3;";

template <typename T>
T to_kph(const T speed)
{
        return 3.6 * speed;
}

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
void write_track_position(std::ofstream& file, const std::vector<Measurements<T>>& measurements)
{
        file << '{';
        file << R"("name":"<b>Track, p<b>")";
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
}

template <typename T>
void write_track_speed(std::ofstream& file, const std::vector<Measurements<T>>& measurements)
{
        file << '{';
        file << R"("name":"Track, v")";
        file << R"(, "mode":"lines")";
        file << R"(, "line_color":"#0000ff")";
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":"dot")";
        file << R"(, "marker_size":None)";
        file << "}\n";
        for (const Measurements<T>& m : measurements)
        {
                file << "(" << m.time << ", " << to_kph(m.true_v) << ")\n";
        }
}

template <typename T>
void write_track(std::ofstream& file, const std::vector<Measurements<T>>& measurements)
{
        write_track_position(file, measurements);
        write_track_speed(file, measurements);
}

template <typename T>
void write_measurement_position(std::ofstream& file, const std::vector<Measurements<T>>& measurements, const T interval)
{
        file << '{';
        file << R"("name":"<b>Measurements, p<b>")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";

        std::optional<T> last_time;
        for (const Measurements<T>& m : measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.x)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        file << "(None, None)\n";
                }
                last_time = m.time;
                file << "(" << m.time << ", " << m.x->value << ")\n";
        }
}

template <typename T>
void write_measurement_position_sigma(
        std::ofstream& file,
        const std::vector<Measurements<T>>& measurements,
        const T interval)
{
        file << '{';
        file << R"s("name":"Measurements, p )s" << SIGMA << "\"";
        file << R"s(, "mode":"lines")s";
        file << R"s(, "line_color":"rgba(128,128,128,0.5)")s";
        file << R"s(, "fill_color":"rgba(180,180,180,0.15)")s";
        file << R"s(, "line_width":1)s";
        file << R"s(, "line_dash":"dot")s";
        file << R"s(, "marker_size":None)s";
        file << "}\n";

        std::optional<T> last_time;
        for (const Measurements<T>& m : measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.x)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        file << "(None, None, None)\n";
                }
                last_time = m.time;
                file << "(" << m.time << ", " << m.true_x << ", " << std::sqrt(m.x->variance) << ")\n";
        }
}

template <typename T>
void write_measurement_speed(std::ofstream& file, const std::vector<Measurements<T>>& measurements, const T interval)
{
        file << '{';
        file << R"("name":"Measurements, v")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";

        std::optional<T> last_time;
        for (const Measurements<T>& m : measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.v)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        file << "(None, None)\n";
                }
                last_time = m.time;
                file << "(" << m.time << ", " << to_kph(m.v->value) << ")\n";
        }
}

template <typename T>
void write_measurements(std::ofstream& file, const std::vector<Measurements<T>>& measurements, const T interval)
{
        write_measurement_position(file, measurements, interval);
        write_measurement_position_sigma(file, measurements, interval);
        write_measurement_speed(file, measurements, interval);
}

template <typename T>
void write_filter_position(std::ofstream& file, const Filter<T>& filter, const T interval)
{
        file << '{';
        file << R"("name":"<b>)" << filter.name << ", p<b>\"";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(filter.color);
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";

        std::optional<T> last_time;
        for (const Point<T>& f : filter.points)
        {
                ASSERT(!last_time || *last_time < f.time);
                if (last_time && f.time > *last_time + interval)
                {
                        file << "(None, None)\n";
                }
                last_time = f.time;
                file << "(" << f.time << ", " << f.x << ")\n";
        }
}

template <typename T>
void write_filter_position_sigma(
        std::ofstream& file,
        const std::unordered_map<T, Measurements<T>>& time_map,
        const Filter<T>& filter,
        const T interval)
{
        file << '{';
        file << R"s("name":")s" << filter.name << ", p " << SIGMA << "\"";
        file << R"s(, "mode":"lines")s";
        file << R"s(, "line_color":"rgba(128,128,0,0.5)")s";
        file << R"s(, "fill_color":"rgba(180,180,0,0.15)")s";
        file << R"s(, "line_width":1)s";
        file << R"s(, "line_dash":"dot")s";
        file << R"s(, "marker_size":None)s";
        file << "}\n";

        std::optional<T> last_time;
        for (const Point<T>& f : filter.points)
        {
                ASSERT(!last_time || *last_time < f.time);
                if (last_time && f.time > *last_time + interval)
                {
                        file << "(None, None, None)\n";
                }
                last_time = f.time;
                const T true_x = measurements_at_time(time_map, f.time).true_x;
                file << "(" << f.time << ", " << true_x << ", " << f.x_stddev << ")\n";
        }
}

template <typename T>
void write_filter_speed(std::ofstream& file, const Filter<T>& filter, const T interval)
{
        file << '{';
        file << R"("name":")" << filter.name << ", v\"";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":)" << color_to_string(filter.color);
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";

        std::optional<T> last_time;
        for (const Point<T>& f : filter.points)
        {
                ASSERT(!last_time || *last_time < f.time);
                if (last_time && f.time > *last_time + interval)
                {
                        file << "(None, None)\n";
                }
                last_time = f.time;
                file << "(" << f.time << ", " << to_kph(f.v) << ")\n";
        }
}

template <typename T>
void write_filters(
        std::ofstream& file,
        const std::vector<Measurements<T>>& measurements,
        const std::vector<Filter<T>>& filters,
        const T interval)
{
        const std::unordered_map<T, Measurements<T>> time_map = measurement_time_map(measurements);
        for (const Filter<T>& filter : filters)
        {
                write_filter_position(file, filter, interval);
                write_filter_position_sigma(file, time_map, filter, interval);
                write_filter_speed(file, filter, interval);
        }
}
}

template <typename T>
void write(
        const std::string_view name,
        const std::string_view annotation,
        const std::vector<Measurements<T>>& measurements,
        const T interval,
        const std::vector<Filter<T>>& filters)
{
        std::ofstream file(settings::test_path(
                "filter_1d_" + replace_space(to_lower(name), '_') + "_" + replace_space(type_name<T>(), '_') + ".txt"));
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::scientific;

        if (!annotation.empty())
        {
                file << '"' << annotation << "\"\n";
        }

        write_track(file, measurements);

        write_measurements(file, measurements, interval);

        write_filters(file, measurements, filters, interval);
}

#define INSTANTIATION(T)                                                                    \
        template void write(                                                                \
                std::string_view, std::string_view, const std::vector<Measurements<T>>&, T, \
                const std::vector<Filter<T>>&);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
