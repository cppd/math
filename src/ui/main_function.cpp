/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "main_function.h"

#include "ui/dialogs/messages/message_box.h"
#include "ui/main_window/main_window.h"
#include "ui/support/support.h"

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
                        catch (std::exception& e)
                        {
                                std::string msg;
                                msg += "Error in an event receiver:\n";
                                msg += e.what();
                                message_critical(nullptr, msg.c_str());
                                error_fatal(msg);
                        }
                        catch (...)
                        {
                                const char* msg = "Error in an event receiver";
                                message_critical(nullptr, msg);
                                error_fatal(msg);
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in exception handlers");
                }
        }

public:
        Application(int& argc, char** argv) : QApplication(argc, argv)
        {
        }
};

int main_function(int argc, char* argv[])
{
        Application a(argc, argv);

        create_and_show_delete_on_close_window<MainWindow>();

        return a.exec();
}
