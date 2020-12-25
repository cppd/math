/*
Copyright (C) 2017-2020 Topological Manifold

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

namespace ns::text
{
std::vector<char32_t> supported_code_points()
{
        std::vector<char32_t> code_points;

        // C0 Controls and Basic Latin
        for (char32_t code_point = 0x0; code_point <= 0x7F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // C1 Controls and Latin-1 Supplement
        for (char32_t code_point = 0x80; code_point <= 0xFF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Latin Extended-A
        for (char32_t code_point = 0x100; code_point <= 0x17F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Latin Extended-B
        for (char32_t code_point = 0x180; code_point <= 0x24F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Latin Extended Additional
        for (char32_t code_point = 0x1E00; code_point <= 0x1EFF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Latin Extended-C
        for (char32_t code_point = 0x2C60; code_point <= 0x2C7F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Latin Extended-D
        for (char32_t code_point = 0xA720; code_point <= 0xA7FF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Latin Extended-E
        for (char32_t code_point = 0xAB30; code_point <= 0xAB6F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // IPA Extensions
        for (char32_t code_point = 0x250; code_point <= 0x2AF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Spacing Modifier Letters
        for (char32_t code_point = 0x2B0; code_point <= 0x2FF; ++code_point)
        {
                code_points.push_back(code_point);
        }

#if 0
        // Combining Diacritical Marks
        for (char32_t code_point = 0x300; code_point <= 0x36F; ++code_point)
        {
                code_points.push_back(code_point);
        }
#endif

        // Greek and Coptic
        for (char32_t code_point = 0x370; code_point <= 0x3FF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Greek Extended
        for (char32_t code_point = 0x1F00; code_point <= 0x1FFF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        //  Cyrillic
        for (char32_t code_point = 0x400; code_point <= 0x4FF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Cyrillic Supplement
        for (char32_t code_point = 0x500; code_point <= 0x52F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Cyrillic Extended-A
        for (char32_t code_point = 0x2DE0; code_point <= 0x2DFF; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Cyrillic Extended-B
        for (char32_t code_point = 0xA640; code_point <= 0xA69F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Cyrillic Extended-C
        for (char32_t code_point = 0x1C80; code_point <= 0x1C8F; ++code_point)
        {
                code_points.push_back(code_point);
        }

        // Replacement character
        code_points.push_back(0xFFFD);

        return code_points;
}
}
