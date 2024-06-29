/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "ui_view_image.h"

#include <QDialog>
#include <QObject>

#include <optional>
#include <string>

namespace ns::gui::dialogs
{
struct ViewImageParameters final
{
        std::string path_string;
        bool normalize;
        bool convert_to_8_bit;
};

class ViewImageDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::ViewImageDialog ui_;

        const std::string* const file_name_;

        std::optional<ViewImageParameters>* const parameters_;

        ViewImageDialog(
                const ViewImageParameters& input,
                const std::string& title,
                const std::string& info,
                const std::string* file_name,
                std::optional<ViewImageParameters>* parameters);

        void done(int r) override;

        void on_select_path_clicked();

public:
        [[nodiscard]] static std::optional<ViewImageParameters> show(
                const std::string& title,
                const std::string& info,
                const std::string& file_name);
};
}
