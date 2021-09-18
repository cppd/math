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

#include "application.h"

#include "com/application.h"
#include "com/command_line.h"
#include "com/main_thread.h"
#include "com/support.h"
#include "dialogs/message.h"
#include "main_window/main_window.h"

#include <src/application/model_events.h>
#include <src/com/error.h>
#include <src/com/log.h>

namespace ns::gui
{
namespace
{
void message_event(const application::MessageEvent& event)
{
        switch (event.type)
        {
        case application::MessageType::ERROR:
                dialog::message_critical(event.text);
                return;
        case application::MessageType::ERROR_FATAL:
                dialog::message_critical(event.text, false /*with_parent*/);
                error_fatal("Exit after error message\n" + event.text);
        case application::MessageType::INFORMATION:
                dialog::message_information(event.text);
                return;
        case application::MessageType::WARNING:
                dialog::message_warning(event.text);
                return;
        }
        error_fatal("Unknown message event type " + std::to_string(static_cast<long long>(event.type)));
}
}

int run_application(int argc, char** argv)
{
        application::ModelEvents model_events;

        Application a(argc, argv);

        MainThread ui_thread;

        application::MessageEventsObserver message_observer(
                [](const application::MessageEvent& event)
                {
                        MainThread::run(
                                [event]()
                                {
                                        message_event(event);
                                });
                });

        LOG(command_line_description());

        create_delete_on_close_window<gui::main_window::MainWindow>()->show();

        int r = Application::exec();

        return r;
}
}
