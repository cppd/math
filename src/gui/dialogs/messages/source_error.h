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

#pragma once

#include "ui_source_error.h"

#include <string>

namespace message_source_error_implementation
{
class SourceError final : public QDialog
{
        Q_OBJECT

public:
        SourceError(QWidget* parent, const std::string& message, const std::string& source);

private:
        Ui::SourceError ui;
};
}

namespace dialog
{
void message_source_error(QWidget* parent, const std::string& message, const std::string& source);
}
