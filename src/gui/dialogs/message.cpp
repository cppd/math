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

#include "message.h"

#include "../com/support.h"

#include <src/settings/name.h>

#include <QMessageBox>

namespace ns::gui::dialog
{
void message_critical(const std::string& message, bool with_parent)
{
        QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::Critical, settings::APPLICATION_NAME, QString::fromStdString(message), QMessageBox::Ok,
                with_parent ? parent_for_dialog() : nullptr);

        w->exec();
}

void message_information(const std::string& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::Information, settings::APPLICATION_NAME, QString::fromStdString(message), QMessageBox::Ok,
                parent_for_dialog());

        w->exec();
}

void message_warning(const std::string& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::Warning, settings::APPLICATION_NAME, QString::fromStdString(message), QMessageBox::Ok,
                parent_for_dialog());

        w->exec();
}

std::optional<bool> message_question_default_yes(const std::string& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::Question, settings::APPLICATION_NAME, QString::fromStdString(message),
                QMessageBox::Yes | QMessageBox::No, parent_for_dialog());

        w->setDefaultButton(QMessageBox::Yes);

        if (w->exec() && !w.isNull())
        {
                return (w->result() == QMessageBox::Yes);
        }

        return std::nullopt;
}

std::optional<bool> message_question_default_no(const std::string& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::Question, settings::APPLICATION_NAME, QString::fromStdString(message),
                QMessageBox::Yes | QMessageBox::No, parent_for_dialog());

        w->setDefaultButton(QMessageBox::No);

        if (w->exec() && !w.isNull())
        {
                return (w->result() == QMessageBox::Yes);
        }

        return std::nullopt;
}
}
