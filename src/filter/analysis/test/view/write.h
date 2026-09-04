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

#include <src/com/string/str.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/filter/analysis/allan_deviation.h>
#include <src/filter/analysis/noise_parameters.h>
#include <src/settings/directory.h>

#include <fstream>
#include <iomanip>
#include <ios>
#include <string_view>
#include <vector>

namespace ns::filter::analysis::test::view
{
template <typename T>
void write_to_file(
        const std::vector<AllanDeviation<T>>& deviations,
        const NoiseParameter<T>& bias_instability,
        const NoiseParameter<T>& angle_random_walk,
        const NoiseParameter<T>& rate_random_walk)
{
        constexpr std::string_view DEGREE = "&#x00b0;";

        constexpr int TEXT_PRECISION = 3;
        constexpr int DATA_PRECISION = Limits<T>::max_digits10();

        std::ofstream file(
                settings::test_path("filter_analysis_allan_deviation_" + replace_space(type_name<T>(), '_') + ".txt"));

        file << "[";

        file << "{";
        file << "'name':'Bias Instability'";
        file << std::setprecision(TEXT_PRECISION);
        file << ", 'annotation':'<b>Bias Instability</b><br>";
        file << bias_instability.value * 3600 << DEGREE << "/h'";
        file << std::setprecision(DATA_PRECISION);
        file << ", 'x':" << bias_instability.line.tau;
        file << ", 'y':" << bias_instability.line.deviation;
        file << ", 'log_slope':" << bias_instability.line.log_slope;
        file << "},";

        file << "{";
        file << "'name':'Angle Random Walk'";
        file << std::setprecision(TEXT_PRECISION);
        file << ", 'annotation':'<b>Angle Random Walk</b><br>";
        file << angle_random_walk.value * 60 << DEGREE << "/" << "h<sup>1/2</sup>'";
        file << std::setprecision(DATA_PRECISION);
        file << ", 'x':" << angle_random_walk.line.tau;
        file << ", 'y':" << angle_random_walk.line.deviation;
        file << ", 'log_slope':" << angle_random_walk.line.log_slope;
        file << "},";

        file << "{";
        file << "'name':'Rate Random Walk'";
        file << std::setprecision(TEXT_PRECISION);
        file << ", 'annotation':'<b>Rate Random Walk</b><br>";
        file << rate_random_walk.value * 60 << DEGREE << "/" << "h<sup>1/2</sup>'";
        file << std::setprecision(DATA_PRECISION);
        file << ", 'x':" << rate_random_walk.line.tau;
        file << ", 'y':" << rate_random_walk.line.deviation;
        file << ", 'log_slope':" << rate_random_walk.line.log_slope;
        file << "},";

        file << "]\n";

        file << std::setprecision(DATA_PRECISION);
        file << std::scientific;
        for (const AllanDeviation<T>& ad : deviations)
        {
                file << "(" << ad.tau << ", " << ad.deviation << ")\n";
        }
}
}
