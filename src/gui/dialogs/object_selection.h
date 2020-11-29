/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <unordered_set>
#include <vector>

namespace gui::dialog
{
struct ObjectSelection final
{
        enum class Type
        {
                BoundCocone,
                Cocone,
                ConvexHull,
                Mst
        };
        std::unordered_set<Type> types;
};

class ObjectSelectionDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::ObjectSelectionDialog ui;

        std::vector<QCheckBox*> m_boxes;

        std::optional<ObjectSelection>& m_parameters;

        ObjectSelectionDialog(const ObjectSelection& input, std::optional<ObjectSelection>& parameters);
        void set_all(bool checked);
        void done(int r) override;

public:
        [[nodiscard]] static std::optional<ObjectSelection> show();
        [[nodiscard]] static ObjectSelection current();
};
}
