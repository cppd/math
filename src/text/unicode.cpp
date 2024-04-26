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

#include "unicode.h"

#include <src/com/error.h>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

// UTF-8
// U+0000  .. U+007F    0xxxxxxx
// U+0080  .. U+07FF    110xxxxx 10xxxxxx
// U+0800  .. U+FFFF    1110xxxx 10xxxxxx 10xxxxxx
// U+10000 .. U+10FFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

namespace ns::text::unicode
{
namespace
{
char32_t utf8_2_to_utf32(const std::string& s, std::size_t* const i)
{
        const unsigned char s1 = s[*i + 1];
        if ((s1 & 0b1100'0000) == 0b1000'0000)
        {
                const unsigned char s0 = s[*i];
                *i += 2;
                return (s0 & 0b1'1111) << 6 | (s1 & 0b11'1111);
        }
        *i += 1;
        return unicode::REPLACEMENT_CHARACTER;
}

char32_t utf8_3_to_utf32(const std::string& s, std::size_t* const i)
{
        const unsigned char s1 = s[*i + 1];
        if ((s1 & 0b1100'0000) == 0b1000'0000)
        {
                const unsigned char s2 = s[*i + 2];
                if ((s2 & 0b1100'0000) == 0b1000'0000)
                {
                        const unsigned char s0 = s[*i];
                        *i += 3;
                        return (s0 & 0b1111) << 12 | (s1 & 0b11'1111) << 6 | (s2 & 0b11'1111);
                }
        }
        *i += 1;
        return unicode::REPLACEMENT_CHARACTER;
}

char32_t utf8_4_to_utf32(const std::string& s, std::size_t* const i)
{
        const unsigned char s1 = s[*i + 1];
        if ((s1 & 0b1100'0000) == 0b1000'0000)
        {
                const unsigned char s2 = s[*i + 2];
                if ((s2 & 0b1100'0000) == 0b1000'0000)
                {
                        const unsigned char s3 = s[*i + 3];
                        if ((s3 & 0b1100'0000) == 0b1000'0000)
                        {
                                const unsigned char s0 = s[*i];
                                const char32_t res =
                                        (s0 & 0b111) << 18 | (s1 & 0b11'1111) << 12 | (s2 & 0b11'1111) << 6
                                        | (s3 & 0b11'1111);
                                if (res <= 0x10FFFF)
                                {
                                        *i += 4;
                                        return res;
                                }
                        }
                }
        }
        *i += 1;
        return unicode::REPLACEMENT_CHARACTER;
}
}

template <typename T>
std::string utf32_to_number_string(const T code_point)
{
        static_assert(std::is_same_v<T, char32_t>);

        std::ostringstream oss;
        oss << "U+";
        oss << std::uppercase << std::hex << std::setfill('0');
        oss << std::setw(4) << static_cast<std::uint_least32_t>(code_point);
        return oss.str();
}

std::string utf8_to_number_string(const std::string& s)
{
        if (s.empty())
        {
                error("Empty UTF-8 string");
        }

        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0');
        for (const char c : s)
        {
                if (!oss.str().empty())
                {
                        oss << " ";
                }
                oss << "0x" << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(c));
        }
        return oss.str();
}

template <typename T>
std::string utf32_to_utf8(const T code_point)
{
        static_assert(std::is_same_v<T, char32_t>);

        if (code_point <= 0x7F)
        {
                return std::string(1, code_point);
        }

        if (code_point <= 0x7FF)
        {
                const char c0 = 0b1100'0000 | (code_point >> 6);
                const char c1 = 0b1000'0000 | (code_point & 0b11'1111);
                return std::string({c0, c1});
        }

        if (code_point <= 0xFFFF)
        {
                const char c0 = 0b1110'0000 | (code_point >> 12);
                const char c1 = 0b1000'0000 | ((code_point >> 6) & 0b11'1111);
                const char c2 = 0b1000'0000 | (code_point & 0b11'1111);
                return std::string({c0, c1, c2});
        }

        if (code_point <= 0x10FFFF)
        {
                const char c0 = 0b1111'0000 | (code_point >> 18);
                const char c1 = 0b1000'0000 | ((code_point >> 12) & 0b11'1111);
                const char c2 = 0b1000'0000 | ((code_point >> 6) & 0b11'1111);
                const char c3 = 0b1000'0000 | (code_point & 0b11'1111);
                return std::string({c0, c1, c2, c3});
        }

        static_assert(unicode::REPLACEMENT_CHARACTER <= 0x10FFFF);
        return utf32_to_utf8(unicode::REPLACEMENT_CHARACTER);
}

char32_t utf8_to_utf32(const std::string& s, std::size_t* const i)
{
        if (*i >= s.size())
        {
                error("UTF-8 string index out of range");
        }

        const unsigned char s0 = s[*i];

        if (s0 <= 0x7F)
        {
                *i += 1;
                return s0;
        }

        if ((s0 & 0b1110'0000) == 0b1100'0000)
        {
                if (*i + 1 < s.size())
                {
                        return utf8_2_to_utf32(s, i);
                }
        }
        else if ((s0 & 0b1111'0000) == 0b1110'0000)
        {
                if (*i + 2 < s.size())
                {
                        return utf8_3_to_utf32(s, i);
                }
        }
        else if ((s0 & 0b1111'1000) == 0b1111'0000)
        {
                if (*i + 3 < s.size())
                {
                        return utf8_4_to_utf32(s, i);
                }
        }

        *i += 1;
        return unicode::REPLACEMENT_CHARACTER;
}

char32_t utf8_to_utf32(const std::string& s)
{
        std::size_t i = 0;
        const char32_t c = unicode::utf8_to_utf32(s, &i);
        if (i == s.size())
        {
                return c;
        }
        error("One UTF-8 character string is too long: " + utf8_to_number_string(s));
}

template std::string utf32_to_number_string(char32_t);
template std::string utf32_to_utf8(char32_t);
}
