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

#include <src/com/file/path.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/settings/directory.h>

#include <cctype>
#include <fstream>
#include <string>

namespace ns::filter::test
{
namespace
{
std::string replace_space(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += !std::isspace(static_cast<unsigned char>(c)) ? c : '_';
        }
        return res;
}

std::filesystem::path file_path(const std::string_view name)
{
        return settings::test_directory() / path_from_utf8(name);
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
}

template <std::size_t N, typename T>
void write_to_file(
        const std::vector<Vector<N, T>>& positions,
        const std::vector<Vector<N, T>>& angle_measurements,
        const std::vector<std::optional<Vector<N, T>>>& position_measurements,
        const std::vector<std::optional<Vector<N, T>>>& position_filter,
        const std::vector<Vector<N, T>>& filter)
{
        std::ofstream file(file_path("filter_2d_" + replace_space(type_name<T>()) + ".txt"));
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::scientific;

        file << '{';
        file << R"("name":"Track")";
        file << R"(, "mode":"lines")";
        file << R"(, "line_color":"#0000ff")";
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":"dot")";
        file << R"(, "marker_size":None)";
        file << "}\n";
        for (const auto& v : positions)
        {
                write(file, v);
        }

        file << '{';
        file << R"("name":"Angle Measurements")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : angle_measurements)
        {
                write(file, v);
        }

        file << '{';
        file << R"("name":"Position Measurements")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : position_measurements)
        {
                write(file, v);
        }

        file << '{';
        file << R"("name":"Position Filter")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#a00000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : position_filter)
        {
                write(file, v);
        }

        file << '{';
        file << R"("name":"Filter")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#00a000")";
        file << R"(, "line_width":0.5)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":2)";
        file << "}\n";
        for (const auto& v : filter)
        {
                write(file, v);
        }
}

#define TEMPLATE(T)                                                                                               \
        template void write_to_file(                                                                              \
                const std::vector<Vector<2, T>>&, const std::vector<Vector<2, T>>&,                               \
                const std::vector<std::optional<Vector<2, T>>>&, const std::vector<std::optional<Vector<2, T>>>&, \
                const std::vector<Vector<2, T>>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
