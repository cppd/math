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

#include "main_window.h"

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
#include <QDesktopWidget>
#include <QPointer>

namespace gui
{
namespace
{
// Размер окна по сравнению с экраном.
constexpr double WINDOW_SIZE_COEF = 0.7;
// Если true, то размер для графики, если false, то размер всего окна.
constexpr bool WINDOW_SIZE_GRAPHICS = true;

// Интервал таймера отображения лога и хода расчётов.
constexpr std::chrono::milliseconds TIMER_INTERVAL{200};

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна.
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

constexpr double MAXIMUM_SPECULAR_POWER = 1000.0;
constexpr double MAXIMUM_MODEL_LIGHTING = 2.0;
}

MainWindow::MainWindow()
{
        ui.setupUi(this);
        this->setWindowTitle(settings::APPLICATION_NAME);

        m_log = std::make_unique<Log>(ui.text_log);
        constructor_graphics_widget();
        constructor_objects();
}

void MainWindow::constructor_graphics_widget()
{
        QSplitter* splitter = find_widget_splitter(this, ui.graphics_widget);
        ASSERT(splitter);

        m_graphics_widget = new GraphicsWidget(this);

        QWidget* w = splitter->replaceWidget(splitter->indexOf(ui.graphics_widget), m_graphics_widget);
        ASSERT(w == ui.graphics_widget);
        delete ui.graphics_widget;
        ui.graphics_widget = nullptr;

        set_horizontal_stretch(m_graphics_widget, 5);
        m_graphics_widget->setMinimumSize(400, 400);
        m_graphics_widget->setVisible(true);

        for (QObject* object : splitter->children())
        {
                if (object != m_graphics_widget && qobject_cast<QWidget*>(object))
                {
                        set_horizontal_stretch(qobject_cast<QWidget*>(object), 1);
                }
        }

        connect(m_graphics_widget, &GraphicsWidget::mouse_wheel, this, &MainWindow::on_graphics_widget_mouse_wheel);
        connect(m_graphics_widget, &GraphicsWidget::mouse_move, this, &MainWindow::on_graphics_widget_mouse_move);
        connect(m_graphics_widget, &GraphicsWidget::mouse_press, this, &MainWindow::on_graphics_widget_mouse_press);
        connect(m_graphics_widget, &GraphicsWidget::mouse_release, this, &MainWindow::on_graphics_widget_mouse_release);
        connect(m_graphics_widget, &GraphicsWidget::widget_resize, this, &MainWindow::on_graphics_widget_resize);
}

void MainWindow::constructor_objects()
{
        // QMenu* menu_create = new QMenu("Create", this);
        // ui.menu_bar->insertMenu(ui.menu_help->menuAction(), menu_create);

        m_worker_threads =
                create_worker_threads(Actions::required_thread_count(), Actions::permanent_thread_id(), ui.status_bar);

        m_repository = std::make_unique<storage::Repository>();

        m_model_tree = std::make_unique<ModelTree>();
        ui.tab_models->setLayout(m_model_tree->layout());

        m_colors_widget = std::make_unique<ColorsWidget>();
        ui.tab_color->setLayout(m_colors_widget->layout());

        m_view_widget = std::make_unique<ViewWidget>();
        ui.tab_view->setLayout(m_view_widget->layout());

        m_mesh_widget = std::make_unique<MeshWidget>(MAXIMUM_SPECULAR_POWER, MAXIMUM_MODEL_LIGHTING);
        ui.tab_mesh->setLayout(m_mesh_widget->layout());

        m_volume_widget = std::make_unique<VolumeWidget>(MAXIMUM_SPECULAR_POWER, MAXIMUM_MODEL_LIGHTING);
        ui.tab_volume->setLayout(m_volume_widget->layout());

        //

        ui.main_widget->layout()->setContentsMargins(3, 3, 3, 3);
        ui.main_widget->layout()->setSpacing(3);

        ui.tab_widget->setCurrentWidget(ui.tab_models);

        ui.action_help->setText(QString(settings::APPLICATION_NAME) + " Help");
        ui.action_about->setText("About " + QString(settings::APPLICATION_NAME));

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.status_bar->setFixedHeight(ui.status_bar->height());

        //

        connect(ui.action_about, &QAction::triggered, this, &MainWindow::on_about_triggered);
        connect(ui.action_exit, &QAction::triggered, this, &MainWindow::on_exit_triggered);
        connect(ui.action_help, &QAction::triggered, this, &MainWindow::on_help_triggered);
        connect(&m_timer, &QTimer::timeout, this, &MainWindow::on_timer);
}

std::vector<view::Command> MainWindow::view_initial_commands() const
{
        return {view::command::SetBackgroundColor(m_colors_widget->background_color()),
                view::command::SetSpecularColor(m_colors_widget->specular_color()),
                view::command::SetWireframeColor(m_colors_widget->wireframe_color()),
                view::command::SetClipPlaneColor(m_colors_widget->clip_plane_color()),
                view::command::SetNormalLength(m_view_widget->normal_length()),
                view::command::SetNormalColorPositive(m_colors_widget->normal_color_positive()),
                view::command::SetNormalColorNegative(m_colors_widget->normal_color_negative()),
                view::command::ShowSmooth(m_view_widget->smooth_checked()),
                view::command::ShowWireframe(m_view_widget->wireframe_checked()),
                view::command::ShowShadow(m_view_widget->shadow_checked()),
                view::command::ShowFog(m_view_widget->fog_checked()),
                view::command::ShowMaterials(m_view_widget->materials_checked()),
                view::command::ShowFps(m_view_widget->fps_checked()),
                view::command::ShowPencilSketch(m_view_widget->pencil_sketch_checked()),
                view::command::ShowDft(m_view_widget->dft_checked()),
                view::command::ShowConvexHull2D(m_view_widget->convex_hull_2d_checked()),
                view::command::ShowOpticalFlow(m_view_widget->optical_flow_checked()),
                view::command::ShowNormals(m_view_widget->normals_checked()),
                view::command::SetLightingIntensity(m_colors_widget->lighting_intensity()),
                view::command::SetDftBrightness(m_view_widget->dft_brightness()),
                view::command::SetDftBackgroundColor(m_colors_widget->dft_background_color()),
                view::command::SetDftColor(m_colors_widget->dft_color()),
                view::command::SetVerticalSync(m_view_widget->vertical_sync_checked()),
                view::command::SetShadowZoom(m_view_widget->shadow_zoom())};
}

MainWindow::~MainWindow()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        terminate_all_threads();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        QPointer ptr(this);
        bool yes;
        if (!dialog::message_question_default_no("Do you want to close the main window?", &yes) || ptr.isNull() || !yes)
        {
                if (!ptr.isNull())
                {
                        event->ignore();
                }
                return;
        }

