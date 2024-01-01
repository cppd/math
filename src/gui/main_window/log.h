/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/color/rgb8.h>
#include <src/com/log/log.h>

#include <QPlainTextEdit>

#include <array>
#include <atomic>
#include <string>
#include <vector>

namespace ns::gui::main_window
{
class Log final
{
        struct Message final
        {
                std::string text;
                color::RGB8 color;

                template <typename T, typename C>
                Message(T&& text, C&& color)
                        : text(std::forward<T>(text)),
                          color(std::forward<C>(color))
                {
                }
        };

        QPlainTextEdit* const text_edit_;

        std::array<std::vector<Message>, 2> messages_;
        std::atomic<std::vector<Message>*> messages_ptr_;

        LogEventsObserver observer_;

public:
        explicit Log(QPlainTextEdit* text_edit);

        Log(const Log&) = delete;
        Log(Log&&) = delete;
        Log& operator=(const Log&) = delete;
        Log& operator=(Log&&) = delete;

        void write();
};
}
