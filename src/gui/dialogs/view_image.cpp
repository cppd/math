/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "view_image.h"

#include "file_dialog.h"
#include "message.h"

#include "../com/support.h"

#include <src/com/file/path.h>
#include <src/com/string/ascii.h>
#include <src/image/file.h>

#include <algorithm>

namespace ns::gui::dialog
{
namespace
{
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif
ViewImageParameters g_parameters{.path_string = {}, .normalize = false, .convert_to_8_bit = false};
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

ViewImageParameters read_current()
{
        return g_parameters;
}

void write_current(ViewImageParameters parameters)
{
        parameters.path_string = generic_utf8_filename(path_from_utf8(parameters.path_string).remove_filename());
        g_parameters = parameters;
}

std::string initial_path_string(const ViewImageParameters& input, const std::string& file_name)
{
        const std::filesystem::path file_path = path_from_utf8(file_name);

        if (file_path.empty())
        {
                return {};
        }

        if (!file_path.has_filename())
        {
                error("No file name in file name string " + file_name);
        }

        if (!std::filesystem::path(file_path).remove_filename().empty())
        {
                return file_name;
        }

        const std::filesystem::path previous_parent_path = path_from_utf8(input.path_string);

        if (previous_parent_path.empty())
        {
                return {};
        }

        ASSERT(std::filesystem::path(previous_parent_path).filename().empty());
        return generic_utf8_filename(previous_parent_path / file_path);
}

bool has_directory_and_filename(const std::string& file_name)
{
        std::filesystem::path path = path_from_utf8(file_name);
        if (path.filename().empty())
        {
                return false;
        }
        return std::filesystem::is_directory(path.remove_filename());
}

void check_print_characters(const std::string& s)
{
        if (!std::all_of(
                    s.begin(), s.end(),
                    [](char c)
                    {
                            return ascii::is_print(c) || !ascii::is_not_new_line(c);
                    }))
        {
                error("Information string has unsupported characters " + s);
        }
}
}

ViewImageDialog::ViewImageDialog(
        const ViewImageParameters& input,
        const std::string& title,
        const std::string& info,
        const std::string& file_name,
        std::optional<ViewImageParameters>& parameters)
        : QDialog(parent_for_dialog()), file_name_(file_name), parameters_(parameters)
{
        ui.setupUi(this);
        setWindowTitle(QString::fromStdString(title));

        ui.checkBox_normalize->setChecked(input.normalize);
        ui.checkBox_8_bit->setChecked(input.convert_to_8_bit);

        if (!info.empty())
        {
                check_print_characters(info);
                ui.label_info->setText(QString::fromStdString(info));
        }
        else
        {
                ui.label_info->setVisible(false);
                ui.line->setVisible(false);
        }

        ui.lineEdit_path->setReadOnly(true);
        ui.lineEdit_path->setText(QString::fromStdString(initial_path_string(input, file_name)));

        connect(ui.toolButton_select_path, &QToolButton::clicked, this, &ViewImageDialog::on_select_path_clicked);

        this->adjustSize();
}

void ViewImageDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        std::string path_string = ui.lineEdit_path->text().toStdString();

        if (!has_directory_and_filename(path_string))
        {
                std::string msg = "File is not selected";
                dialog::message_critical(msg);
                return;
        }

        parameters_.emplace();

        parameters_->path_string = std::move(path_string);
        parameters_->normalize = ui.checkBox_normalize->isChecked();
        parameters_->convert_to_8_bit = ui.checkBox_8_bit->isChecked();

        QDialog::done(r);
}

void ViewImageDialog::on_select_path_clicked()
{
        QPointer ptr(this);

        const std::string caption = "File";
        dialog::FileFilter filter;
        filter.name = "Images";
        filter.file_extensions.emplace_back(image::file_extension());
        constexpr bool read_only = true;

        const std::string file_name =
                generic_utf8_filename(path_from_utf8(ui.lineEdit_path->text().toStdString()).filename());

        std::optional<std::string> path =
                dialog::save_file(caption, file_name.empty() ? file_name_ : file_name, {filter}, read_only);

        if (path && !ptr.isNull())
        {
                ui.lineEdit_path->setText(QString::fromStdString(*path));
        }
}

std::optional<ViewImageParameters> ViewImageDialog::show(
        const std::string& title,
        const std::string& info,
        const std::string& file_name)
{
        std::optional<ViewImageParameters> parameters;

        QtObjectInDynamicMemory w(new ViewImageDialog(read_current(), title, info, file_name, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        write_current(*parameters);

        return parameters;
}
}
