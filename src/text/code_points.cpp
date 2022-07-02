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

#include "code_points.h"

#include <src/com/error.h>

#include <tuple>

namespace ns::text
{
namespace
{
std::vector<std::tuple<char32_t, char32_t>> code_point_ranges()
{
        std::vector<std::tuple<char32_t, char32_t>> res;
        res.reserve(19);

        // C0 Controls and Basic Latin
        res.emplace_back(0x0, 0x7F);

        // C1 Controls and Latin-1 Supplement
        res.emplace_back(0x80, 0xFF);

        // Latin Extended-A
        res.emplace_back(0x100, 0x17F);

        // Latin Extended-B
        res.emplace_back(0x180, 0x24F);

        // Latin Extended Additional
        res.emplace_back(0x1E00, 0x1EFF);

        // Latin Extended-C
        res.emplace_back(0x2C60, 0x2C7F);

        // Latin Extended-D
        res.emplace_back(0xA720, 0xA7FF);

        // Latin Extended-E
        res.emplace_back(0xAB30, 0xAB6F);

        // IPA Extensions
        res.emplace_back(0x250, 0x2AF);

        // Spacing Modifier Letters
        res.emplace_back(0x2B0, 0x2FF);

#if 0
        // Combining Diacritical Marks
        res.emplace_back(0x300, 0x36F);
#endif

        // Greek and Coptic
        res.emplace_back(0x370, 0x3FF);

        // Greek Extended
        res.emplace_back(0x1F00, 0x1FFF);

        // Cyrillic
        res.emplace_back(0x400, 0x4FF);

        // Cyrillic Supplement
        res.emplace_back(0x500, 0x52F);

        // Cyrillic Extended-A
        res.emplace_back(0x2DE0, 0x2DFF);

        // Cyrillic Extended-B
        res.emplace_back(0xA640, 0xA69F);

        // Cyrillic Extended-C
        res.emplace_back(0x1C80, 0x1C8F);

        // Replacement character
        res.emplace_back(0xFFFD, 0xFFFD);

        return res;
}
}

std::vector<char32_t> supported_code_points()
{
        const std::vector<std::tuple<char32_t, char32_t>>& ranges = code_point_ranges();

        const std::size_t count = [&]
        {
                std::size_t res = ranges.size();
                for (const auto& [min, max] : ranges)
                {
                        ASSERT(max >= min);
                        res += max - min;
                }
                return res;
        }();

        std::vector<char32_t> res;
        res.reserve(count);
        for (const auto& [min, max] : ranges)
        {
                for (char32_t i = min; i <= max; ++i)
                {
                        res.push_back(i);
                }
        }
        return res;
}
}
