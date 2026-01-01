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

#include "main_window.h"

#include "actions.h"
#include "colors_widget.h"
#include "graphics_widget.h"
#include "lighting_widget.h"
#include "log.h"
#include "mesh_widget.h"
#include "model_tree.h"
#include "view_widget.h"
#include "volume_widget.h"

#include "ui_main_window.h"

#include <src/com/error.h>
#include <src/com/merge.h>
#include <src/com/message.h>
#include <src/gui/com/application.h>
#include <src/gui/com/command_line.h>
#include <src/gui/com/model_events.h>
#include <src/gui/com/support.h>
#include <src/gui/dialogs/application_about.h>
#include <src/gui/dialogs/application_help.h>
#include <src/gui/dialogs/message.h>
#include <src/settings/name.h>
#include <src/storage/repository.h>
#include <src/view/create.h>
#include <src/view/event.h>
#include <src/view/view.h>
#include <src/vulkan/instance.h>

#include <QAction>
#include <QCloseEvent>
#include <QList>
#include <QMouseEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QSinglePointEvent>
#include <QSize>
#include <QSplitter>
#include <QTimer>
#include <QWheelEvent>
#include <Qt>

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ns::gui::main_window
{
namespace
{
constexpr double WINDOW_SIZE_COEF = 0.7;
constexpr bool WINDOW_SIZE_GRAPHICS = true;

constexpr std::chrono::milliseconds UPDATE_INTERVAL{200};
constexpr std::chrono::milliseconds WINDOW_SHOW_DELAY{50};
}

MainWindow::MainWindow()
{
        ui_.setupUi(this);

        this->setWindowTitle(settings::APPLICATION_NAME);
        ui_.action_about->setText("About " + QString(settings::APPLICATION_NAME));

        log_ = std::make_unique<Log>(ui_.text_log);

        constructor_graphics_widget();
        constructor_splitters();
        constructor_objects();
        constructor_connections();
}

void MainWindow::constructor_graphics_widget()
{
        QSplitter* const splitter = com::find_widget_splitter(this, ui_.graphics_widget);
        ASSERT(splitter && splitter->orientation() == Qt::Horizontal);

        graphics_widget_ = new GraphicsWidget(this);
        if (splitter->replaceWidget(splitter->indexOf(ui_.graphics_widget), graphics_widget_) != ui_.graphics_widget)
        {
                error_fatal("Failed to replace graphics widget");
        }
        delete ui_.graphics_widget;
        ui_.graphics_widget = nullptr;

        graphics_widget_->setMinimumSize(100, 100);
        graphics_widget_->setVisible(true);
}

void MainWindow::constructor_splitters()
{
        QSplitter* const horizontal = com::find_widget_splitter(this, graphics_widget_);
        ASSERT(horizontal && horizontal->orientation() == Qt::Horizontal);
        ASSERT(horizontal->children().size() >= 2);
        ASSERT((horizontal->children()[0] == graphics_widget_ && horizontal->children()[1] == ui_.tab_widget)
               || (horizontal->children()[1] == graphics_widget_ && horizontal->children()[0] == ui_.tab_widget));
        horizontal->setSizes(QList<int>() << 1 << 1'000'000);

        QSplitter* const vertical = com::find_widget_splitter(this, horizontal);
        ASSERT(vertical && vertical->orientation() == Qt::Vertical);
        ASSERT(vertical->children().size() >= 2);
        ASSERT((vertical->children()[0] == ui_.text_log && vertical->children()[1] == horizontal)
               || (vertical->children()[1] == ui_.text_log || vertical->children()[0] == horizontal));
        vertical->setSizes(QList<int>() << 1'000'000 << 1);
}

void MainWindow::constructor_objects()
{
        repository_ = std::make_unique<storage::Repository>();

        model_tree_ = std::make_unique<ModelTree>();
        com::add_widget(ui_.tab_models, model_tree_.get());

        lighting_widget_ = std::make_unique<LightingWidget>();
        com::add_widget(ui_.tab_lighting, lighting_widget_.get());

        colors_widget_ = std::make_unique<ColorsWidget>();
        com::add_widget(ui_.tab_color, colors_widget_.get());

        view_widget_ = std::make_unique<ViewWidget>();
        com::add_widget(ui_.tab_view, view_widget_.get());

        mesh_widget_ = std::make_unique<MeshWidget>();
        com::add_widget(ui_.tab_mesh, mesh_widget_.get());

        volume_widget_ = std::make_unique<VolumeWidget>();
        com::add_widget(ui_.tab_volume, volume_widget_.get());

        //

        ui_.main_widget->layout()->setContentsMargins(0, 0, 0, 0);
        ui_.main_widget->layout()->setSpacing(0);

        // disable height changing when a widget is added or removed
        ui_.status_bar->setFixedHeight(ui_.status_bar->height());
}

void MainWindow::constructor_connections()
{
        connect(graphics_widget_, &GraphicsWidget::mouse_wheel, this, &MainWindow::on_graphics_widget_mouse_wheel);
        connect(graphics_widget_, &GraphicsWidget::mouse_move, this, &MainWindow::on_graphics_widget_mouse_move);
        connect(graphics_widget_, &GraphicsWidget::mouse_press, this, &MainWindow::on_graphics_widget_mouse_press);
        connect(graphics_widget_, &GraphicsWidget::mouse_release, this, &MainWindow::on_graphics_widget_mouse_release);
        connect(graphics_widget_, &GraphicsWidget::widget_resize, this, &MainWindow::on_graphics_widget_resize);

        connect(ui_.action_about, &QAction::triggered, this,
                [this]
                {
                        bool ray_tracing = false;
                        if (view_)
                        {
                                std::optional<view::info::Description> description;
                                view_->receive({&description});
                                if (description)
                                {
                                        ray_tracing = description->ray_tracing;
                                }
                                else
                                {
                                        message_error("Failed to receive view description");
                                }
                        }
                        dialogs::application_about(ray_tracing);
                });

        connect(ui_.action_help, &QAction::triggered, this,
                []
                {
                        dialogs::application_help();
                });

        connect(&timer_, &QTimer::timeout, this, &MainWindow::on_timer);
}

std::vector<view::Command> MainWindow::view_initial_commands() const
{
        return merge<std::vector<view::Command>>(
                colors_widget_->commands(), lighting_widget_->commands(), view_widget_->commands());
}

MainWindow::~MainWindow()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        terminate_all_threads();
}

