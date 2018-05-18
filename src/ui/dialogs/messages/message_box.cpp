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

#include "message_box.h"

#include "application/application_name.h"
#include "ui/support/support.h"

#include <QMessageBox>

void message_critical(QWidget* parent, const QString& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::Critical, APPLICATION_NAME, message, QMessageBox::Ok, parent);
        w->exec();
}

void message_information(QWidget* parent, const QString& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::Information, APPLICATION_NAME, message, QMessageBox::Ok, parent);
        w->exec();
}

void message_warning(QWidget* parent, const QString& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::Warning, APPLICATION_NAME, message, QMessageBox::Ok, parent);
        w->exec();
}

bool message_question_default_yes(QWidget* parent, const QString& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::Question, APPLICATION_NAME, message,
                                               QMessageBox::Yes | QMessageBox::No, parent);
        w->setDefaultButton(QMessageBox::Yes);
        return w->exec() == QMessageBox::Yes;
}

bool message_question_default_no(QWidget* parent, const QString& message)
{
        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::Question, APPLICATION_NAME, message,
                                               QMessageBox::Yes | QMessageBox::No, parent);
        w->setDefaultButton(QMessageBox::No);
        return w->exec() == QMessageBox::Yes;
}
