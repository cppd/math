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

#include <functional>
#include <string>
#include <variant>
#include <vector>

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

void set_log_events(const std::function<void(LogEvent&&)>* events);
void set_message_events(const std::function<void(MessageEvent&&)>* events);

void log_impl(const std::string& msg, LogEvent::Type type) noexcept;
void message_impl(const std::string& msg, MessageEvent::Type type) noexcept;
