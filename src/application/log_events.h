/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <mutex>
#include <string>
#include <vector>

namespace ns::application
{
struct LogEvent final
{
        enum class Type
        {
                Normal,
                Error,
                Warning,
                Information
        };

        std::string text;
        Type type;

        template <typename T>
        LogEvent(T&& text, Type type) : text(std::forward<T>(text)), type(type)
        {
        }
};

struct MessageEvent final
{
        enum class Type
        {
                Error,
                ErrorFatal,
                Warning,
                Information
        };

        std::string text;
        Type type;

        template <typename T>
        MessageEvent(T&& text, Type type) : text(std::forward<T>(text)), type(type)
        {
        }
};

class LogEvents final
{
        friend class LogEventsObserver;
        friend class MessageEventsObserver;
        friend void log_impl(LogEvent&&) noexcept;
        friend void log_impl(MessageEvent&&) noexcept;

        std::mutex lock_;
        std::vector<const std::function<void(const LogEvent&)>*> log_observers_;
        std::vector<const std::function<void(const MessageEvent&)>*> msg_observers_;

        void insert(const std::function<void(const LogEvent&)>* observer);
        void erase(const std::function<void(const LogEvent&)>* observer);

        void insert(const std::function<void(const MessageEvent&)>* observer);
        void erase(const std::function<void(const MessageEvent&)>* observer);

        void log_event(LogEvent&& event) noexcept;
        void log_event(MessageEvent&& event) noexcept;

public:
        LogEvents();
        ~LogEvents();

        LogEvents(const LogEvents&) = delete;
        LogEvents(LogEvents&&) = delete;
        LogEvents& operator=(const LogEvents&) = delete;
        LogEvents& operator=(LogEvents&&) = delete;
};

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

void log_impl(LogEvent&& event) noexcept;
void log_impl(MessageEvent&& event) noexcept;
}
