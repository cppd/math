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

#pragma once

#include <exception>
#include <string>
#include <utility>

class ErrorException final : public std::exception
{
        std::string m_text;

public:
        template <typename T, typename = std::enable_if_t<!std::is_same_v<ErrorException, std::remove_cvref<T>>>>
        explicit ErrorException(T&& text) : m_text(std::forward<T>(text))
        {
        }
        const char* what() const noexcept override
        {
                return m_text.c_str();
        }
};

[[noreturn]] void error(const std::string& text);
[[noreturn]] void error(std::string&& text);

[[noreturn]] void error_fatal(const char* text) noexcept;
[[noreturn]] void error_fatal(const std::string& text) noexcept;

namespace error_implementation
{
[[noreturn]] void error_assert(const char* expr, const char* file, int line) noexcept;
}

#if 1
#define ASSERT(expr) ((expr) ? static_cast<void>(0) : error_implementation::error_assert(#expr, __FILE__, __LINE__))
#else
#define ASSERT(expr) (static_cast<void>(0))
#endif
