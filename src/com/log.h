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

#include <sstream>
#include <string>

void LOG(const std::string& msg) noexcept;
void LOG_ERROR(const std::string& msg) noexcept;
void LOG_WARNING(const std::string& msg) noexcept;
void LOG_INFORMATION(const std::string& msg) noexcept;

void MESSAGE_ERROR(const std::string& msg) noexcept;
void MESSAGE_ERROR_FATAL(const std::string& msg) noexcept;
void MESSAGE_WARNING(const std::string& msg) noexcept;
void MESSAGE_INFORMATION(const std::string& msg) noexcept;

class LogStringStream
{
        std::ostringstream m_stream;

protected:
        LogStringStream() = default;
        ~LogStringStream() = default;

        std::string str() const noexcept
        {
                return m_stream.str();
        }

public:
        template <typename T>
        LogStringStream& operator<<(T&& s)
        {
                m_stream << std::forward<T>(s);
                return *this;
        }
};

template <typename T>
class Log final
{
public:
        Log(const T& v)
        {
                LOG(v);
        }
};

template <>
class Log<void> final : public LogStringStream
{
public:
        ~Log()
        {
                LOG(str());
        }
};

template <typename = void>
Log() -> Log<void>;
