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

#include "file_dialog.h"

#include "../../support/support.h"

#include <src/com/error.h>

#include <QFileDialog>

namespace
{
bool exec_dialog_for_single_file(QtObjectInDynamicMemory<QFileDialog>* w, std::string* name)
{
        ASSERT(!w->isNull());
        ASSERT(name);

        if (!(*w)->exec())
        {
                return false;
        }

        if (w->isNull())
        {
                return false;
        }

        QStringList list = (*w)->selectedFiles();
        if (list.size() != 1)
        {
                error("QFileDialog selected file count (" + to_string(list.size()) + ") is not equal to 1.");
        }

        *name = list[0].toStdString();

        return true;
}

QFileDialog::Options make_options(bool read_only)
{
        QFileDialog::Options options = QFileDialog::DontUseNativeDialog;

        if (read_only)
        {
                options |= QFileDialog::ReadOnly;
        }

        return options;
}

std::string join_filters(const std::vector<std::string>& filters)
{
        std::string filter_string;
        for (const std::string& filter : filters)
        {
                if (!filter_string.empty())
                {
                        filter_string += ";;";
                }
                filter_string += filter;
        }
        return filter_string;
}
}

namespace dialog
{
bool save_file(
        QWidget* parent,
        const std::string& caption,
        const std::vector<std::string>& filters,
        bool read_only,
        std::string* name)
{
        QtObjectInDynamicMemory<QFileDialog> w(
                parent, QString::fromStdString(caption), QString(), QString::fromStdString(join_filters(filters)));

        w->setOptions(make_options(read_only));
        w->setAcceptMode(QFileDialog::AcceptSave);
        w->setFileMode(QFileDialog::AnyFile);

        return exec_dialog_for_single_file(&w, name);
}

bool open_file(
        QWidget* parent,
        const std::string& caption,
        const std::vector<std::string>& filters,
        bool read_only,
        std::string* name)
{
        QtObjectInDynamicMemory<QFileDialog> w(
                parent, QString::fromStdString(caption), QString(), QString::fromStdString(join_filters(filters)));

        w->setOptions(make_options(read_only));
        w->setAcceptMode(QFileDialog::AcceptOpen);
        w->setFileMode(QFileDialog::ExistingFile);

        return exec_dialog_for_single_file(&w, name);
}
}
