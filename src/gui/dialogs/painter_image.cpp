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
        : QDialog(parent_for_dialog()), m_path_type(path_type), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle(QString::fromStdString(title));

        set_path();
        set_checkboxes(use_all);

        this->adjustSize();
}

void PainterImageDialog::set_path()
{
        switch (m_path_type)
        {
        case PainterImagePathType::None:
                ui.label_path_name->setVisible(false);
                ui.lineEdit_path->setVisible(false);
                ui.toolButton_select_path->setVisible(false);
                return;
        case PainterImagePathType::Directory:
                ui.label_path_name->setText("Directory:");
                ui.lineEdit_path->setReadOnly(true);
                connect(ui.toolButton_select_path, &QToolButton::clicked, this,
                        &PainterImageDialog::on_select_path_clicked);
                return;
        case PainterImagePathType::File:
                ui.label_path_name->setText("File:");
                ui.lineEdit_path->setReadOnly(true);
                connect(ui.toolButton_select_path, &QToolButton::clicked, this,
                        &PainterImageDialog::on_select_path_clicked);
                return;
        }
        error("Unknown path type " + to_string(static_cast<long long>(m_path_type)));
}

void PainterImageDialog::set_checkboxes(bool use_all)
{
        if (use_all)
        {
                ui.checkBox_all->setVisible(true);
                connect(ui.checkBox_all, &QCheckBox::toggled, this, &PainterImageDialog::on_all_toggled);
        }
        else
        {
                ui.checkBox_all->setVisible(false);
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

        if (m_path_type == PainterImagePathType::Directory)
        {
                path_string = ui.lineEdit_path->text().toStdString();
                std::filesystem::path path = path_from_utf8(*path_string);
                if (!std::filesystem::is_directory(path))
                {
                        std::string msg = "Directory is not selected";
                        dialog::message_critical(msg);
                        return;
                }
        }
        else if (m_path_type == PainterImagePathType::File)
        {
                path_string = ui.lineEdit_path->text().toStdString();
                std::filesystem::path path = path_from_utf8(*path_string);
                if (!std::filesystem::is_directory(path.parent_path()) || path.filename().empty())
                {
                        std::string msg = "File is not selected";
                        dialog::message_critical(msg);
                        return;
                }
        }

        m_parameters.emplace();

        m_parameters->path_string = path_string;

        if (ui.checkBox_all->isVisible() && ui.checkBox_all->isChecked())
        {
                m_parameters->all = true;
                m_parameters->with_background = false;
                m_parameters->convert_to_8_bit = false;
        }
        else
        {
                m_parameters->all = false;
                m_parameters->with_background = ui.checkBox_with_background->isChecked();
                m_parameters->convert_to_8_bit = ui.checkBox_8_bit->isChecked();
        }

        QDialog::done(r);
}

void PainterImageDialog::on_select_path_clicked()
{
        if (m_path_type == PainterImagePathType::None)
        {
                return;
        }

        QPointer ptr(this);
        std::optional<std::string> path;

        if (m_path_type == PainterImagePathType::Directory)
        {
                const std::string caption = "Directory";
                const bool read_only = false;
                path = dialog::select_directory(caption, read_only);
        }
        else if (m_path_type == PainterImagePathType::File)
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
                ui.lineEdit_path->setText(QString::fromStdString(*path));
        }
}

void PainterImageDialog::on_all_toggled()
{
        if (ui.checkBox_all->isChecked())
        {
                ui.checkBox_8_bit->setVisible(false);
                ui.checkBox_with_background->setVisible(false);
        }
        else
        {
                ui.checkBox_8_bit->setVisible(true);
                ui.checkBox_with_background->setVisible(true);
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
