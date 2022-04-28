/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../com/application.h"
#include "../com/command_line.h"
#include "../com/support.h"
#include "../dialogs/application_about.h"
#include "../dialogs/application_help.h"
#include "../dialogs/message.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/message.h>
#include <src/com/type/limit.h>
#include <src/settings/name.h>
#include <src/view/create.h>

#include <QCloseEvent>
#include <QPointer>
#include <QScreen>

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

        log_ = std::make_unique<Log>(ui_.text_log);
        constructor_graphics_widget();
        constructor_objects();
}

void MainWindow::constructor_graphics_widget()
{
        QSplitter* const splitter = find_widget_splitter(this, ui_.graphics_widget);
        ASSERT(splitter && splitter->orientation() == Qt::Horizontal);

        graphics_widget_ = new GraphicsWidget(this);

        QWidget* w = splitter->replaceWidget(splitter->indexOf(ui_.graphics_widget), graphics_widget_);
        if (w != ui_.graphics_widget)
        {
                error_fatal("Failed to replace graphics widget");
        }
        delete ui_.graphics_widget;
        ui_.graphics_widget = nullptr;

        graphics_widget_->setMinimumSize(400, 400);
        graphics_widget_->setVisible(true);

        set_horizontal_stretch(graphics_widget_, 1);
        for (QObject* object : splitter->children())
        {
                if (object != graphics_widget_ && qobject_cast<QWidget*>(object))
                {
                        set_horizontal_stretch(qobject_cast<QWidget*>(object), 0);
                }
        }

        set_vertical_stretch(splitter, 1);
        QSplitter* const vertical_splitter = find_widget_splitter(this, splitter);
        ASSERT(vertical_splitter && vertical_splitter->orientation() == Qt::Vertical);
        for (QObject* object : vertical_splitter->children())
        {
                if (object != splitter && qobject_cast<QWidget*>(object))
                {
                        set_vertical_stretch(qobject_cast<QWidget*>(object), 0);
                }
        }

        connect(graphics_widget_, &GraphicsWidget::mouse_wheel, this, &MainWindow::on_graphics_widget_mouse_wheel);
        connect(graphics_widget_, &GraphicsWidget::mouse_move, this, &MainWindow::on_graphics_widget_mouse_move);
        connect(graphics_widget_, &GraphicsWidget::mouse_press, this, &MainWindow::on_graphics_widget_mouse_press);
        connect(graphics_widget_, &GraphicsWidget::mouse_release, this, &MainWindow::on_graphics_widget_mouse_release);
        connect(graphics_widget_, &GraphicsWidget::widget_resize, this, &MainWindow::on_graphics_widget_resize);
}

void MainWindow::constructor_objects()
{
        repository_ = std::make_unique<storage::Repository>();

        model_tree_ = std::make_unique<ModelTree>();
        add_widget(ui_.tab_models, model_tree_.get());

        lighting_widget_ = std::make_unique<LightingWidget>();
        add_widget(ui_.tab_lighting, lighting_widget_.get());

        colors_widget_ = std::make_unique<ColorsWidget>();
        add_widget(ui_.tab_color, colors_widget_.get());

        view_widget_ = std::make_unique<ViewWidget>();
        add_widget(ui_.tab_view, view_widget_.get());

        mesh_widget_ = std::make_unique<MeshWidget>();
        add_widget(ui_.tab_mesh, mesh_widget_.get());

        volume_widget_ = std::make_unique<VolumeWidget>();
        add_widget(ui_.tab_volume, volume_widget_.get());

        //

        ui_.main_widget->layout()->setContentsMargins(0, 0, 0, 0);
        ui_.main_widget->layout()->setSpacing(0);

        ui_.tab_widget->setCurrentWidget(ui_.tab_models);

        // disable height changing when a widget is added or removed
        ui_.status_bar->setFixedHeight(ui_.status_bar->height());

        //

        ui_.action_about->setText("About " + QString(settings::APPLICATION_NAME));
        connect(ui_.action_about, &QAction::triggered, this,
                [this]
                {
                        bool ray_tracing = false;
                        if (view_)
                        {
                                view::info::Description description;
                                view_->receive({&description});
                                ray_tracing = description.ray_tracing;
                        }
                        dialog::application_about(ray_tracing);
                });

        connect(ui_.action_help, &QAction::triggered, this,
                []
                {
                        dialog::application_help();
                });

        connect(&timer_, &QTimer::timeout, this, &MainWindow::on_timer);
}

