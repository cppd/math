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

#include <string>
#include <variant>

struct WindowEvent
{
        struct MessageError final
        {
                std::string text;
                explicit MessageError(const std::string& text) : text(text)
                {
                }
        };

        struct MessageErrorFatal final
        {
                std::string text;
                explicit MessageErrorFatal(const std::string& text) : text(text)
                {
                }
        };

        struct MessageInformation final
        {
                std::string text;
                explicit MessageInformation(const std::string& text) : text(text)
                {
                }
        };

        struct MessageWarning final
        {
                std::string text;
                explicit MessageWarning(const std::string& text) : text(text)
                {
                }
        };

        struct FileLoaded final
        {
                std::string file_name;
                size_t dimension;
                FileLoaded(const std::string& file_name, size_t dimension) : file_name(file_name), dimension(dimension)
                {
                }
        };

        using T = std::variant<FileLoaded, MessageError, MessageErrorFatal, MessageInformation, MessageWarning>;

        template <typename Type, typename = std::enable_if_t<!std::is_same_v<WindowEvent, std::remove_cvref_t<Type>>>>
        WindowEvent(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};
