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

#include "str.h"

#include "com/error.h"

#include <cctype>

namespace
{
char to_upper(char c)
{
        return std::toupper(static_cast<unsigned char>(c));
}
char to_lower(char c)
{
        return std::tolower(static_cast<unsigned char>(c));
}
bool is_space(char c)
{
        return std::isspace(static_cast<unsigned char>(c));
}
}

std::string trim(const std::string_view& s)
{
        if (s.size() == 0)
        {
                return std::string();
        }

        size_t n = s.size();
        size_t i = 0;
        while (i < n && is_space(s[i]))
        {
                ++i;
        }
        if (i == n)
        {
                return std::string();
        }

        size_t ri = s.size() - 1;
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

std::string to_upper(const std::string_view& s)
{
        std::string result;
        result.reserve(s.size());
        for (char c : s)
        {
                result += to_upper(c);
        }
        return result;
}

std::string to_lower(const std::string_view& s)
{
        std::string result;
        result.reserve(s.size());
        for (char c : s)
        {
                result += to_lower(c);
        }
        return result;
}
