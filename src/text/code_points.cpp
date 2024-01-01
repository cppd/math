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

#include "code_points.h"

#include <src/com/error.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::text
{
namespace
{
constexpr std::array RANGES = std::to_array<std::array<char32_t, 2>>({
        {   0x0,   0x7F}, // C0 Controls and Basic Latin
        {  0x80,   0xFF}, // C1 Controls and Latin-1 Supplement
        { 0x100,  0x17F}, // Latin Extended-A
        { 0x180,  0x24F}, // Latin Extended-B
        {0x1E00, 0x1EFF}, // Latin Extended Additional
        {0x2C60, 0x2C7F}, // Latin Extended-C
        {0xA720, 0xA7FF}, // Latin Extended-D
        {0xAB30, 0xAB6F}, // Latin Extended-E
        { 0x250,  0x2AF}, // IPA Extensions
        { 0x2B0,  0x2FF}, // Spacing Modifier Letters
        { 0x370,  0x3FF}, // Greek and Coptic
        {0x1F00, 0x1FFF}, // Greek Extended
        { 0x400,  0x4FF}, // Cyrillic
        { 0x500,  0x52F}, // Cyrillic Supplement
        {0x2DE0, 0x2DFF}, // Cyrillic Extended-A
        {0xA640, 0xA69F}, // Cyrillic Extended-B
        {0x1C80, 0x1C8F}, // Cyrillic Extended-C
        {0xFFFD, 0xFFFD}  // Replacement character
});

constexpr std::size_t COUNT = []
{
        std::size_t res = RANGES.size();
        for (const auto [min, max] : RANGES)
        {
                ASSERT(max >= min);
                res += max - min;
        }
        return res;
}();
}

std::vector<char32_t> supported_code_points()
{
        std::vector<char32_t> res;
        res.reserve(COUNT);
        for (const auto [min, max] : RANGES)
        {
                for (char32_t i = min; i <= max; ++i)
                {
                        res.push_back(i);
                }
        }
        return res;
}
}
