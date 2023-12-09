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

#include "application_message.h"

#include <src/com/error.h>
#include <src/com/log.h>

#include <QString>
#include <QtGlobal>
#include <string>

namespace ns::gui
{
namespace
{
void message_handler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
        std::string s = "Qt:";

        if (!msg.isEmpty())
        {
                s += " ";
                s += msg.toStdString();
        }

        if (context.file)
        {
                s += " (";
                s += context.file;
                s += ":";
                s += std::to_string(context.line);
                s += ")";
        }

        if (context.function)
        {
                s += " (";
                s += context.function;
                s += ")";
        }

        switch (type)
        {
        case QtDebugMsg:
                LOG(s);
                return;
        case QtInfoMsg:
                LOG_INFORMATION(s);
                return;
        case QtWarningMsg:
                LOG_WARNING(s);
                return;
        case QtCriticalMsg:
                LOG_ERROR(s);
                return;
        case QtFatalMsg:
                error_fatal(s);
        }

        LOG(s);
}
}

ApplicationMessage::ApplicationMessage()
{
        qInstallMessageHandler(message_handler);
}

ApplicationMessage::~ApplicationMessage()
{
        qInstallMessageHandler(nullptr);
}
}
