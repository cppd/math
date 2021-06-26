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

#include "actions.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/com/names.h>
#include <src/process/computing.h>
#include <src/process/exporting.h>
#include <src/process/loading.h>
#include <src/process/painting.h>
#include <src/process/testing.h>

namespace ns::gui::main_window
{
namespace
{
constexpr unsigned WORKER_THREAD_ID = 0;
constexpr unsigned SELF_TEST_THREAD_ID = 1;
constexpr unsigned REQUIRED_THREAD_COUNT = 2;

std::string action_name(const QAction* action)
{
        std::string s = action->text().toStdString();
        while (!s.empty() && s.back() == '.')
        {
                s.pop_back();
        }
        return s;
}

void load_point_mesh(
        WorkerThreads* threads,
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        WorkerThreads::Function f = process::action_load_point_mesh(repository, dimension, object_name);
                        //model_tree->clear();
                        //view->send(view::command::ResetView());
                        return f;
                });
}

void load_facet_mesh(
        WorkerThreads* threads,
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        WorkerThreads::Function f = process::action_load_facet_mesh(repository, dimension, object_name);
                        //model_tree->clear();
                        //view->send(view::command::ResetView());
                        return f;
                });
}

void load_mesh(
        WorkerThreads* threads,
        const std::filesystem::path& path,
        bool use_object_selection_dialog,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        WorkerThreads::Function f = process::action_load_mesh(path, use_object_selection_dialog);
                        //model_tree->clear();
                        //view->send(view::command::ResetView());
                        return f;
                });
}

void load_volume(
        WorkerThreads* threads,
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        return process::action_load_volume(repository, dimension, object_name);
                });
}

void load_volume(WorkerThreads* threads, const std::filesystem::path& path, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        return process::action_load_volume(path);
                });
}

void export_mesh(WorkerThreads* threads, const ModelTree* model_tree, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        std::optional<storage::MeshObjectConst> object = model_tree->current_mesh_const();
                        if (!object)
                        {
                                MESSAGE_WARNING("No mesh to export");
                                return WorkerThreads::Function();
                        }
                        return process::action_export(*object);
                });
}

void painter(
        WorkerThreads* threads,
        const ModelTree* model_tree,
        view::View* view,
        const ColorsWidget* colors,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        std::vector<storage::MeshObjectConst> objects = model_tree->const_mesh_objects();
                        if (objects.empty())
                        {
                                MESSAGE_WARNING("No objects to paint");
                                return WorkerThreads::Function();
                        }
                        view::info::Camera camera;
                        view->receive({&camera});
                        return process::action_painter(
                                objects, camera, colors->lighting_color(), colors->background_color());
                });
}

void bound_cocone(WorkerThreads* threads, const ModelTree* model_tree, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]()
                {
                        std::optional<storage::MeshObjectConst> object = model_tree->current_mesh_const();
                        if (!object)
                        {
                                MESSAGE_WARNING("No object to compute BoundCocone");
                                return WorkerThreads::Function();
                        }
                        return process::action_bound_cocone(*object);
                });
}

void self_test(WorkerThreads* threads, process::TestType test_type, const std::string& action)
{
        threads->terminate_and_start(
                SELF_TEST_THREAD_ID, action,
                [&]()
                {
                        return process::action_self_test(test_type);
                });
}
}

