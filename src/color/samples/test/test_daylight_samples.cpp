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

#include <src/color/samples/daylight_samples.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ns::color::samples
{
namespace
{
class Exception final : public std::exception
{
        std::string text_;

public:
        explicit Exception(std::string&& text)
                : text_(std::move(text))
        {
        }

        [[nodiscard]] const char* what() const noexcept override
        {
                return text_.c_str();
        }
};

void check_parameters(const int min, const int max, const unsigned count)
{
        if (!(min >= DAYLIGHT_SAMPLES_MIN_WAVELENGTH))
        {
                error("Error min " + to_string(min));
        }

        if (!(max <= DAYLIGHT_SAMPLES_MAX_WAVELENGTH))
        {
                error("Error max " + to_string(max));
        }

        if (!(max > min))
        {
                error("Error min " + to_string(min) + " and max " + to_string(max));
        }

        if (!(count > 0))
        {
                error("Error count " + to_string(count));
        }
}

template <typename T>
[[nodiscard]] std::optional<T> compare(const T d65, const T daylight)
{
        if (!(d65 >= 0))
        {
                error("D65 " + to_string(d65) + " is not positive and not zero");
        }

        if (!(daylight >= 0))
        {
                error("Daylight " + to_string(daylight) + " is not positive and not zero");
        }

        if (d65 == daylight)
        {
                return std::nullopt;
        }

        const T abs = std::abs(d65 - daylight);
        if (!(abs < T{0.014}))
        {
                throw Exception(
                        "D65 " + to_string(d65) + " and daylight " + to_string(daylight)
                        + " are not equal, absolute error " + to_string(abs));
        }

        const T rel = abs / std::max(std::abs(d65), std::abs(daylight));
        if (!(rel < T{3.5e-4}))
        {
                throw Exception(
                        "D65 " + to_string(d65) + " and daylight " + to_string(daylight)
                        + " are not equal, relative error " + to_string(rel));
        }

        return abs;
}

void compare_d65(const double cct, const int min, const int max, const unsigned count)
{
        check_parameters(min, max, count);

        const std::vector<double> d65 = daylight_d65_samples(min, max, count);
        if (!(d65.size() == count))
        {
                error("D65 sample count " + to_string(d65.size()) + " is not equal to " + to_string(count));
        }

        const std::vector<double> daylight = daylight_samples(cct, min, max, count);
        if (!(daylight.size() == count))
        {
                error("Daylight sample count " + to_string(daylight.size()) + " is not equal to " + to_string(count));
        }

        double abs_sum = 0;
        for (std::size_t i = 0; i < d65.size(); ++i)
        {
                const auto abs = compare(d65[i], daylight[i]);
                if (abs)
                {
                        abs_sum += *abs;
                }
        }

        if (!(abs_sum / count < 5.7e-3))
        {
                throw Exception("Mean absolute error " + to_string(abs_sum / count) + " is too large");
        }
}

void check_equal_to_d65(const double cct, const int min, const int max, const unsigned count)
{
        try
        {
                compare_d65(cct, min, max, count);
        }
        catch (const Exception& e)
        {
                error(e.what());
        }
}

void check_not_equal_to_d65(const double cct, const int min, const int max, const unsigned count)
{
        try
        {
                compare_d65(cct, min, max, count);
        }
        catch (const Exception&)
        {
                return;
        }
        error("Samples CCT " + to_string(cct) + " are equal to D65");
}

void test()
{
        LOG("Test daylight samples");

        constexpr int MIN = DAYLIGHT_SAMPLES_MIN_WAVELENGTH;
        constexpr int MAX = DAYLIGHT_SAMPLES_MAX_WAVELENGTH;
        constexpr double D65_CCT = 6503.5;
        {
                static_assert(MAX - MIN > 5);
                const int count = std::lround((MAX - MIN) / 5);
                check_equal_to_d65(D65_CCT, MIN, MAX, count);
        }
        {
                constexpr int COUNT = 64;
                check_equal_to_d65(D65_CCT, MIN, MAX, COUNT);
        }
        {
                constexpr int COUNT = 64;
                check_not_equal_to_d65(5000, MIN, MAX, COUNT);
                check_not_equal_to_d65(6500, MIN, MAX, COUNT);
                check_not_equal_to_d65(6510, MIN, MAX, COUNT);
                check_not_equal_to_d65(8000, MIN, MAX, COUNT);
        }

        LOG("Test daylight samples passed");
}

TEST_SMALL("Daylight Samples", test)
}
}
