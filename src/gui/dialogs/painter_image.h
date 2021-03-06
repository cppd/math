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

#include "ui_painter_image.h"

#include <optional>

namespace ns::gui::dialog
{
enum class PainterImagePathType
{
        None,
        File,
        Directory
};

struct PainterImageParameters final
{
        std::optional<std::string> path_string;
        bool with_background;
        std::optional<bool> grayscale;
        bool convert_to_8_bit;
};

class PainterImageDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::PainterImageDialog ui;

        const PainterImagePathType m_path_type;

        std::optional<PainterImageParameters>& m_parameters;

        PainterImageDialog(
                const std::string& title,
                PainterImagePathType path_type,
                bool use_grayscale,
                std::optional<PainterImageParameters>& parameters);

        void set_path();
        void set_grayscale(bool use_grayscale);

        void done(int r) override;

        void on_select_path_clicked();
        void on_grayscale_toggled();

public:
        [[nodiscard]] static std::optional<PainterImageParameters> show(
                const std::string& title,
                PainterImagePathType path_type,
                bool use_grayscale);
};
}
