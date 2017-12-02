/*
Copyright (C) 2017 Topological Manifold

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

#include "print.h"

#include <algorithm>
#if !defined(__clang__)
#include <quadmath.h>
#endif

std::string to_string(unsigned __int128 t)
{
        constexpr unsigned __int128 low_mask = (static_cast<unsigned __int128>(1) << 64) - 1;

        std::array<char, 100> buf;
        unsigned long long high = t >> 64;
        unsigned long long low = t & low_mask;
        if (high != 0)
        {
                snprintf(buf.data(), buf.size(), "%llu%llu", high, low);
        }
        else
        {
                snprintf(buf.data(), buf.size(), "%llu", low);
        }
        return buf.data();
}
std::string to_string(__int128 t)
{
        if (t >= 0)
        {
                return to_string(static_cast<unsigned __int128>(t));
        }
        else
        {
                return '-' + to_string(static_cast<unsigned __int128>(-t));
        }
}

#if 0 && !defined(__clang__)
std::string to_string(__float128 t)
{
        constexpr const char QUAD_MATH_FORMAT[] = "%.36Qe"; //"%+-#*.36Qe"

        std::array<char, 1000> buf;
        quadmath_snprintf(buf.data(), buf.size(), QUAD_MATH_FORMAT, t);
        return buf.data();
}
#endif

std::string source_with_line_numbers(const std::string s)
{
        size_t cnt = std::count(s.begin(), s.end(), '\n');
        if (cnt == 0)
        {
                return s;
        }

        int width = std::floor(std::log10(cnt)) + 1;

        std::ostringstream os;

        os << std::setfill('0');

        int line = 1;
        os << std::setw(width) << line << ": ";

        for (size_t i = 0; i < s.size(); ++i)
        {
                if (s[i] != '\n')
                {
                        os << s[i];
                }
                else
                {
                        os << '\n';
                        ++line;
                        os << std::setw(width) << line << ": ";
                }
        }

        return os.str();
}

std::string to_string_digit_groups(unsigned long long v, char s)
{
        std::string r;
        r.reserve(26);

        int i = 0;
        do
        {
                if ((++i % 3) == 1 && i != 1)
                {
                        r += s;
                }
                char c = (v % 10) + '0';
                r += c;

        } while ((v /= 10) > 0);

        std::reverse(r.begin(), r.end());

        return r;
}

std::string to_string_digit_groups(long long v, char s)
{
        if (v >= 0)
        {
                return to_string_digit_groups(static_cast<unsigned long long>(v), s);
        }
        else
        {
                return "-" + to_string_digit_groups(static_cast<unsigned long long>(-v), s);
        }
}

std::string to_string_digit_groups(unsigned int v, char s)
{
        return to_string_digit_groups(static_cast<unsigned long long>(v), s);
}
std::string to_string_digit_groups(unsigned long v, char s)
{
        return to_string_digit_groups(static_cast<unsigned long long>(v), s);
}
std::string to_string_digit_groups(int v, char s)
{
        return to_string_digit_groups(static_cast<long long>(v), s);
}
std::string to_string_digit_groups(long v, char s)
{
        return to_string_digit_groups(static_cast<long long>(v), s);
}
