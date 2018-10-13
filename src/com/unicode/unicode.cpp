/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "unicode.h"

#include "names.h"

#include "com/error.h"
#include "com/print.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <type_traits>

// UTF-8
// U+0000  .. U+007F    0xxxxxxx
// U+0080  .. U+07FF    110xxxxx 10xxxxxx
// U+0800  .. U+FFFF    1110xxxx 10xxxxxx 10xxxxxx
// U+10000 .. U+10FFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

namespace
{
char32_t utf8_to_utf32(const std::string& s, size_t from, size_t count)
{
        if (from >= s.size())
        {
                error("Empty UTF-8 string");
        }

        if (from + count > s.size())
        {
                return unicode_code_points::REPLACEMENT_CHARACTER;
        }

        switch (count)
        {
        case 1:
        {
                unsigned char s0 = s[from];
                return s0 & 0b111'1111;
        }
        case 2:
        {
                unsigned char s0 = s[from];
                unsigned char s1 = s[from + 1];
                return (s0 & 0b1'1111) << 6 | (s1 & 0b11'1111);
        }
        case 3:
        {
                unsigned char s0 = s[from];
                unsigned char s1 = s[from + 1];
                unsigned char s2 = s[from + 2];
                return (s0 & 0b1111) << 12 | (s1 & 0b11'1111) << 6 | (s2 & 0b11'1111);
        }
        case 4:
        {
                unsigned char s0 = s[from];
                unsigned char s1 = s[from + 1];
                unsigned char s2 = s[from + 2];
                unsigned char s3 = s[from + 3];
                return (s0 & 0b111) << 18 | (s1 & 0b11'1111) << 12 | (s2 & 0b11'1111) << 6 | (s3 & 0b11'1111);
        }
        }

        error("Error UTF-8 size " + to_string(count));
}
}

namespace unicode
{
template <typename T>
std::enable_if_t<std::is_same_v<T, char32_t>, std::string> utf32_to_utf8(T code_point)
{
        static_assert(std::is_same_v<T, char32_t>);

        if (code_point <= 0x7F)
        {
                return std::string(1, code_point);
        }
        if (code_point <= 0x7FF)
        {
                char c0 = 0b11000000 | (code_point >> 6);
                char c1 = 0b10000000 | (code_point & 0b11'1111);
                return std::string({c0, c1});
        }
        if (code_point <= 0xFFFF)
        {
                char c0 = 0b11100000 | (code_point >> 12);
                char c1 = 0b10000000 | ((code_point >> 6) & 0b11'1111);
                char c2 = 0b10000000 | (code_point & 0b11'1111);
                return std::string({c0, c1, c2});
        }
        if (code_point <= 0x10FFFF)
        {
                char c0 = 0b11110000 | (code_point >> 18);
                char c1 = 0b10000000 | ((code_point >> 12) & 0b11'1111);
                char c2 = 0b10000000 | ((code_point >> 6) & 0b11'1111);
                char c3 = 0b10000000 | (code_point & 0b11'1111);
                return std::string({c0, c1, c2, c3});
        }

        static_assert(unicode_code_points::REPLACEMENT_CHARACTER <= 0x10FFFF);
        return utf32_to_utf8(unicode_code_points::REPLACEMENT_CHARACTER);
}

template <typename T>
std::enable_if_t<std::is_same_v<T, char32_t>, std::string> utf32_to_number_string(T code_point)
{
        static_assert(std::is_same_v<T, char32_t>);

        // C++17, 6.9.1.5
        // Types char16_t and char32_t denote distinct types with the same size, signedness, and alignment
        // as uint_least16_t and uint_least32_t, respectively, in <cstdint>, called the underlying types.
        std::ostringstream oss;
        oss << "U+" << std::uppercase << std::hex << static_cast<std::uint_least32_t>(code_point);
        return oss.str();
}

std::string utf8_to_number_string(const std::string& s)
{
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');
        for (char c : s)
        {
                if (oss.str().size() > 0)
                {
                        oss << " ";
                }
                oss << "0x" << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(c));
        }
        return oss.str();
}

char32_t read_utf8_as_utf32(const std::string& s, size_t& i)
{
        ASSERT(i < s.size());

        unsigned char c = s[i];

        if (c <= 0x7F)
        {
                i += 1;
                return c;
        }

        size_t b = i;

        if (c >> 5 == 0b110)
        {
                i += 2;
                return ::utf8_to_utf32(s, b, 2);
        }
        if (c >> 4 == 0b1110)
        {
                i += 3;
                return ::utf8_to_utf32(s, b, 3);
        }
        if (c >> 3 == 0b11110)
        {
                i += 4;
                return ::utf8_to_utf32(s, b, 4);
        }

        i += 1;
        return unicode_code_points::REPLACEMENT_CHARACTER;
}

template std::string utf32_to_number_string(char32_t code_point);
template std::string utf32_to_utf8(char32_t code_point);
}
