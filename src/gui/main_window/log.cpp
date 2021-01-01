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

#include "log.h"

#include "../com/support.h"

namespace ns::gui::main_window
{
namespace
{
Srgb8 event_color(application::LogEvent::Type type)
{
        switch (type)
        {
        case application::LogEvent::Type::Normal:
        {
                return Srgb8(0, 0, 0);
        }
        case application::LogEvent::Type::Error:
        {
                return Srgb8(255, 0, 0);
        }
        case application::LogEvent::Type::Warning:
        {
                return Srgb8(200, 150, 0);
        }
        case application::LogEvent::Type::Information:
        {
                return Srgb8(0, 0, 255);
        }
        }
        error_fatal("Unknown log event type");
}

template <typename D, typename S>
class Switcher final
{
        std::atomic<D*>* const m_dst;
        S* const m_src;

public:
        Switcher(std::atomic<D*>* dst, S* src) : m_dst(dst), m_src(src)
        {
        }
        ~Switcher()
        {
                m_src->clear();
                *m_dst = m_src;
        }
        Switcher(const Switcher&) = delete;
        Switcher(Switcher&&) = delete;
        Switcher& operator=(const Switcher&) = delete;
        Switcher& operator=(Switcher&&) = delete;
};
}

Log::Log(QPlainTextEdit* text_edit)
        : m_text_edit(text_edit),
          m_messages_ptr(&m_messages[0]),
          m_observer(
                  [this](const application::LogEvent& event)
                  {
                          Srgb8 color = event_color(event.type);
                          if (!(*m_messages_ptr).empty() && (*m_messages_ptr).back().color == color)
                          {
                                  (*m_messages_ptr).back().text += '\n';
                                  (*m_messages_ptr).back().text += event.text;
                                  return;
                          }
                          (*m_messages_ptr).emplace_back(event.text, color);
                  })
{
}

void Log::write()
{
        std::vector<Message>& log = m_messages[(m_messages_ptr == &m_messages[0]) ? 1 : 0];
        Switcher switcher(&m_messages_ptr, &log);
        for (const Message& m : log)
        {
                append_to_text_edit(m_text_edit, m.text, m.color);
        }
}
}