std::vector<view::Command> MainWindow::view_initial_commands() const
{
        return {view::command::ClipPlaneSetColor(colors_widget_->clip_plane_color()),
                view::command::ConvexHullShow(view_widget_->convex_hull_2d_checked()),
                view::command::DftSetBackgroundColor(colors_widget_->dft_background_color()),
                view::command::DftSetBrightness(view_widget_->dft_brightness()),
                view::command::DftSetColor(colors_widget_->dft_color()),
                view::command::DftShow(view_widget_->dft_checked()),
                view::command::OpticalFlowShow(view_widget_->optical_flow_checked()),
                view::command::PencilSketchShow(view_widget_->pencil_sketch_checked()),
                view::command::SetLightingColor(lighting_widget_->rgb()),
                view::command::SetNormalColorNegative(colors_widget_->normal_color_negative()),
                view::command::SetNormalColorPositive(colors_widget_->normal_color_positive()),
                view::command::SetNormalLength(view_widget_->normal_length()),
                view::command::SetShadowZoom(view_widget_->shadow_zoom()),
                view::command::SetVerticalSync(view_widget_->vertical_sync_checked()),
                view::command::SetWireframeColor(colors_widget_->wireframe_color()),
                view::command::ShowFog(view_widget_->fog_checked()),
                view::command::ShowFps(view_widget_->fps_checked()),
                view::command::ShowMaterials(view_widget_->materials_checked()),
                view::command::ShowNormals(view_widget_->normals_checked()),
                view::command::ShowShadow(view_widget_->shadow_checked()),
                view::command::ShowSmooth(view_widget_->smooth_checked()),
                view::command::ShowWireframe(view_widget_->wireframe_checked()),
                view::command::SetBackgroundColor(colors_widget_->background_color())};
}

MainWindow::~MainWindow()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        terminate_all_threads();
}

void MainWindow::closeEvent(QCloseEvent* const event)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        QPointer ptr(this);
        std::optional<bool> yes = dialog::message_question_default_no("Do you want to close the main window?");
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

        // window is not yet visible on showEvent
        QTimer::singleShot(
                WINDOW_SHOW_DELAY, this,
                [this]()
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
        if (WINDOW_SIZE_GRAPHICS)
        {
                const QScreen& screen = *Application::instance()->primaryScreen();
                const QSize size = screen.geometry().size() * WINDOW_SIZE_COEF;
                resize_window_widget(this, graphics_widget_, size);
        }
        else
        {
                const QScreen& screen = *Application::instance()->primaryScreen();
                const QSize size = screen.availableGeometry().size() * WINDOW_SIZE_COEF;
                resize_window_frame(this, size);
        }

        move_window_to_desktop_center(this);

        const CommandLineOptions options = command_line_options();

        vulkan_instance_ = std::make_unique<vulkan::Instance>();

        view_ = view::create_view(
                widget_window_id(graphics_widget_),
                std::array{
                        static_cast<double>(graphics_widget_->widthMM()),
                        static_cast<double>(graphics_widget_->heightMM())},
                view_initial_commands());

        lighting_widget_->set_view(view_.get());
        colors_widget_->set_view(view_.get());
        view_widget_->set_view(view_.get());
        mesh_widget_->set_model_tree(model_tree_.get());
        volume_widget_->set_model_tree(model_tree_.get());
        model_events_ = std::make_unique<ModelEvents>(model_tree_->events(), view_.get());
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

void MainWindow::on_graphics_widget_mouse_wheel(QWheelEvent* const e)
{
        if (view_)
        {
                view_->send(
                        view::command::MouseWheel(e->position().x(), e->position().y(), e->angleDelta().y() / 120.0));
        }
}

void MainWindow::on_graphics_widget_mouse_move(QMouseEvent* const e)
{
        if (view_)
        {
                view_->send(view::command::MouseMove(e->x(), e->y()));
        }
}

void MainWindow::on_graphics_widget_mouse_press(QMouseEvent* const e)
{
        if (view_)
        {
                if (e->button() == Qt::MouseButton::LeftButton)
                {
                        view_->send(view::command::MousePress(e->x(), e->y(), view::MouseButton::LEFT));
                        return;
                }
                if (e->button() == Qt::MouseButton::RightButton)
                {
                        view_->send(view::command::MousePress(e->x(), e->y(), view::MouseButton::RIGHT));
                        return;
                }
        }
}

void MainWindow::on_graphics_widget_mouse_release(QMouseEvent* const e)
{
        if (view_)
        {
                if (e->button() == Qt::MouseButton::LeftButton)
                {
                        view_->send(view::command::MouseRelease(e->x(), e->y(), view::MouseButton::LEFT));
                        return;
                }
                if (e->button() == Qt::MouseButton::RightButton)
                {
                        view_->send(view::command::MouseRelease(e->x(), e->y(), view::MouseButton::RIGHT));
                        return;
                }
        }
}

void MainWindow::on_graphics_widget_resize(QResizeEvent* const e)
{
        if (view_)
        {
                view_->send(view::command::WindowResize(e->size().width(), e->size().height()));
        }
}
}
