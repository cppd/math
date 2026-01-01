/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "model_tree_style.h"

#include <QColor>
#include <QFont>
#include <QTreeWidgetItem>

namespace ns::gui::main_window
{
namespace
{
constexpr QColor COLOR_VISIBLE(0, 0, 0);
constexpr QColor COLOR_HIDDEN(128, 128, 128);
}

void set_model_tree_item_style(QTreeWidgetItem* const item, const bool visible)
{
        item->setForeground(0, visible ? COLOR_VISIBLE : COLOR_HIDDEN);
}

void set_model_tree_item_style_deleted(QTreeWidgetItem* const item)
{
        QFont font = item->font(0);
        font.setStrikeOut(true);
        item->setFont(0, font);
        set_model_tree_item_style(item, false);
}
}
