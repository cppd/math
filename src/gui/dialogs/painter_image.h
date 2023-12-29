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

#pragma once

#include "ui_painter_image.h"

#include <QDialog>
#include <QObject>
#include <optional>
#include <string>

namespace ns::gui::dialog
{
enum class PainterImagePathType
{
        NONE,
        FILE,
        DIRECTORY
};

struct PainterImageParameters final
{
        std::optional<std::string> path_string;
        bool all;
        bool with_background;
        bool convert_to_8_bit;
};

class PainterImageDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::PainterImageDialog ui_;

        const PainterImagePathType path_type_;

        std::optional<PainterImageParameters>* const parameters_;

        PainterImageDialog(
                const std::string& title,
                PainterImagePathType path_type,
                bool use_all,
                std::optional<PainterImageParameters>* parameters);

        void set_path();
        void set_checkboxes(bool use_all);

        void done(int r) override;

        void on_select_path_clicked();
        void on_all_toggled();

public:
        [[nodiscard]] static std::optional<PainterImageParameters> show(
                const std::string& title,
                PainterImagePathType path_type,
                bool use_all);
};
}
