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

#include "actions_repository.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/gui/com/connection.h>
#include <src/gui/com/threads.h>
#include <src/process/loading.h>
#include <src/storage/repository.h>

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QString>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace ns::gui::main_window
{
namespace
{
std::string action_name(const QAction& action)
{
        std::string s = action.text().toStdString();
        while (!s.empty() && s.back() == '.')
        {
                s.pop_back();
        }
        return s;
}

void load_point_mesh(
        const unsigned thread_id,
        com::WorkerThreads* const threads,
        const storage::Repository* const repository,
        const int dimension,
        const std::string& object_name,
        const std::string& action)
{
        threads->terminate_and_start(
                thread_id, action,
                [&]
                {
                        com::WorkerThreads::Function f =
                                process::action_load_point_mesh(repository, dimension, object_name);
                        // model_tree->clear();
                        // view->send(view::command::ResetView());
                        return f;
                });
}

void load_facet_mesh(
        const unsigned thread_id,
        com::WorkerThreads* const threads,
        const storage::Repository* const repository,
        const int dimension,
        const std::string& object_name,
        const std::string& action)
{
        threads->terminate_and_start(
                thread_id, action,
                [&]
                {
                        com::WorkerThreads::Function f =
                                process::action_load_facet_mesh(repository, dimension, object_name);
                        // model_tree->clear();
                        // view->send(view::command::ResetView());
                        return f;
                });
}

void load_volume(
        const unsigned thread_id,
        com::WorkerThreads* const threads,
        const storage::Repository* const repository,
        const int dimension,
        const std::string& object_name,
        const std::string& action)
{
        threads->terminate_and_start(
                thread_id, action,
                [&]
                {
                        return process::action_load_volume(repository, dimension, object_name);
                });
}

void create_point_mesh_menu(
        const unsigned thread_id,
        const int dimension,
        std::vector<std::string>&& object_names,
        std::vector<com::Connection>* const connections,
        com::WorkerThreads* const threads,
        QMenu* const menu,
        const storage::Repository* const repository)
{
        if (object_names.empty())
        {
                return;
        }

        if (!menu->actions().isEmpty())
        {
                menu->addSeparator();
        }

        std::ranges::sort(object_names);
        for (const std::string& object_name : object_names)
        {
                ASSERT(!object_name.empty());

                const QAction* const action = menu->addAction(QString::fromStdString(object_name + "..."));
                connections->emplace_back(
                        QObject::connect(
                                action, &QAction::triggered,
                                [=]
                                {
                                        load_point_mesh(
                                                thread_id, threads, repository, dimension, object_name,
                                                action_name(*action));
                                }));
        }
}

void create_facet_mesh_menu(
        const unsigned thread_id,
        const int dimension,
        std::vector<std::string>&& object_names,
        std::vector<com::Connection>* const connections,
        com::WorkerThreads* const threads,
        QMenu* const menu,
        const storage::Repository* const repository)
{
        if (object_names.empty())
        {
                return;
        }

        if (!menu->actions().isEmpty())
        {
                menu->addSeparator();
        }

        std::ranges::sort(object_names);
        for (const std::string& object_name : object_names)
        {
                ASSERT(!object_name.empty());

                const QAction* const action = menu->addAction(QString::fromStdString(object_name + "..."));
                connections->emplace_back(
                        QObject::connect(
                                action, &QAction::triggered,
                                [=]
                                {
                                        load_facet_mesh(
                                                thread_id, threads, repository, dimension, object_name,
                                                action_name(*action));
                                }));
        }
}

void create_volume_menu(
        const unsigned thread_id,
        const int dimension,
        std::vector<std::string>&& object_names,
        std::vector<com::Connection>* const connections,
        com::WorkerThreads* const threads,
        QMenu* const menu,
        const storage::Repository* const repository)
{
        if (object_names.empty() || dimension != 3)
        {
                return;
        }

        if (!menu->actions().isEmpty())
        {
                menu->addSeparator();
        }

        std::ranges::sort(object_names);
        for (const std::string& object_name : object_names)
        {
                ASSERT(!object_name.empty());

                const QAction* const action = menu->addAction(QString::fromStdString(object_name + "..."));
                connections->emplace_back(
                        QObject::connect(
                                action, &QAction::triggered,
                                [=]
                                {
                                        load_volume(
                                                thread_id, threads, repository, dimension, object_name,
                                                action_name(*action));
                                }));
        }
}
}

void create_repository_menu(
        const unsigned thread_id,
        std::vector<com::Connection>* const connections,
        com::WorkerThreads* const threads,
        QMenu* const menu_create,
        const storage::Repository* const repository)
{
        std::vector<storage::Repository::ObjectNames> repository_objects = repository->object_names();

        std::ranges::sort(
                repository_objects,
                [](const auto& a, const auto& b)
                {
                        return a.dimension < b.dimension;
                });

        for (storage::Repository::ObjectNames& objects : repository_objects)
        {
                QMenu* const sub_menu = menu_create->addMenu(QString::fromStdString(space_name(objects.dimension)));

                create_point_mesh_menu(
                        thread_id, objects.dimension, std::move(objects.point_mesh_names), connections, threads,
                        sub_menu, repository);

                create_facet_mesh_menu(
                        thread_id, objects.dimension, std::move(objects.facet_mesh_names), connections, threads,
                        sub_menu, repository);

                create_volume_menu(
                        thread_id, objects.dimension, std::move(objects.volume_names), connections, threads, sub_menu,
                        repository);
        }
}
}
