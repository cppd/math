/*
Copyright (C) 2017-2021 Topological Manifold

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

#if 0

#include "conversion.h"

#include <iomanip>
#include <sstream>

namespace ns::color
{
namespace
{
template <typename T>
T srgb_float_to_linear_float(T c)
{
        static_assert(std::is_same_v<T, long double>);

        if (c >= 1)
        {
                return 1;
        }
        if (c >= T(0.04045))
        {
                return std::pow((c + T(0.055)) / T(1.055), T(2.4));
        }
        if (c > 0)
        {
                return c / T(12.92);
        }
        return 0;
}
}

std::string lookup_table_float()
{
        std::ostringstream oss;
        oss << std::setprecision(limits<float>::max_digits10);
        oss << std::scientific;
        oss << "// clang-format off\n";
        oss << "inline constexpr std::array<float, 256> SRGB_UINT8_TO_RGB_FLOAT =\n";
        oss << "{";
        for (unsigned i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i & 0b11) != 0) ? " " : "\n" + std::string(8, ' '));
                const long double srgb_float = i / 255.0L;
                const float linear_float = srgb_float_to_linear_float(srgb_float);
                oss << linear_float;
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}

std::string lookup_table_uint16()
{
        std::ostringstream oss;
        oss << std::setfill(' ');
        oss << "// clang-format off\n";
        oss << "inline constexpr std::array<std::uint16_t, 256> SRGB_UINT8_TO_RGB_UINT16 =\n";
        oss << "{";
        constexpr long double MAX_UINT16 = limits<std::uint16_t>::max();
        for (unsigned i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i % 16) != 0) ? " " : "\n" + std::string(8, ' '));
                const long double srgb_float = i / 255.0L;
                const long double linear_float = srgb_float_to_linear_float(srgb_float);
                const std::uint16_t linear_uint16 = std::lround(linear_float * MAX_UINT16);
                oss << std::setw(5) << linear_uint16;
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
}
#endif
