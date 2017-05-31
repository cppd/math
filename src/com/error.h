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
#ifndef ERROR_H
#define ERROR_H

#include "log.h"

#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

class ErrorException final : public std::exception
{
        const std::string m_msg;

public:
        ErrorException(const std::string& msg) : m_msg(msg)
        {
        }
        const char* what() const noexcept override
        {
                return m_msg.c_str();
        }
};

class ErrorSourceException final : public std::exception
{
        const std::string m_msg, m_src;

public:
        ErrorSourceException(const std::string& msg, const std::string& src) : m_msg(msg), m_src(src)
        {
        }
        const char* what() const noexcept override
        {
                return m_msg.c_str();
        }
        std::string get_msg() const
        {
                return m_msg;
        }
        std::string get_src() const
        {
                return m_src;
        }
};

// #define likely(x) __builtin_expect(static_cast<bool>(x), true)
// #define unlikely(x) __builtin_expect(static_cast<bool>(x), false)

[[noreturn]] inline void error(const std::string& text)
{
        throw ErrorException(text);
}

[[noreturn]] inline void error_src(const std::string& msg, const std::string& src)
{
        throw ErrorSourceException(msg, src);
}

[[noreturn]] inline void error_fatal(const std::string& text) noexcept
{
        try
        {
                LOG_ERROR(text);
        }
        catch (...)
        {
        }

        std::quick_exit(EXIT_FAILURE);
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
#define ASSERT_ENABLED
#define ASSERT(expr)                     \
        ((expr) ? static_cast<void>(0) : \
                  error("Assert \"" + std::string(#expr) + "\" failed: " + __FILE__ + ":" + std::to_string(__LINE__)))
#else
#undef ASSERT_ENABLED
#define ASSERT(expr) (static_cast<void>(0))
#endif

#endif