        terminate_all_threads();

        event->accept();
}

void MainWindow::terminate_all_threads()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_worker_threads->terminate_all();

        m_timer.stop();

        m_actions.reset();
        m_model_events.reset();

        m_colors_widget.reset();
        m_view_widget.reset();
        m_mesh_widget.reset();
        m_volume_widget.reset();

        m_model_tree.reset();
        m_view.reset();
}

void MainWindow::showEvent(QShowEvent* /*event*/)
{
        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        // Окно ещё не видно, поэтому небольшая задержка, чтобы окно реально появилось.
        QTimer::singleShot(WINDOW_SHOW_DELAY_MSEC, this, [this]() {
                try
                {
                        first_shown();
                }
                catch (const std::exception& e)
                {
                        MESSAGE_ERROR_FATAL(e.what());
                }
                catch (...)
                {
                        MESSAGE_ERROR_FATAL("Error first show");
                }
        });
}

void MainWindow::first_shown()
{
        if (WINDOW_SIZE_GRAPHICS)
        {
                QSize size = QDesktopWidget().screenGeometry(this).size() * WINDOW_SIZE_COEF;
                resize_window_widget(this, m_graphics_widget, size);
        }
        else
        {
                QSize size = QDesktopWidget().availableGeometry(this).size() * WINDOW_SIZE_COEF;
                resize_window_frame(this, size);
        }

        move_window_to_desktop_center(this);

        const CommandLineOptions options = command_line_options();

        m_view = view::create_view(
                widget_window_id(m_graphics_widget), widget_pixels_per_inch(m_graphics_widget),
                view_initial_commands());

        m_colors_widget->set_view(m_view.get());
        m_view_widget->set_view(m_view.get());
        m_mesh_widget->set_model_tree(m_model_tree.get());
        m_volume_widget->set_model_tree(m_model_tree.get());
        m_model_events = std::make_unique<application::ModelEvents>(m_model_tree->events(), m_view.get());
        m_actions = std::make_unique<Actions>(
                options, ui.action_load, ui.action_export, ui.action_self_test, ui.menu_create, ui.menu_edit,
                ui.menu_rendering, m_repository.get(), m_worker_threads.get(), m_view.get(), m_model_tree.get(),
                m_colors_widget.get());

        m_timer.start(TIMER_INTERVAL);
}

void MainWindow::on_timer()
{
        m_log->write();
        m_worker_threads->set_progresses();
}

void MainWindow::on_exit_triggered()
{
        close();
}

void MainWindow::on_help_triggered()
{
        dialog::application_help();
}

void MainWindow::on_about_triggered()
{
        dialog::application_about();
}

void MainWindow::on_graphics_widget_mouse_wheel(QWheelEvent* e)
{
        if (m_view)
        {
                m_view->send(
                        view::command::MouseWheel(e->position().x(), e->position().y(), e->angleDelta().y() / 120.0));
        }
}

void MainWindow::on_graphics_widget_mouse_move(QMouseEvent* e)
{
        if (m_view)
        {
                m_view->send(view::command::MouseMove(e->x(), e->y()));
        }
}

void MainWindow::on_graphics_widget_mouse_press(QMouseEvent* e)
{
        if (m_view)
        {
                if (e->button() == Qt::MouseButton::LeftButton)
                {
                        m_view->send(view::command::MousePress(e->x(), e->y(), view::command::MouseButton::Left));
                        return;
                }
                if (e->button() == Qt::MouseButton::RightButton)
                {
                        m_view->send(view::command::MousePress(e->x(), e->y(), view::command::MouseButton::Right));
                        return;
                }
        }
}

void MainWindow::on_graphics_widget_mouse_release(QMouseEvent* e)
{
        if (m_view)
        {
                if (e->button() == Qt::MouseButton::LeftButton)
                {
                        m_view->send(view::command::MouseRelease(e->x(), e->y(), view::command::MouseButton::Left));
                        return;
                }
                if (e->button() == Qt::MouseButton::RightButton)
                {
                        m_view->send(view::command::MouseRelease(e->x(), e->y(), view::command::MouseButton::Right));
                        return;
                }
        }
}

void MainWindow::on_graphics_widget_resize(QResizeEvent* e)
{
        if (m_view)
        {
                m_view->send(view::command::WindowResize(e->size().width(), e->size().height()));
        }
}
}