void MainWindow::closeEvent(QCloseEvent* const event)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const QPointer ptr(this);
        std::optional<bool> yes = dialogs::message_question_default_no("Do you want to close the main window?");
        if (ptr.isNull())
        {
                return;
        }
        if (!yes || !*yes)
        {
                event->ignore();
                return;
        }

        terminate_all_threads();

        event->accept();
}

void MainWindow::terminate_all_threads()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        timer_.stop();

        actions_.reset();
        model_events_.reset();

        lighting_widget_.reset();
        colors_widget_.reset();
        view_widget_.reset();
        mesh_widget_.reset();
        volume_widget_.reset();

        model_tree_.reset();
        view_.reset();
}

void MainWindow::showEvent(QShowEvent* const /*event*/)
{
        if (!first_show_)
        {
                return;
        }
        first_show_ = false;

        ui_.tab_widget->setCurrentWidget(ui_.tab_color);
        ui_.tab_widget->setCurrentWidget(ui_.tab_lighting);
        ui_.tab_widget->setCurrentWidget(ui_.tab_mesh);
        ui_.tab_widget->setCurrentWidget(ui_.tab_view);
        ui_.tab_widget->setCurrentWidget(ui_.tab_volume);
        ui_.tab_widget->setCurrentWidget(ui_.tab_models);

        if (WINDOW_SIZE_GRAPHICS)
        {
                const QScreen& screen = *com::Application::instance()->primaryScreen();
                const QSize size = screen.geometry().size() * WINDOW_SIZE_COEF;
                com::resize_window_widget(this, graphics_widget_, size);
        }
        else
        {
                const QScreen& screen = *com::Application::instance()->primaryScreen();
                const QSize size = screen.availableGeometry().size() * WINDOW_SIZE_COEF;
                com::resize_window_frame(this, size);
        }

        com::move_window_to_desktop_center(this);

        // window is not yet visible on showEvent
        QTimer::singleShot(
                WINDOW_SHOW_DELAY, this,
                [this]
                {
                        try
                        {
                                first_shown();
                        }
                        catch (const std::exception& e)
                        {
                                message_error_fatal(e.what());
                        }
                        catch (...)
                        {
                                message_error_fatal("Error first show");
                        }
                });
}

