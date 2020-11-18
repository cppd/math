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

#include "../com/support.h"

#include <src/com/error.h>

#include <QFileDialog>
#include <map>

namespace gui::dialog
{
namespace
{
std::optional<std::string> exec_dialog_for_single_file(QtObjectInDynamicMemory<QFileDialog>* w)
{
        ASSERT(!w->isNull());

        if (!(*w)->exec() || w->isNull())
        {
                return std::nullopt;
        }

        QStringList list = (*w)->selectedFiles();
        if (list.size() != 1)
        {
                error("QFileDialog selected item count (" + to_string(list.size()) + ") is not equal to 1.");
        }

        return list[0].toStdString();
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

template <typename... T>
QString file_filter(const std::string& name, const T&... extensions)
{
        static_assert(sizeof...(T) > 0);

        if (name.empty())
        {
                error("No filter file name");
        }

        std::string filter;

        filter += name + " (";

        bool first = true;

        auto add_string = [&](const std::string& ext) {
                if (std::count(ext.cbegin(), ext.cend(), '*') > 0)
                {
                        error("Character * in file filter extension " + ext);
                }
                if (!first)
                {
                        filter += " ";
                }
                first = false;
                filter += "*." + ext;
        };

        auto add = [&](const auto& ext) {
                if constexpr (has_begin_end<decltype(ext)>)
                {
                        if constexpr (!std::is_same_v<char, std::remove_cvref_t<decltype(*std::cbegin(ext))>>)
                        {
                                for (const std::string& e : ext)
                                {
                                        add_string(e);
                                }
                        }
                        else
                        {
                                add_string(ext);
                        }
                }
                else
                {
                        add_string(ext);
                }
        };

        (add(extensions), ...);

        if (first)
        {
                error("No file filter extensions");
        }

        filter += ")";

        return QString::fromStdString(filter);
}

QString join_filters(const std::vector<QString>& filters)
{
        QString filter_string;
        for (const QString& filter : filters)
        {
                if (!filter_string.isEmpty())
                {
                        filter_string += ";;";
                }
                filter_string += filter;
        }
        return filter_string;
}

void check_filter(const FileFilter& filter)
{
        if (filter.name.empty())
        {
                error("File filter has no name");
        }
        if (filter.file_extensions.empty())
        {
                error("File filter has no file expensions");
        }
        for (const std::string& s : filter.file_extensions)
        {
                if (s.empty())
                {
                        error("File filter extension is empty");
                }
        }
}
}

std::optional<std::string> save_file(const std::string& caption, const std::vector<FileFilter>& filters, bool read_only)
{
        std::map<QString, QString> map;
        std::vector<QString> dialog_filters;
        for (const FileFilter& v : filters)
        {
                check_filter(v);
                const QString filter = file_filter(v.name, v.file_extensions);
                dialog_filters.push_back(filter);
                map.emplace(filter, QString::fromStdString(v.file_extensions.front()));
        }

        QtObjectInDynamicMemory<QFileDialog> w(
                parent_for_dialog(), QString::fromStdString(caption), QString(), join_filters(dialog_filters));

        QObject::connect(w.data(), &QFileDialog::filterSelected, [&](const QString& filter) {
                ASSERT(map.count(filter) == 1);
                w->setDefaultSuffix(map[filter]);
        });

        if (!dialog_filters.empty())
        {
                Q_EMIT w->filterSelected(dialog_filters.front());
        }

        w->setOptions(make_options(read_only));
        w->setAcceptMode(QFileDialog::AcceptSave);
        w->setFileMode(QFileDialog::AnyFile);

        return exec_dialog_for_single_file(&w);
}

std::optional<std::string> open_file(const std::string& caption, const std::vector<FileFilter>& filters, bool read_only)
{
        std::vector<QString> dialog_filters;
        for (const FileFilter& v : filters)
        {
                check_filter(v);
                dialog_filters.push_back(file_filter(v.name, v.file_extensions));
        }

        QtObjectInDynamicMemory<QFileDialog> w(
                parent_for_dialog(), QString::fromStdString(caption), QString(), join_filters(dialog_filters));

        w->setOptions(make_options(read_only));
        w->setAcceptMode(QFileDialog::AcceptOpen);
        w->setFileMode(QFileDialog::ExistingFile);

        return exec_dialog_for_single_file(&w);
}

std::optional<std::string> select_directory(const std::string& caption, bool read_only)
{
        QtObjectInDynamicMemory<QFileDialog> w(parent_for_dialog(), QString::fromStdString(caption));

        w->setOptions(make_options(read_only));
        w->setAcceptMode(QFileDialog::AcceptOpen);
        w->setFileMode(QFileDialog::Directory);

        return exec_dialog_for_single_file(&w);
}
}
