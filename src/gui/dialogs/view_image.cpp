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

#include "view_image.h"

#include "file_dialog.h"
#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/string/ascii.h>
#include <src/image/file_save.h>

#include <algorithm>

namespace ns::gui::dialog
{
namespace
{
class DialogParameters final
{
        ViewImageParameters parameters_{.path_string = {}, .normalize = false, .convert_to_8_bit = false};

public:
        [[nodiscard]] ViewImageParameters read() const
        {
                return parameters_;
        }

        void write(ViewImageParameters parameters)
        {
                parameters.path_string =
                        generic_utf8_filename(path_from_utf8(parameters.path_string).remove_filename());
                parameters_ = std::move(parameters);
        }
};

DialogParameters& dialog_parameters()
{
        static DialogParameters parameters;
        return parameters;
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

void set_line_edit_width(QLineEdit* const line_edit)
{
        line_edit->setMinimumWidth(line_edit->fontMetrics().boundingRect(QString(75, 'a')).width());
}
}

ViewImageDialog::ViewImageDialog(
        const ViewImageParameters& input,
        const std::string& title,
        const std::string& info,
        const std::string* const file_name,
        std::optional<ViewImageParameters>* const parameters)
        : QDialog(parent_for_dialog()),
          file_name_(file_name),
          parameters_(parameters)
{
        ui_.setupUi(this);
        setWindowTitle(QString::fromStdString(title));

        ui_.check_box_normalize->setChecked(input.normalize);
        ui_.check_box_8_bit->setChecked(input.convert_to_8_bit);

        if (!info.empty())
        {
                check_print_characters(info);
                ui_.label_info->setText(QString::fromStdString(info));
        }
        else
        {
                ui_.label_info->setVisible(false);
                ui_.line->setVisible(false);
        }

        ui_.line_edit_path->setReadOnly(true);
        ui_.line_edit_path->setText(QString::fromStdString(initial_path_string(input, *file_name)));

        connect(ui_.tool_button_select_path, &QToolButton::clicked, this, &ViewImageDialog::on_select_path_clicked);

        set_line_edit_width(ui_.line_edit_path);

        set_dialog_height(this);
}

void ViewImageDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        std::string path_string = ui_.line_edit_path->text().toStdString();

        if (!has_directory_and_filename(path_string))
        {
                const std::string msg = "File is not selected";
                dialog::message_critical(msg);
                return;
        }

        auto& parameters = parameters_->emplace();
        parameters.path_string = std::move(path_string);
        parameters.normalize = ui_.check_box_normalize->isChecked();
        parameters.convert_to_8_bit = ui_.check_box_8_bit->isChecked();

        QDialog::done(r);
}

void ViewImageDialog::on_select_path_clicked()
{
        const QPointer ptr(this);

        const std::string caption = "File";
        dialog::FileFilter filter;
        filter.name = "Images";
        filter.file_extensions.emplace_back(image::save_file_extension());
        constexpr bool READ_ONLY = true;

        const std::string file_name =
                generic_utf8_filename(path_from_utf8(ui_.line_edit_path->text().toStdString()).filename());

        std::optional<std::string> path =
                dialog::save_file(caption, file_name.empty() ? *file_name_ : file_name, {filter}, READ_ONLY);

        if (path && !ptr.isNull())
        {
                ui_.line_edit_path->setText(QString::fromStdString(*path));
        }
}

std::optional<ViewImageParameters> ViewImageDialog::show(
        const std::string& title,
        const std::string& info,
        const std::string& file_name)
{
        std::optional<ViewImageParameters> parameters;

        const QtObjectInDynamicMemory w(
                new ViewImageDialog(dialog_parameters().read(), title, info, &file_name, &parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        dialog_parameters().write(*parameters);

        return parameters;
}
}