void MainWindow::first_shown()
{
        const com::CommandLineOptions options = com::command_line_options();

        vulkan_instance_ = std::make_unique<vulkan::Instance>();

        view_ = view::create_view(
                com::widget_window_id(graphics_widget_), com::widget_size(graphics_widget_), view_initial_commands());

        lighting_widget_->set_view(view_.get());
        colors_widget_->set_view(view_.get());
        view_widget_->set_view(view_.get());
        mesh_widget_->set_model_tree(model_tree_.get());
        volume_widget_->set_model_tree(model_tree_.get());
        model_events_ = std::make_unique<com::ModelEvents>(model_tree_->events(), view_.get());
        actions_ = std::make_unique<Actions>(
                options, ui_.status_bar, ui_.action_self_test, ui_.action_benchmark, ui_.menu_file, ui_.menu_create,
                ui_.menu_edit, ui_.menu_rendering, repository_.get(), view_.get(), model_tree_.get(),
                lighting_widget_.get(), colors_widget_.get());

        if (!ui_.menu_file->actions().empty())
        {
                ui_.menu_file->addSeparator();
        }
        connect(ui_.menu_file->addAction("Exit..."), &QAction::triggered, this, &MainWindow::close);

        timer_.start(UPDATE_INTERVAL);
}

void MainWindow::on_timer()
{
        log_->write();
        actions_->set_progresses();
}

std::tuple<double, double> MainWindow::graphics_widget_position(const QSinglePointEvent* const event) const
{
        static_assert(std::is_floating_point_v<decltype(event->position().x())>);
        static_assert(std::is_floating_point_v<decltype(event->position().y())>);

        const double x = event->position().x() / graphics_widget_->width();
        const double y = event->position().y() / graphics_widget_->height();
        return {x, y};
}

void MainWindow::on_graphics_widget_mouse_wheel(const QWheelEvent* const event)
{
        if (view_)
        {
                const auto [x, y] = graphics_widget_position(event);
                view_->send(view::command::MouseWheel(x, y, event->angleDelta().y() / 120.0));
        }
}

void MainWindow::on_graphics_widget_mouse_move(const QMouseEvent* const event)
{
        if (view_)
        {
                const auto [x, y] = graphics_widget_position(event);
                view_->send(view::command::MouseMove(x, y));
        }
}

void MainWindow::on_graphics_widget_mouse_press(const QMouseEvent* const event)
{
        if (view_)
        {
                if (event->button() == Qt::MouseButton::LeftButton)
                {
                        const auto [x, y] = graphics_widget_position(event);
                        view_->send(view::command::MousePress(x, y, view::MouseButton::LEFT));
                        return;
                }
                if (event->button() == Qt::MouseButton::RightButton)
                {
                        const auto [x, y] = graphics_widget_position(event);
                        view_->send(view::command::MousePress(x, y, view::MouseButton::RIGHT));
                        return;
                }
        }
}

void MainWindow::on_graphics_widget_mouse_release(const QMouseEvent* const event)
{
        if (view_)
        {
                if (event->button() == Qt::MouseButton::LeftButton)
                {
                        const auto [x, y] = graphics_widget_position(event);
                        view_->send(view::command::MouseRelease(x, y, view::MouseButton::LEFT));
                        return;
                }
                if (event->button() == Qt::MouseButton::RightButton)
                {
                        const auto [x, y] = graphics_widget_position(event);
                        view_->send(view::command::MouseRelease(x, y, view::MouseButton::RIGHT));
                        return;
                }
        }
}

void MainWindow::on_graphics_widget_resize(const QResizeEvent* const event)
{
        if (view_)
        {
                view_->send(view::command::WindowResize(event->size().width(), event->size().height()));
        }
}
}
