/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../dialogs/message.h"

#include <src/com/error.h>

#include <QApplication>
#include <QEvent>
#include <QObject>
#include <Qt>
#include <atomic>
#include <exception>
#include <functional>
#include <string>

namespace ns::gui
{
Application::Application(int* const argc, char** const argv)
        : QApplication(*argc, argv)
{
        static std::atomic_int call_counter = 0;
        if (++call_counter != 1)
        {
                error_fatal("Application must be created once");
        }

        qRegisterMetaType<std::function<void()>>("std::function<void()>");

        connect(
                this, &Application::signal, this,
                [](const std::function<void()>& f)
                {
                        f();
                },
                Qt::AutoConnection);

        application_ = this;
}

Application::~Application()
{
        application_ = nullptr;
}

void Application::run(const std::function<void()>& f)
{
        Q_EMIT application_->signal(f);
}

const QApplication* Application::instance()
{
        return application_;
}

bool Application::notify(QObject* const receiver, QEvent* const event) noexcept
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
                        constexpr bool WITH_PARENT = false;
                        dialog::message_critical(msg, WITH_PARENT);
                        error_fatal(msg);
                }
                catch (...)
                {
                        constexpr const char* MSG = "Error in an event receiver";
                        constexpr bool WITH_PARENT = false;
                        dialog::message_critical(MSG, WITH_PARENT);
                        error_fatal(MSG);
                }
        }
        catch (...)
        {
                error_fatal("Exception in the notify exception handlers");
        }
}
}
