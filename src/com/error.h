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

#pragma once

#include <exception>
#include <string>
#include <utility>

namespace ns
{
class ErrorException final : public std::exception
{
        std::string text_;

public:
        explicit ErrorException(std::string&& text)
                : text_(std::move(text))
        {
        }

        [[nodiscard]] const char* what() const noexcept override
        {
                return text_.c_str();
        }
};

[[noreturn]] void error(std::string text);

[[noreturn]] void error_fatal(const char* text) noexcept;
[[noreturn]] void error_fatal(const std::string& text) noexcept;

namespace error_implementation
{
[[noreturn]] void error_assert(const char* expr, const char* file, int line) noexcept;
}
}

#if !defined(BUILD_RELEASE)
#define ASSERT(expr)                    \
        (static_cast<bool>(expr)        \
                 ? static_cast<void>(0) \
                 : ::ns::error_implementation::error_assert(#expr, __FILE__, __LINE__))
#else
#define ASSERT(expr) (static_cast<void>(0))
#endif
