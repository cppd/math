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

void log_init();
void log_exit();

enum class LogMessageType
{
        Normal,
        Error,
        Warning,
        Information
};

struct LogEvent final
{
        struct Message final
        {
                std::string text;
                LogMessageType type;
                template <typename T>
                Message(T&& text, LogMessageType type) : text(std::forward<T>(text)), type(type)
                {
                }
        };

        using T = std::variant<Message>;

        template <typename Type, typename = std::enable_if_t<!std::is_same_v<LogEvent, std::remove_cvref_t<Type>>>>
        LogEvent(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

enum class MessageType
{
        Error,
        ErrorFatal,
        Warning,
        Information
};

struct MessageEvent final
{
        struct Message final
        {
                std::string text;
                MessageType type;
                template <typename T>
                Message(T&& text, MessageType type) : text(std::forward<T>(text)), type(type)
                {
                }
        };

        using T = std::variant<Message>;

        template <typename Type, typename = std::enable_if_t<!std::is_same_v<MessageEvent, std::remove_cvref_t<Type>>>>
        MessageEvent(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

void set_log_events(const std::function<void(LogEvent&&)>& events);
void set_message_events(const std::function<void(MessageEvent&&)>& events);

std::vector<std::string> format_log_message(const std::string& msg) noexcept;
void write_formatted_log_messages_to_stderr(const std::vector<std::string>& lines) noexcept;

void log_impl(const std::string& msg, LogMessageType type) noexcept;
void message_impl(const std::string& msg, MessageType type) noexcept;
