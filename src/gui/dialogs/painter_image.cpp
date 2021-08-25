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

#include "painter_image.h"

#include "file_dialog.h"
#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/image/file.h>

namespace ns::gui::dialog
{
PainterImageDialog::PainterImageDialog(
        const std::string& title,
        PainterImagePathType path_type,
        bool use_all,
        std::optional<PainterImageParameters>& parameters)
        : QDialog(parent_for_dialog()), path_type_(path_type), parameters_(parameters)
{
        ui_.setupUi(this);
        setWindowTitle(QString::fromStdString(title));

        set_path();
        set_checkboxes(use_all);

        this->adjustSize();
}

void PainterImageDialog::set_path()
{
        switch (path_type_)
        {
        case PainterImagePathType::NONE:
                ui_.label_path_name->setVisible(false);
                ui_.lineEdit_path->setVisible(false);
                ui_.toolButton_select_path->setVisible(false);
                return;
        case PainterImagePathType::DIRECTORY:
                ui_.label_path_name->setText("Directory:");
                ui_.lineEdit_path->setReadOnly(true);
                connect(ui_.toolButton_select_path, &QToolButton::clicked, this,
                        &PainterImageDialog::on_select_path_clicked);
                return;
        case PainterImagePathType::FILE:
                ui_.label_path_name->setText("File:");
                ui_.lineEdit_path->setReadOnly(true);
                connect(ui_.toolButton_select_path, &QToolButton::clicked, this,
                        &PainterImageDialog::on_select_path_clicked);
                return;
        }
        error("Unknown path type " + to_string(static_cast<long long>(path_type_)));
}

void PainterImageDialog::set_checkboxes(bool use_all)
{
        if (use_all)
        {
                ui_.checkBox_all->setVisible(true);
                connect(ui_.checkBox_all, &QCheckBox::toggled, this, &PainterImageDialog::on_all_toggled);
        }
        else
        {
                ui_.checkBox_all->setVisible(false);
        }
}

void PainterImageDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        std::optional<std::string> path_string;

        if (path_type_ == PainterImagePathType::DIRECTORY)
        {
                path_string = ui_.lineEdit_path->text().toStdString();
                std::filesystem::path path = path_from_utf8(*path_string);
                if (!std::filesystem::is_directory(path))
                {
                        std::string msg = "Directory is not selected";
                        dialog::message_critical(msg);
                        return;
                }
        }
        else if (path_type_ == PainterImagePathType::FILE)
        {
                path_string = ui_.lineEdit_path->text().toStdString();
                std::filesystem::path path = path_from_utf8(*path_string);
                if (!std::filesystem::is_directory(path.parent_path()) || path.filename().empty())
                {
                        std::string msg = "File is not selected";
                        dialog::message_critical(msg);
                        return;
                }
        }

        parameters_.emplace();

        parameters_->path_string = path_string;

        if (ui_.checkBox_all->isVisible() && ui_.checkBox_all->isChecked())
        {
                parameters_->all = true;
                parameters_->with_background = false;
                parameters_->convert_to_8_bit = false;
        }
        else
        {
                parameters_->all = false;
                parameters_->with_background = ui_.checkBox_with_background->isChecked();
                parameters_->convert_to_8_bit = ui_.checkBox_8_bit->isChecked();
        }

        QDialog::done(r);
}

void PainterImageDialog::on_select_path_clicked()
{
        if (path_type_ == PainterImagePathType::NONE)
        {
                return;
        }

        QPointer ptr(this);
        std::optional<std::string> path;

        if (path_type_ == PainterImagePathType::DIRECTORY)
        {
                const std::string caption = "Directory";
                const bool read_only = false;
                path = dialog::select_directory(caption, read_only);
        }
        else if (path_type_ == PainterImagePathType::FILE)
        {
                const std::string caption = "File";
                dialog::FileFilter filter;
                filter.name = "Images";
                filter.file_extensions.emplace_back(image::file_extension());
                const bool read_only = true;
                path = dialog::save_file(caption, {filter}, read_only);
        }
        else
        {
                return;
        }

        if (path && !ptr.isNull())
        {
                ui_.lineEdit_path->setText(QString::fromStdString(*path));
        }
}

void PainterImageDialog::on_all_toggled()
{
        if (ui_.checkBox_all->isChecked())
        {
                ui_.checkBox_8_bit->setVisible(false);
                ui_.checkBox_with_background->setVisible(false);
        }
        else
        {
                ui_.checkBox_8_bit->setVisible(true);
                ui_.checkBox_with_background->setVisible(true);
        }
}

std::optional<PainterImageParameters> PainterImageDialog::show(
        const std::string& title,
        PainterImagePathType path_type,
        bool use_all)
{
        std::optional<PainterImageParameters> parameters;

        QtObjectInDynamicMemory w(new PainterImageDialog(title, path_type, use_all, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
