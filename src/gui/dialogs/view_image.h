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

#pragma once

#include "ui_view_image.h"

#include <optional>
#include <string>

namespace ns::gui::dialog
{
struct ViewImageParameters final
{
        std::string path_string;
        std::optional<bool> convert_to_8_bit;
};

class ViewImageDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::ViewImageDialog ui;

        std::optional<ViewImageParameters>& m_parameters;

        ViewImageDialog(const std::string& title, bool use_to_8_bit, std::optional<ViewImageParameters>& parameters);

        void done(int r) override;

        void on_select_path_clicked();

public:
        [[nodiscard]] static std::optional<ViewImageParameters> show(
                const std::string& title,
                bool use_convert_to_8_bit);
};
}
