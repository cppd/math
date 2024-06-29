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

#include "model_tree_menu.h"

#include "model_tree_actions.h"

#include <src/gui/dialogs/message.h>
#include <src/model/object_id.h>

#include <QAction>
#include <QMenu>
#include <QObject>

#include <memory>

namespace ns::gui::main_window
{
std::unique_ptr<QMenu> make_model_tree_menu_for_object(
        ModelTreeActions* const actions,
        const model::ObjectId id,
        const bool visible)
{
        auto menu = std::make_unique<QMenu>();

        QAction* action = nullptr;

        action = menu->addAction("Show Only It");
        QObject::connect(
                action, &QAction::triggered,
                [=]()
                {
                        actions->show_only_it(id);
                });

        action = visible ? menu->addAction("Hide") : menu->addAction("Show");
        QObject::connect(
                action, &QAction::triggered,
                [=, show = !visible]()
                {
                        actions->show(id, show);
                });

        menu->addSeparator();

        action = menu->addAction("Delete");
        QObject::connect(
                action, &QAction::triggered,
                [=]()
                {
                        const auto yes = dialogs::message_question_default_no("Delete?");
                        if (yes && *yes)
                        {
                                actions->erase(id);
                        }
                });

        action = menu->addAction("Delete All");
        QObject::connect(
                action, &QAction::triggered,
                [=]()
                {
                        const auto yes = dialogs::message_question_default_no("Delete All?");
                        if (yes && *yes)
                        {
                                actions->clear();
                        }
                });

        return menu;
}
}
