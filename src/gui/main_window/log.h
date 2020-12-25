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

#include <src/application/log_events.h>
#include <src/color/color.h>

#include <QPlainTextEdit>
#include <array>
#include <atomic>
#include <string>
#include <vector>

namespace ns::gui::main_window
{
class Log final
{
        struct Message
        {
                std::string text;
                Srgb8 color;
                template <typename T, typename C>
                Message(T&& text, C&& color) : text(std::forward<T>(text)), color(std::forward<C>(color))
                {
                }
        };

        QPlainTextEdit* const m_text_edit;

        std::array<std::vector<Message>, 2> m_messages;
        std::atomic<std::vector<Message>*> m_messages_ptr;

        application::LogEventsObserver m_observer;

public:
        explicit Log(QPlainTextEdit* text_edit);

        Log(const Log&) = delete;
        Log(Log&&) = delete;
        Log& operator=(const Log&) = delete;
        Log& operator=(Log&&) = delete;

        void write();
};
}
