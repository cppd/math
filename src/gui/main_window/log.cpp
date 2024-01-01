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

#include "log.h"

#include "../com/support.h"

#include <src/color/rgb8.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log/log.h>
#include <src/com/print.h>

#include <QPlainTextEdit>

#include <atomic>
#include <vector>

namespace ns::gui::main_window
{
namespace
{
color::RGB8 event_color(const LogType type)
{
        switch (type)
        {
        case LogType::NORMAL:
        {
                return {0, 0, 0};
        }
        case LogType::ERROR:
        {
                return {255, 0, 0};
        }
        case LogType::WARNING:
        {
                return {200, 150, 0};
        }
        case LogType::INFORMATION:
        {
                return {0, 0, 255};
        }
        }
        error_fatal("Unknown log event type " + to_string(enum_to_int(type)));
}

template <typename D, typename S>
class Switcher final
{
        std::atomic<D*>* const dst_;
        S* const src_;

public:
        Switcher(std::atomic<D*>* const dst, S* const src)
                : dst_(dst),
                  src_(src)
        {
        }

        ~Switcher()
        {
                src_->clear();
                *dst_ = src_;
        }

        Switcher(const Switcher&) = delete;
        Switcher(Switcher&&) = delete;
        Switcher& operator=(const Switcher&) = delete;
        Switcher& operator=(Switcher&&) = delete;
};
}

Log::Log(QPlainTextEdit* const text_edit)
        : text_edit_(text_edit),
          messages_ptr_(&messages_[0]),
          observer_(
                  [this](const LogEvent& event)
                  {
                          const color::RGB8 color = event_color(event.type);
                          if (!(*messages_ptr_).empty() && (*messages_ptr_).back().color == color)
                          {
                                  (*messages_ptr_).back().text += '\n';
                                  (*messages_ptr_).back().text += event.text;
                                  return;
                          }
                          (*messages_ptr_).emplace_back(event.text, color);
                  })
{
}

void Log::write()
{
        std::vector<Message>& log = messages_[(messages_ptr_ == &messages_[0]) ? 1 : 0];
        const Switcher switcher(&messages_ptr_, &log);
        for (const Message& m : log)
        {
                append_to_text_edit(text_edit_, m.text, m.color);
        }
}
}
