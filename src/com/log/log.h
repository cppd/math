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

#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace ns
{
enum class LogType
{
        NORMAL,
        ERROR,
        WARNING,
        INFORMATION
};

enum class MessageType
{
        ERROR,
        ERROR_FATAL,
        WARNING,
        INFORMATION
};

struct LogEvent final
{
        std::string text;
        LogType type;

        template <typename T>
        LogEvent(T&& text, const LogType type)
                : text(std::forward<T>(text)),
                  type(type)
        {
        }
};

struct MessageEvent final
{
        std::string text;
        MessageType type;

        template <typename T>
        MessageEvent(T&& text, const MessageType type)
                : text(std::forward<T>(text)),
                  type(type)
        {
        }
};

void log(std::string_view text, LogType type) noexcept;
void log(std::string_view text, MessageType type) noexcept;

class LogEventsObserver final
{
        std::function<void(const LogEvent&)> observer_;

public:
        explicit LogEventsObserver(std::function<void(const LogEvent&)> observer);
        ~LogEventsObserver();

        LogEventsObserver(const LogEventsObserver&) = delete;
        LogEventsObserver(LogEventsObserver&&) = delete;
        LogEventsObserver& operator=(const LogEventsObserver&) = delete;
        LogEventsObserver& operator=(LogEventsObserver&&) = delete;
};

class MessageEventsObserver final
{
        std::function<void(const MessageEvent&)> observer_;

public:
        explicit MessageEventsObserver(std::function<void(const MessageEvent&)> observer);
        ~MessageEventsObserver();

        MessageEventsObserver(const MessageEventsObserver&) = delete;
        MessageEventsObserver(MessageEventsObserver&&) = delete;
        MessageEventsObserver& operator=(const MessageEventsObserver&) = delete;
        MessageEventsObserver& operator=(MessageEventsObserver&&) = delete;
};
}
