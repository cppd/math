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

#include "set_message.h"

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
        const auto visitors = Visitors{[](const MessageEvent::Message& d) {
                switch (d.type)
                {
                case MessageType::Error:
                {
                        LOG_ERROR(d.text);
                        dialog::message_critical(d.text);
                        break;
                }
                case MessageType::ErrorFatal:
                {
                        std::string message = !d.text.empty() ? d.text : "Unknown Error. Exit Failure.";
                        LOG_ERROR(message);
                        dialog::message_critical(message, false /*with_parent*/);
                        std::_Exit(EXIT_FAILURE);
                }
                case MessageType::Information:
                {
                        LOG_INFORMATION(d.text);
                        dialog::message_information(d.text);
                        break;
                }
                case MessageType::Warning:
                {
                        LOG_WARNING(d.text);
                        dialog::message_warning(d.text);
                        break;
                }
                }
        }};

        std::visit(visitors, event.data());
}
}

SetMessage::SetMessage()
{
        set_message_events([=](MessageEvent&& event) {
                ThreadUI::run_in_ui_thread([&, event = std::move(event)]() { message_event(event); });
        });
}

SetMessage::~SetMessage()
{
        set_message_events(nullptr);
}
}