Actions::Actions(
        const CommandLineOptions& options,
        QStatusBar* status_bar,
        QAction* action_self_test,
        QMenu* menu_file,
        QMenu* menu_create,
        QMenu* menu_edit,
        QMenu* menu_rendering,
        const storage::Repository* repository,
        view::View* view,
        ModelTree* model_tree,
        const ColorsWidget* colors)
        : m_worker_threads(create_worker_threads(REQUIRED_THREAD_COUNT, SELF_TEST_THREAD_ID, status_bar))
{
        WorkerThreads* threads = m_worker_threads.get();

        {
                QAction* action = menu_file->addAction("Load Mesh...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=]()
                        {
                                load_mesh(threads, "", true, action_name(action));
                        }));
        }
        {
                QAction* action = menu_file->addAction("Load Volume...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=]()
                        {
                                load_volume(threads, "", action_name(action));
                        }));
        }
        {
                QAction* action = menu_file->addAction("Export...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=]()
                        {
                                export_mesh(threads, model_tree, action_name(action));
                        }));
        }
        {
                QAction* action = action_self_test;
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=]()
                        {
                                self_test(threads, process::TestType::SmallAndLarge, action_name(action));
                        }));
        }
        {
                QAction* action = menu_rendering->addAction("Painter...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=]()
                        {
                                painter(threads, model_tree, view, colors, action_name(action));
                        }));
        }
        {
                QAction* action = menu_edit->addAction("BoundCocone...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=]()
                        {
                                bound_cocone(threads, model_tree, action_name(action));
                        }));
        }

        std::vector<storage::Repository::ObjectNames> repository_objects = repository->object_names();
        std::sort(
                repository_objects.begin(), repository_objects.end(),
                [](const auto& a, const auto& b)
                {
                        return a.dimension < b.dimension;
                });
        for (storage::Repository::ObjectNames& objects : repository_objects)
        {
                const int dimension = objects.dimension;
                ASSERT(dimension > 0);

                QMenu* sub_menu = menu_create->addMenu(QString::fromStdString(space_name(dimension)));

                if (!objects.point_mesh_names.empty())
                {
                        if (!sub_menu->actions().isEmpty())
                        {
                                sub_menu->addSeparator();
                        }

                        std::sort(objects.point_mesh_names.begin(), objects.point_mesh_names.end());
                        for (const std::string& object_name : objects.point_mesh_names)
                        {
                                ASSERT(!object_name.empty());

                                QAction* action = sub_menu->addAction(QString::fromStdString(object_name + "..."));
                                m_connections.emplace_back(QObject::connect(
                                        action, &QAction::triggered,
                                        [=]()
                                        {
                                                load_point_mesh(
                                                        threads, repository, dimension, object_name,
                                                        action_name(action));
                                        }));
                        }
                }

                if (!objects.facet_mesh_names.empty())
                {
                        if (!sub_menu->actions().isEmpty())
                        {
                                sub_menu->addSeparator();
                        }

                        std::sort(objects.facet_mesh_names.begin(), objects.facet_mesh_names.end());
                        for (const std::string& object_name : objects.facet_mesh_names)
                        {
                                ASSERT(!object_name.empty());

                                QAction* action = sub_menu->addAction(QString::fromStdString(object_name + "..."));
                                m_connections.emplace_back(QObject::connect(
                                        action, &QAction::triggered,
                                        [=]()
                                        {
                                                load_facet_mesh(
                                                        threads, repository, dimension, object_name,
                                                        action_name(action));
                                        }));
                        }
                }

                if (!objects.volume_names.empty() && dimension == 3)
                {
                        if (!sub_menu->actions().isEmpty())
                        {
                                sub_menu->addSeparator();
                        }

                        std::sort(objects.volume_names.begin(), objects.volume_names.end());
                        for (const std::string& object_name : objects.volume_names)
                        {
                                ASSERT(!object_name.empty());

                                QAction* action = sub_menu->addAction(QString::fromStdString(object_name + "..."));
                                m_connections.emplace_back(QObject::connect(
                                        action, &QAction::triggered,
                                        [=]()
                                        {
                                                load_volume(
                                                        threads, repository, dimension, object_name,
                                                        action_name(action));
                                        }));
                        }
                }
        }

        self_test(m_worker_threads.get(), process::TestType::Small, "Self-Test");

        if (!options.file_name.empty())
        {
                load_mesh(m_worker_threads.get(), options.file_name, !options.no_object_selection_dialog, "Load Mesh");
        }
}

Actions::~Actions()
{
        m_worker_threads->terminate_all();
}

void Actions::set_progresses()
{
        m_worker_threads->set_progresses();
}
}
