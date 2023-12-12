/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "log/write.h"

#include <string>
#include <utility>

namespace ns
{
void error(std::string text)
{
        throw ErrorException(std::move(text));
}

void error_fatal(const char* const text) noexcept
{
        // no call to other functions because of a possible recursion
        static_assert(noexcept(write_log_fatal_error_and_exit(text)));
        write_log_fatal_error_and_exit(text);
}

void error_fatal(const std::string& text) noexcept
{
        error_fatal(text.c_str());
}

namespace error_implementation
{
void error_assert(const char* const expr, const char* const file, const int line) noexcept
{
        try
        {
                error_fatal(std::string("Assert \"") + expr + "\" failed: " + file + ":" + std::to_string(line));
        }
        catch (...)
        {
                error_fatal("Exception when creating assert message");
        }
}
}
}
