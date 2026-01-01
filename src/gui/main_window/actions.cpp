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

#include "actions.h"

#include "actions_repository.h"
#include "colors_widget.h"
#include "lighting_widget.h"
#include "model_tree.h"

#include <src/com/message.h>
#include <src/gui/com/command_line.h>
#include <src/gui/com/connection.h>
#include <src/gui/com/threads.h>
#include <src/process/computing.h>
#include <src/process/loading.h>
#include <src/process/painting.h>
#include <src/process/saving.h>
#include <src/process/testing.h>
#include <src/storage/repository.h>
#include <src/storage/types.h>
#include <src/view/event.h>
#include <src/view/view.h>

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QStatusBar>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ns::gui::main_window
{
namespace
{
enum ThreadId
{
        WORKER_THREAD_ID,
        SAVE_THREAD_ID,
        SELF_TEST_THREAD_ID,
        THREAD_ID_COUNT
};

std::string action_name(const QAction& action)
{
        std::string s = action.text().toStdString();
        while (!s.empty() && s.back() == '.')
        {
                s.pop_back();
        }
        return s;
}

void load_mesh(
        com::WorkerThreads* const threads,
        const std::filesystem::path& path,
        const bool use_object_selection_dialog,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]
                {
                        com::WorkerThreads::Function f = process::action_load_mesh(path, use_object_selection_dialog);
                        // model_tree->clear();
                        // view->send(view::command::ResetView());
                        return f;
                });
}

void load_volume(com::WorkerThreads* const threads, const std::filesystem::path& path, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]
                {
                        return process::action_load_volume(path);
                });
}

void save_mesh(com::WorkerThreads* const threads, const ModelTree* const model_tree, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]
                {
                        std::optional<storage::MeshObjectConst> object = model_tree->current_mesh_const();
                        if (!object)
                        {
                                message_warning("No mesh to save");
                                return com::WorkerThreads::Function();
                        }
                        return process::action_save(*object);
                });
}

void save_view_image(com::WorkerThreads* const threads, view::View* const view, const std::string& action)
{
        threads->terminate_and_start(
                SAVE_THREAD_ID, action,
                [&]
                {
                        std::optional<view::info::Image> image;
                        view->receive({&image});
                        if (!image)
                        {
                                message_error("Failed to receive view image");
                                return com::WorkerThreads::Function();
                        }

                        return process::action_save(std::chrono::system_clock::now(), std::move(image->image));
                });
}

void painter(
        com::WorkerThreads* const threads,
        const ModelTree* const model_tree,
        view::View* const view,
        const LightingWidget* const lighting,
        const ColorsWidget* const colors,
        const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]
                {
                        const std::vector<storage::MeshObjectConst> objects = model_tree->const_mesh_objects();
                        if (objects.empty())
                        {
                                message_warning("No objects to paint");
                                return com::WorkerThreads::Function();
                        }

                        std::optional<view::info::Camera> camera;
                        std::optional<view::info::ClipPlane> clip_plane;
                        view->receive({&camera, &clip_plane});
                        if (!camera || !clip_plane)
                        {
                                message_error("Failed to receive view information");
                                return com::WorkerThreads::Function();
                        }

                        return process::action_painter(
                                objects, *camera, *clip_plane, lighting->front_lighting_proportion(), lighting->color(),
                                colors->background_color());
                });
}

void bound_cocone(com::WorkerThreads* const threads, const ModelTree* const model_tree, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]
                {
                        std::optional<storage::MeshObjectConst> object = model_tree->current_mesh_const();
                        if (!object)
                        {
                                message_warning("No object to compute BoundCocone");
                                return com::WorkerThreads::Function();
                        }
                        return process::action_bound_cocone(*object);
                });
}

void volume_3d_slice(com::WorkerThreads* const threads, const ModelTree* const model_tree, const std::string& action)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, action,
                [&]
                {
                        std::optional<storage::VolumeObjectConst> object = model_tree->current_volume_const();
                        if (!object)
                        {
                                message_warning("No volume object");
                                return com::WorkerThreads::Function();
                        }
                        return process::action_3d_slice(*object);
                });
}

void self_test(com::WorkerThreads* const threads, const process::TestType test_type, const std::string& action)
{
        threads->terminate_and_start(
                SELF_TEST_THREAD_ID, action,
                [&]
                {
                        return process::action_self_test(test_type);
                });
}

void create_menu(
        std::vector<com::Connection>* const connections,
        com::WorkerThreads* const threads,
        QAction* const action_self_test,
        QAction* const action_benchmark,
        QMenu* const menu_file,
        QMenu* const menu_edit,
        QMenu* const menu_rendering,
        view::View* const view,
        ModelTree* const model_tree,
        const LightingWidget* const lighting,
        const ColorsWidget* const colors)
{
        const QAction* action = nullptr;

        action = menu_file->addAction("Load Mesh...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                load_mesh(threads, "", true, action_name(*action));
                        }));

        action = menu_file->addAction("Load Volume...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                load_volume(threads, "", action_name(*action));
                        }));

        action = menu_file->addAction("Save...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                save_mesh(threads, model_tree, action_name(*action));
                        }));

        action = menu_file->addAction("Save Image...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                save_view_image(threads, view, action_name(*action));
                        }));

        action = action_self_test;
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                self_test(threads, process::TestType::ALL, action_name(*action));
                        }));

        action = action_benchmark;
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                self_test(threads, process::TestType::BENCHMARK, action_name(*action));
                        }));

        action = menu_rendering->addAction("Painter...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                painter(threads, model_tree, view, lighting, colors, action_name(*action));
                        }));

        action = menu_edit->addAction("BoundCocone...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                bound_cocone(threads, model_tree, action_name(*action));
                        }));

        action = menu_edit->addAction("3D Slice...");
        connections->emplace_back(
                QObject::connect(
                        action, &QAction::triggered,
                        [=]
                        {
                                volume_3d_slice(threads, model_tree, action_name(*action));
                        }));
}
}

Actions::Actions(
        const com::CommandLineOptions& options,
        QStatusBar* const status_bar,
        QAction* const action_self_test,
        QAction* const action_benchmark,
        QMenu* const menu_file,
        QMenu* const menu_create,
        QMenu* const menu_edit,
        QMenu* const menu_rendering,
        const storage::Repository* const repository,
        view::View* const view,
        ModelTree* const model_tree,
        const LightingWidget* const lighting,
        const ColorsWidget* const colors)
        : worker_threads_(com::create_worker_threads(THREAD_ID_COUNT, SELF_TEST_THREAD_ID, status_bar))
{
        create_menu(
                &connections_, worker_threads_.get(), action_self_test, action_benchmark, menu_file, menu_edit,
                menu_rendering, view, model_tree, lighting, colors);

        create_repository_menu(WORKER_THREAD_ID, &connections_, worker_threads_.get(), menu_create, repository);

#ifndef BUILD_RELEASE
        self_test(worker_threads_.get(), process::TestType::SMALL, "Self-Test");
#endif

        if (!options.file_name.empty())
        {
                load_mesh(worker_threads_.get(), options.file_name, !options.no_object_selection_dialog, "Load Mesh");
        }
}

Actions::~Actions()
{
        worker_threads_->terminate_all();
}

void Actions::set_progresses()
{
        worker_threads_->set_progresses();
}
}
