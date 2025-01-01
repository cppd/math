/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "ui_point_object.h"

#include <QDialog>
#include <QObject>

#include <optional>
#include <string>

namespace ns::gui::dialogs
{
struct PointObjectParameters final
{
        int point_count;
};

class PointObjectParametersDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::PointObjectParametersDialog ui_;

        const int min_point_count_;
        const int max_point_count_;

        std::optional<PointObjectParameters>* const parameters_;

        PointObjectParametersDialog(
                int dimension,
                const std::string& object_name,
                int default_point_count,
                int min_point_count,
                int max_point_count,
                std::optional<PointObjectParameters>* parameters);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<PointObjectParameters> show(
                int dimension,
                const std::string& object_name,
                int default_point_count,
                int min_point_count,
                int max_point_count);
};
}
