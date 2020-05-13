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

#include "message_events.h"

#include "thread_ui.h"

#include "../dialogs/message.h"

#include <src/com/log.h>
#include <src/com/log_impl.h>
#include <src/com/variant.h>

namespace gui::application
{
namespace
{
void message_event(const MessageEvent& event)
{
        switch (event.type)
        {
        case MessageEvent::Type::Error:
        {
                LOG_ERROR(event.text);
                dialog::message_critical(event.text);
                break;
        }
        case MessageEvent::Type::ErrorFatal:
        {
                std::string message = !event.text.empty() ? event.text : "Unknown Error. Exit Failure.";
                LOG_ERROR(message);
                dialog::message_critical(message, false /*with_parent*/);
                std::_Exit(EXIT_FAILURE);
        }
        case MessageEvent::Type::Information:
        {
                LOG_INFORMATION(event.text);
                dialog::message_information(event.text);
                break;
        }
        case MessageEvent::Type::Warning:
        {
                LOG_WARNING(event.text);
                dialog::message_warning(event.text);
                break;
        }
        }
}
}

MessageEvents::MessageEvents()
{
        m_events = [](MessageEvent&& event) {
                ThreadUI::run_in_ui_thread([event = std::move(event)]() { message_event(event); });
        };

        set_message_events(&m_events);
}

MessageEvents::~MessageEvents()
{
        set_message_events(nullptr);
}
}
