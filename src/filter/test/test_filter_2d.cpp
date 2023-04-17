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

#include "simulator.h"

#include "../filter.h"
#include "../models.h"

#include <src/com/exponent.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <optional>
#include <vector>

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

template <std::size_t N, typename T>
void write_to_file(const std::string& file_name, const Track<N, T>& track, const std::vector<Vector<N, T>>& filter)
{
        std::ofstream file(file_path(file_name));
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
        for (const auto& v : track.positions)
        {
                write(file, v);
        }

        file << '{';
        file << R"("name":"Measurements")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#000000")";
        file << R"(, "line_width":0.25)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";
        for (const auto& [_, v] : std::map{track.position_measurements.cbegin(), track.position_measurements.cend()})
        {
                write(file, v);
        }

        file << '{';
        file << R"("name":"Filter")";
        file << R"(, "mode":"lines+markers")";
        file << R"(, "line_color":"#008000")";
        file << R"(, "line_width":1)";
        file << R"(, "line_dash":None)";
        file << R"(, "marker_size":4)";
        file << "}\n";
        for (const auto& v : filter)
        {
                write(file, v);
        }
}

template <typename T>
void test_impl()
{
        constexpr std::size_t N = 4;
        constexpr std::size_t M = 2;

        constexpr T DT = 1;
        constexpr T TRACK_VELOCITY_MEAN = 1;
        constexpr T TRACK_VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T PROCESS_VARIANCE = power<2>(0.1);

        constexpr T VELOCITY_MEASUREMENT_VARIANCE = power<2>(0.2);
        constexpr T POSITION_MEASUREMENT_VARIANCE = power<2>(3);

        constexpr Vector<N, T> X(10, 5, 10, 5);
        constexpr Matrix<N, N, T> P{
                {500,  0,   0,  0},
                {  0, 50,   0,  0},
                {  0,  0, 500,  0},
                {  0,  0,   0, 50}
        };
        constexpr Matrix<N, N, T> F{
                {1, DT, 0,  0},
                {0,  1, 0,  0},
                {0,  0, 1, DT},
                {0,  0, 0,  1}
        };
        constexpr Matrix<N, N, T> Q = []()
        {
                const auto m = discrete_white_noise<N / 2, T>(DT, PROCESS_VARIANCE);
                return block_diagonal(std::array{m, m});
        }();

        constexpr Matrix<M, N, T> H_POSITION{
                {1, 0, 0, 0},
                {0, 0, 1, 0}
        };
        constexpr Matrix<M, M, T> R_POSITION{
                {POSITION_MEASUREMENT_VARIANCE,                             0},
                {                            0, POSITION_MEASUREMENT_VARIANCE}
        };

        constexpr Matrix<M, N, T> H_VELOCITY{
                {0, 1, 0, 0},
                {0, 0, 0, 1}
        };
        constexpr Matrix<M, M, T> R_VELOCITY{
                {VELOCITY_MEASUREMENT_VARIANCE,                             0},
                {                            0, VELOCITY_MEASUREMENT_VARIANCE}
        };

        constexpr std::size_t COUNT = 1000;
        constexpr std::array<std::size_t, 2> POSITION_OUTAGE = {350, 400};
        constexpr std::size_t POSITION_INTERVAL = 5;
        const Track track = generate_track<2, T>(
                COUNT, DT, TRACK_VELOCITY_MEAN, TRACK_VELOCITY_VARIANCE, VELOCITY_MEASUREMENT_VARIANCE,
                POSITION_MEASUREMENT_VARIANCE, POSITION_OUTAGE, POSITION_INTERVAL);

        Filter<N, M, T> filter;
        filter.set_x(X);
        filter.set_p(P);
        filter.set_f(F);
        filter.set_q(Q);

        std::vector<Vector<2, T>> result;
        result.reserve(COUNT);
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                filter.predict();

                if (const auto iter = track.position_measurements.find(i);
                    iter != track.position_measurements.cend() && iter->second)
                {
                        filter.set_h(H_POSITION);
                        filter.set_r(R_POSITION);
                        filter.update(*iter->second);
                }
                else
                {
                        filter.set_h(H_VELOCITY);
                        filter.set_r(R_VELOCITY);
                        filter.update(track.velocity_measurements[i]);
                }

                result.push_back({filter.x()[0], filter.x()[2]});
        }

        write_to_file("filter_2d_" + replace_space(type_name<T>()) + ".txt", track, result);
}

void test()
{
        LOG("Test Filter 2D");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Filter 2D passed");
}

TEST_SMALL("Filter 2D", test)
}
}
