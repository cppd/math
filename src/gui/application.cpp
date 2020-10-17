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

#include "application.h"

#include "com/command_line.h"
#include "com/main_thread.h"
#include "com/support.h"
#include "dialogs/message.h"
#include "main_window/main_window.h"

#include <src/com/error.h>
#include <src/com/log.h>

namespace gui
{
namespace
{
class Application final : public QApplication
{
        bool notify(QObject* receiver, QEvent* event) noexcept override
        {
                try
                {
                        try
                        {
                                return QApplication::notify(receiver, event);
                        }
                        catch (const std::exception& e)
                        {
                                std::string msg;
                                msg += "Error in an event receiver\n";
                                msg += e.what();
                                dialog::message_critical(msg, false /*with_parent*/);
                                error_fatal(msg);
                        }
                        catch (...)
                        {
                                const char* msg = "Error in an event receiver";
                                dialog::message_critical(msg, false /*with_parent*/);
                                error_fatal(msg);
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in the notify exception handlers");
                }
        }

public:
        Application(int& argc, char** argv) : QApplication(argc, argv)
        {
        }
};

void message_event(const MessageEvent& event)
{
        switch (event.type)
        {
        case MessageEvent::Type::Error:
                dialog::message_critical(event.text);
                return;
        case MessageEvent::Type::ErrorFatal:
                dialog::message_critical(event.text, false /*with_parent*/);
                error_fatal("Exit after error message\n" + event.text);
        case MessageEvent::Type::Information:
                dialog::message_information(event.text);
                return;
        case MessageEvent::Type::Warning:
                dialog::message_warning(event.text);
                return;
        }
}
}

int run_application(int argc, char** argv)
{
        Application a(argc, argv);

        MainThread ui_thread;
        application::MessageEventsObserver message_observer(
                [](const MessageEvent& event) { MainThread::run([event]() { message_event(event); }); });

        LOG(command_line_description());

        create_delete_on_close_window<MainWindow>()->show();

        int r = Application::exec();

        return r;
}
}
