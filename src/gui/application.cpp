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

#include "application.h"

#include "com/application.h"
#include "com/application_message.h"
#include "com/command_line.h"
#include "com/support.h"
#include "dialogs/message.h"
#include "main_window/main_window.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/log/log.h>
#include <src/com/print.h>

namespace ns::gui
{
namespace
{
void message_event(const MessageEvent& event)
{
        switch (event.type)
        {
        case MessageType::ERROR:
                dialogs::message_critical(event.text);
                return;
        case MessageType::ERROR_FATAL:
                dialogs::message_critical(event.text, false /*with_parent*/);
                error_fatal("Exit after error message\n" + event.text);
        case MessageType::INFORMATION:
                dialogs::message_information(event.text);
                return;
        case MessageType::WARNING:
                dialogs::message_warning(event.text);
                return;
        }
        error_fatal("Unknown message event type " + to_string(enum_to_int(event.type)));
}
}

int run_application(int argc, char** const argv)
{
        const ApplicationMessage application_message;

        const Application a(&argc, argv);

        const MessageEventsObserver message_observer(
                [](const MessageEvent& event)
                {
                        Application::run(
                                [event]()
                                {
                                        message_event(event);
                                });
                });

        LOG(command_line_description());

        create_delete_on_close_window<gui::main_window::MainWindow>()->show();

        const int res = Application::exec();

        return res;
}
}
