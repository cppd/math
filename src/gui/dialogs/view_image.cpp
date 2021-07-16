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
#include <src/image/file.h>

namespace ns::gui::dialog
{
ViewImageDialog::ViewImageDialog(
        const std::string& title,
        bool use_convert_to_8_bit,
        std::optional<ViewImageParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle(QString::fromStdString(title));

        ui.checkBox_8_bit->setVisible(use_convert_to_8_bit);

        ui.label_path_name->setText("File:");
        ui.lineEdit_path->setReadOnly(true);
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
        std::filesystem::path path = path_from_utf8(path_string);
        if (!std::filesystem::is_directory(path.parent_path()) || path.filename().empty())
        {
                std::string msg = "File is not selected";
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();

        m_parameters->path_string = std::move(path_string);

        if (ui.checkBox_8_bit->isVisible())
        {
                m_parameters->convert_to_8_bit = ui.checkBox_8_bit->isChecked();
        }

        QDialog::done(r);
}

void ViewImageDialog::on_select_path_clicked()
{
        QPointer ptr(this);

        const std::string caption = "File";
        dialog::FileFilter filter;
        filter.name = "Images";
        filter.file_extensions.emplace_back(image::file_extension());
        const bool read_only = true;
        std::optional<std::string> path = dialog::save_file(caption, {filter}, read_only);

        if (path && !ptr.isNull())
        {
                ui.lineEdit_path->setText(QString::fromStdString(*path));
        }
}

std::optional<ViewImageParameters> ViewImageDialog::show(const std::string& title, bool use_convert_to_8_bit)
{
        std::optional<ViewImageParameters> parameters;

        QtObjectInDynamicMemory w(new ViewImageDialog(title, use_convert_to_8_bit, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
