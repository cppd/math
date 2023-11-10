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

#include "ui_object_selection.h"

#include <optional>

namespace ns::gui::dialog
{
struct ObjectSelectionParameters final
{
        bool bound_cocone;
        bool cocone;
        bool convex_hull;
        bool mst;
};

class ObjectSelectionParametersDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::ObjectSelectionParametersDialog ui_;

        std::optional<ObjectSelectionParameters>* const parameters_;

        ObjectSelectionParametersDialog(
                const ObjectSelectionParameters& input,
                std::optional<ObjectSelectionParameters>* parameters);

        void set_all(bool checked);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<ObjectSelectionParameters> show();
        [[nodiscard]] static ObjectSelectionParameters current();
};
}
