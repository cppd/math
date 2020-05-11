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

#include "log.h"

#include "thread_ui.h"

#include <src/com/log.h>
#include <src/com/variant.h>

namespace gui::application
{
Log::Log(std::function<void(const std::vector<std::string>&, LogMessageType)> window_log)
{
        set_log_events([window_log](LogEvent&& event) {
                ThreadUI::run_in_ui_thread([&, event = std::move(event)]() {
                        const auto visitors = Visitors{[&](const LogEvent::Message& d) {
                                // Здесь без вызовов функции LOG, так как начнёт вызывать сама себя
                                std::vector<std::string> lines = format_log_message(d.text);
                                write_formatted_log_messages_to_stderr(lines);
                                window_log(lines, d.type);
                        }};
                        std::visit(visitors, event.data());
                });
        });
}

Log::~Log()
{
        set_log_events(nullptr);
}
}
