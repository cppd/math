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

#include "source_error.h"

#include "../../support/support.h"

#include <QStyle>

namespace message_source_error_implementation
{
SourceError::SourceError(QWidget* parent, const std::string& message, const std::string& source) : QDialog(parent)
{
        ui.setupUi(this);

        ui.labelPixmap->setText("");
        ui.labelPixmap->setPixmap(ui.labelPixmap->style()->standardPixmap(QStyle::SP_MessageBoxCritical));

        ui.plainTextEdit->setPlainText(message.c_str());
        ui.textEdit->setText(source.c_str());

        setWindowTitle("Source Error");
}
}

namespace dialog
{
void message_source_error(QWidget* parent, const std::string& message, const std::string& source)
{
        QtObjectInDynamicMemory<message_source_error_implementation::SourceError> w(parent, message, source);
        w->exec();
}
}
