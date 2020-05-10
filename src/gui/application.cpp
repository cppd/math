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

#include "com/support.h"
#include "com/thread_ui_impl.h"
#include "dialogs/message.h"
#include "main_window/main_window.h"

#include <src/com/error.h>

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
                                msg += "Error in an event receiver:\n";
                                msg += e.what();
                                dialog::message_critical(nullptr, msg);
                                error_fatal(msg);
                        }
                        catch (...)
                        {
                                const char* msg = "Error in an event receiver";
                                dialog::message_critical(nullptr, msg);
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
}

int application(int argc, char* argv[])
{
        Application a(argc, argv);

        thread_ui_impl::ThreadUI thread_ui_object;

        create_and_show_delete_on_close_window<MainWindow>();

        return Application::exec();
}
