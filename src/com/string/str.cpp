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

#include "str.h"

#include <src/com/error.h>

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

namespace ns
{
namespace
{
char to_upper(const unsigned char c)
{
        return std::toupper(c);
}

char to_lower(const unsigned char c)
{
        return std::tolower(c);
}

bool is_space(const unsigned char c)
{
        return std::isspace(c) != 0;
}

bool is_alpha(const unsigned char c)
{
        return std::isalpha(c) != 0;
}

bool is_print(const unsigned char c)
{
        return std::isprint(c) != 0;
}
}

std::string trim(const std::string_view s)
{
        if (s.empty())
        {
                return {};
        }

        const std::size_t n = s.size();

        std::size_t i = 0;
        while (i < n && is_space(s[i]))
        {
                ++i;
        }
        if (i == n)
        {
                return {};
        }

        std::size_t ri = s.size() - 1;
        while (is_space(s[ri]))
        {
                if (ri == 0)
                {
                        error("trim error from string: " + std::string(s));
                }
                --ri;
        }

        return std::string(s.substr(i, ri - i + 1));
}

std::string to_upper(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += to_upper(c);
        }
        return res;
}

std::string to_lower(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += to_lower(c);
        }
        return res;
}

std::string to_upper_first_letters(const std::string_view s)
{
        bool first_letter = true;
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                if (is_alpha(c))
                {
                        if (first_letter)
                        {
                                res += to_upper(c);
                                first_letter = false;
                        }
                        else
                        {
                                res += to_lower(c);
                        }
                }
                else
                {
                        res += c;
                        first_letter = true;
                }
        }
        return res;
}

std::string add_indent(const std::string_view s, const unsigned indent_size)
{
        const std::string indent(indent_size, ' ');

        std::string res;
        res.reserve(indent_size + s.size());
        res += indent;
        for (const char c : s)
        {
                res += c;
                if (c == '\n')
                {
                        res += indent;
                }
        }
        if (!s.empty() && s.back() == '\n')
        {
                res.resize(res.size() - indent_size);
        }
        return res;
}

std::string printable_characters(const std::string_view s)
{
        std::string res;
        for (const char c : s)
        {
                res += is_print(c) ? c : ' ';
        }
        return res;
}

std::string replace_space(const std::string_view s, const char value)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += !is_space(c) ? c : value;
        }
        return res;
}
}
