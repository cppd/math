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

#include <src/com/constant.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/filter/utility/allan_deviation.h>
#include <src/filter/utility/files.h>
#include <src/test/test.h>

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <ios>
#include <random>
#include <string_view>
#include <vector>

namespace ns::filter::utility::test
{
namespace
{
constexpr std::string_view DEGREE = "&#x00b0;";

template <typename T>
void save_to_file(
        const std::vector<AllanDeviation<T>> deviations,
        const BiasInstability<T>& bi,
        const AngleRandomWalk<T>& arw,
        const RateRandomWalk<T>& rrw)
{
        constexpr int TEXT_PRECISION = 3;
        constexpr int DATA_PRECISION = Limits<T>::max_digits10();

        std::ofstream file(test_file_path("filter_utility_allan_deviation_" + replace_space(type_name<T>()) + ".txt"));

        file << "[";

        file << "{";
        file << "'name':'Bias Instability'";
        file << std::setprecision(TEXT_PRECISION);
        file << ", 'annotation':'<b>Bias Instability</b><br>";
        file << bi.bias_instability * 3600 << DEGREE << "/h'";
        file << std::setprecision(DATA_PRECISION);
        file << ", 'x':" << bi.tau;
        file << ", 'y':" << bi.deviation;
        file << ", 'log_slope':" << T{0};
        file << "},";

        file << "{";
        file << "'name':'Angle Random Walk'";
        file << std::setprecision(TEXT_PRECISION);
        file << ", 'annotation':'<b>Angle Random Walk</b><br>";
        file << arw.angle_random_walk * 60 << DEGREE << "/" << "h<sup>1/2</sup>'";
        file << std::setprecision(DATA_PRECISION);
        file << ", 'x':" << arw.tau;
        file << ", 'y':" << arw.deviation;
        file << ", 'log_slope':" << T{-0.5};
        file << "},";

        file << "{";
        file << "'name':'Rate Random Walk'";
        file << std::setprecision(TEXT_PRECISION);
        file << ", 'annotation':'<b>Rate Random Walk</b><br>";
        file << rrw.rate_random_walk * 60 << DEGREE << "/" << "h<sup>1/2</sup>'";
        file << std::setprecision(DATA_PRECISION);
        file << ", 'x':" << rrw.tau;
        file << ", 'y':" << rrw.deviation;
        file << ", 'log_slope':" << T{0.5};
        file << "},";

        file << "]\n";

        file << std::setprecision(DATA_PRECISION);
        file << std::scientific;
        for (const AllanDeviation<T>& ad : deviations)
        {
                file << "(" << ad.tau << ", " << ad.deviation << ")\n";
        }
}

template <typename T>
void check(const BiasInstability<T>& bi, const AngleRandomWalk<T>& arw, const RateRandomWalk<T>& rrw)
{
        if (!(bi.bias_instability > T{0.073} && bi.bias_instability < T{0.092}))
        {
                error("Bias instability (" + to_string(bi.bias_instability) + ") is out of range");
        }

        if (!(arw.angle_random_walk > T{0.092} && arw.angle_random_walk < T{0.12}))
        {
                error("Angle random walk (" + to_string(arw.angle_random_walk) + ") is out of range");
        }

        if (!(rrw.rate_random_walk > T{0.034} && rrw.rate_random_walk < T{0.041}))
        {
                error("Rate random walk (" + to_string(rrw.rate_random_walk) + ") is out of range");
        }
}

template <typename T>
void test_impl()
{
        const std::size_t count = 100'000;
        const T frequency = 100;
        const T bias_interval = 500;
        const std::size_t output_count = 500;

        const std::vector<T> data = [&]
        {
                PCG engine;
                std::normal_distribution<T> nd;
                std::vector<T> res(count);
                T sum = 0;
                for (std::size_t i = 0; i < count; ++i)
                {
                        const T bias = std::sin(i * (2 * PI<T> / (frequency * bias_interval)));
                        const T speed = bias + nd(engine);
                        sum += speed / frequency;
                        res[i] = sum;
                }
                return res;
        }();

        const std::vector<AllanDeviation<T>> deviations = allan_deviation(data, frequency, output_count);

        const BiasInstability<T> bi = bias_instability(deviations);
        const AngleRandomWalk<T> arw = angle_random_walk(deviations);
        const RateRandomWalk<T> rrw = rate_random_walk(deviations);

        save_to_file(deviations, bi, arw, rrw);

        check(bi, arw, rrw);
}

void test()
{
        LOG("Test Allan deviation");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Allan deviation passed");
}

TEST_SMALL("Allan Deviation", test)
}
}
