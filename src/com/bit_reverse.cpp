/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "bit_reverse.h"

#if 0
#include <iomanip>
#include <sstream>

namespace ns
{
std::string lookup_table_bit_reverse()
{
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        oss << "// clang-format off\n";
        oss << "inline constexpr std::array<std::uint8_t, 256> BIT_REVERSE_TABLE =\n";
        oss << "{";
        for (int i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i & 0b111) != 0) ? " " : "\n" + std::string(8, ' '));
                oss << "0x" << std::setw(2) << bit_reverse(8, i);
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
}
#endif
