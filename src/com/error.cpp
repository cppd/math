/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "error.h"

#include <cstdio>
#include <cstdlib>

void error(const std::string& text)
{
        throw ErrorException(text);
}

void error(std::string&& text)
{
        throw ErrorException(std::move(text));
}

void error_source(std::string&& text, std::string&& source_text)
{
        throw ErrorSourceException(std::move(text), std::move(source_text));
}

void error_fatal(const char* text) noexcept
{
        // Без вызовов других функций программы, так как они могут вызвать эту же функцию.
        // Поэтому только std::fprintf и завершение программы.
        try
        {
                std::fprintf(stderr, "%s\n", text);
                std::fflush(stderr);
        }
        catch (...)
        {
        }

        std::_Exit(EXIT_FAILURE);
}

void error_fatal(const std::string& text) noexcept
{
        error_fatal(text.c_str());
}

namespace error_implementation
{
void error_assert(const char* expr, const char* file, int line) noexcept
{
        error_fatal(std::string("Assert \"") + expr + "\" failed: " + file + ":" + std::to_string(line));
}
}
