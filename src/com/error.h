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

#pragma once

#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

class ErrorException final : public std::exception
{
        const std::string m_text;

public:
        ErrorException(const std::string& text) : m_text(text)
        {
        }
        const char* what() const noexcept override
        {
                return m_text.c_str();
        }
};

class ErrorSourceException final : public std::exception
{
        const std::string m_text, m_source_text;

public:
        ErrorSourceException(const std::string& text, const std::string& source_text) : m_text(text), m_source_text(source_text)
        {
        }
        const char* what() const noexcept override
        {
                return m_text.c_str();
        }
        std::string get_msg() const
        {
                return m_text;
        }
        std::string get_src() const
        {
                return m_source_text;
        }
};

[[noreturn]] inline void error(const std::string& text)
{
        throw ErrorException(text);
}

[[noreturn]] inline void error_source(const std::string& text, const std::string& source_text)
{
        throw ErrorSourceException(text, source_text);
}

[[noreturn]] inline void error_fatal(const std::string& text) noexcept
{
        // Без вызовов других функций программы, так как они могут вызвать эту же функцию.
        // Поэтому только std::fprintf и завершение программы.
        try
        {
                std::fprintf(stderr, "%s\n", text.c_str());
                std::fflush(stderr);
        }
        catch (...)
        {
        }

        std::_Exit(EXIT_FAILURE);
}

inline std::string get_error_list(const std::vector<std::string>& v)
{
        std::string names;
        for (const std::string& s : v)
        {
                if (s.size() == 0)
                {
                        continue;
                }
                if (names.size() > 0)
                {
                        names += "\n" + s;
                }
                else
                {
                        names += s;
                }
        }
        return names;
}

#if 1
#define ASSERT(expr)                     \
        ((expr) ? static_cast<void>(0) : \
                  error_fatal("Assert \"" + std::string(#expr) + "\" failed: " + __FILE__ + ":" + std::to_string(__LINE__)))
#else
#define ASSERT(expr) (static_cast<void>(0))
#endif
