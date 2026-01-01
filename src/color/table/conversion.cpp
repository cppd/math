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

#include "conversion.h"

#include <src/color/conversion.h>
#include <src/com/type/limit.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

namespace ns::color::table
{
std::string conversion_lookup_table_float()
{
        const std::string new_line = '\n' + std::string(8, ' ');

        std::ostringstream oss;
        oss << std::setprecision(Limits<float>::max_digits10());
        oss << std::scientific;

        oss << "// clang-format off\n";
        oss << "inline constexpr std::array<float, 256> SRGB_UINT8_TO_RGB_FLOAT =\n";
        oss << "{";
        for (int i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i % 4) != 0) ? " " : new_line);
                const long double srgb_float = i / 255.0L;
                const long double linear_float = srgb_float_to_linear_float(srgb_float);
                oss << static_cast<float>(linear_float);
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";

        return oss.str();
}

std::string conversion_lookup_table_uint16()
{
        constexpr long double MAX_UINT16 = Limits<std::uint16_t>::max();
        const std::string new_line = '\n' + std::string(8, ' ');

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        oss << "// clang-format off\n";
        oss << "inline constexpr std::array<std::uint16_t, 256> SRGB_UINT8_TO_RGB_UINT16 =\n";
        oss << "{";
        for (int i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i % 8) != 0) ? " " : new_line);
                const long double srgb_float = i / 255.0L;
                const long double linear_float = srgb_float_to_linear_float(srgb_float);
                const std::uint16_t linear_uint16 = std::lround(linear_float * MAX_UINT16);
                oss << "0x" << std::setw(4) << linear_uint16;
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";

        return oss.str();
}
}
