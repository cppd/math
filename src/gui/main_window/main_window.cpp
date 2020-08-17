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
#include "../com/thread_ui.h"
#include "../dialogs/application_about.h"
#include "../dialogs/application_help.h"
#include "../dialogs/color_dialog.h"
#include "../dialogs/message.h"

#include <src/com/error.h>
#include <src/com/exception.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/message.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/process/computing.h>
#include <src/process/exporting.h>
#include <src/process/loading.h>
#include <src/process/painting.h>
#include <src/process/testing.h>
#include <src/settings/name.h>
#include <src/utility/file/sys.h>
#include <src/view/create.h>

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPointer>
#include <QSignalBlocker>

namespace gui
{
namespace
{
// Размер окна по сравнению с экраном.
constexpr double WINDOW_SIZE_COEF = 0.7;
// Если true, то размер для графики, если false, то размер всего окна.
constexpr bool WINDOW_SIZE_GRAPHICS = true;

constexpr double DFT_MAX_BRIGHTNESS = 50000;
constexpr double DFT_GAMMA = 0.5;

// Таймер отображения хода расчётов. Величина в миллисекундах.
constexpr int TIMER_PROGRESS_BAR_INTERVAL = 100;

// Цвета по умолчанию
constexpr QRgb BACKGROUND_COLOR = qRgb(50, 100, 150);
constexpr QRgb SPECULAR_COLOR = qRgb(255, 255, 255);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);
constexpr QRgb CLIP_PLANE_COLOR = qRgb(250, 230, 150);
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна.
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

// увеличение текстуры тени по сравнению с размером окна.
constexpr int SHADOW_ZOOM = 2;

// Максимальное увеличение для освещений ambient, diffuse, specular.
constexpr double MAXIMUM_COLOR_AMPLIFICATION = 3;

// Максимальный коэффициент для умножения и деления α на него.
constexpr double VOLUME_ALPHA_COEFFICIENT = 20;

constexpr float NORMAL_LENGTH_MINIMUM = 0.001;
constexpr float NORMAL_LENGTH_DEFAULT = 0.05;
constexpr float NORMAL_LENGTH_MAXIMUM = 0.2;
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(200, 200, 0);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(50, 150, 50);
}

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), m_thread_id(std::this_thread::get_id()), m_first_show(true)
{
        ui.setupUi(this);

        this->setWindowTitle(settings::APPLICATION_NAME);

        constructor_graphics_widget();
        constructor_objects();
        constructor_interface();
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

        connect(m_graphics_widget, &GraphicsWidget::mouse_wheel, this, &MainWindow::graphics_widget_mouse_wheel);
        connect(m_graphics_widget, &GraphicsWidget::mouse_move, this, &MainWindow::graphics_widget_mouse_move);
        connect(m_graphics_widget, &GraphicsWidget::mouse_press, this, &MainWindow::graphics_widget_mouse_press);
        connect(m_graphics_widget, &GraphicsWidget::mouse_release, this, &MainWindow::graphics_widget_mouse_release);
        connect(m_graphics_widget, &GraphicsWidget::resize, this, &MainWindow::graphics_widget_resize);
}

void MainWindow::constructor_objects()
{
        m_worker_threads = create_worker_threads();

        m_mesh_and_volume_events = std::make_unique<ModelEvents>(
                &m_model_tree, &m_view, [this](ObjectId id) { update_mesh_ui(id); },
                [this](ObjectId id) { update_volume_ui(id); });

        m_repository = std::make_unique<storage::Repository>();

        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);

        m_repository_actions = std::make_unique<RepositoryActions>(ui.menuCreate, *m_repository);

        connect(m_repository_actions.get(), &RepositoryActions::mesh, this, &MainWindow::action_mesh_repository);
        connect(m_repository_actions.get(), &RepositoryActions::volume, this, &MainWindow::action_volume_repository);

        m_model_tree = std::make_unique<ModelTree>(ui.model_tree, [this]() { model_tree_item_changed(); });

        m_slider_volume_levels = std::make_unique<RangeSlider>(
                ui.slider_volume_level_min, ui.slider_volume_level_max,
                [this](double min, double max) { slider_volume_levels_range_changed(min, max); });
}

void MainWindow::constructor_interface()
{
        connect(&m_timer_progress_bar, &QTimer::timeout, this, &MainWindow::timer_progress_bar);

        QMainWindow::addAction(ui.actionFullScreen);

        {
                QSignalBlocker blocker_check_box(ui.checkBox_clip_plane);
                QSignalBlocker blocker_slider(ui.slider_clip_plane);
                ui.checkBox_clip_plane->setChecked(false);
                ui.slider_clip_plane->setEnabled(false);
                set_slider_position(ui.slider_clip_plane, 0.5);
                // Должно быть точное среднее положение
                ASSERT(((ui.slider_clip_plane->maximum() - ui.slider_clip_plane->minimum()) & 1) == 0);
        }

        {
                QSignalBlocker blocker_check_box(ui.checkBox_normals);
                QSignalBlocker blocker_slider(ui.slider_normals);
                ui.checkBox_normals->setChecked(false);
                ui.slider_normals->setEnabled(false);
                constexpr float v = NORMAL_LENGTH_DEFAULT - NORMAL_LENGTH_MINIMUM;
                constexpr float d = NORMAL_LENGTH_MAXIMUM - NORMAL_LENGTH_MINIMUM;
                constexpr float p = v / d;
                static_assert(v >= 0 && v <= d && d > 0);
                set_slider_position(ui.slider_normals, p);
        }

        disable_mesh_parameters();
        disable_volume_parameters();

        set_widgets_enabled(QMainWindow::layout(), true);
        set_dependent_interface();

        set_background_color(BACKGROUND_COLOR);
        set_specular_color(SPECULAR_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);
        set_clip_plane_color(CLIP_PLANE_COLOR);
        set_normal_color_positive(NORMAL_COLOR_POSITIVE);
        set_normal_color_negative(NORMAL_COLOR_NEGATIVE);

        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

        ui.mainWidget->layout()->setContentsMargins(3, 3, 3, 3);
        ui.mainWidget->layout()->setSpacing(3);

        ui.tabWidget->setCurrentIndex(0);

        ui.actionHelp->setText(QString(settings::APPLICATION_NAME) + " Help");
        ui.actionAbout->setText("About " + QString(settings::APPLICATION_NAME));

        ui.slider_shadow_quality->setSliderPosition(SHADOW_ZOOM);

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.statusBar->setFixedHeight(ui.statusBar->height());

        // Должно быть точное среднее положение
        ASSERT(((ui.slider_ambient->maximum() - ui.slider_ambient->minimum()) & 1) == 0);
        ASSERT(((ui.slider_diffuse->maximum() - ui.slider_diffuse->minimum()) & 1) == 0);
        ASSERT(((ui.slider_specular->maximum() - ui.slider_specular->minimum()) & 1) == 0);
}

void MainWindow::disable_mesh_parameters()
{
        ui.tabMesh->setEnabled(false);

        set_widget_color(ui.widget_mesh_color, QColor(255, 255, 255));

        QSignalBlocker blocker(ui.slider_mesh_transparency);
        set_slider_position(ui.slider_mesh_transparency, 0);
}

void MainWindow::disable_volume_parameters()
{
        ui.tabVolume->setEnabled(false);

        m_slider_volume_levels->set_range(0, 1);

        set_widget_color(ui.widget_volume_color, QColor(255, 255, 255));

        {
                QSignalBlocker blocker_1(ui.slider_volume_transparency);
                QSignalBlocker blocker_2(ui.slider_isosurface_transparency);
                QSignalBlocker blocker_3(ui.checkBox_isosurface);
                QSignalBlocker blocker_4(ui.slider_isovalue);

                set_slider_to_middle(ui.slider_volume_transparency);
                set_slider_position(ui.slider_isosurface_transparency, 0);
                ui.checkBox_isosurface->setChecked(false);
                ui.slider_isovalue->setEnabled(false);
                set_slider_to_middle(ui.slider_isovalue);
                // Должно быть точное среднее положение
                ASSERT(((ui.slider_isovalue->maximum() - ui.slider_isovalue->minimum()) & 1) == 0);
        }
}

MainWindow::~MainWindow()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        terminate_all_threads();
}

void MainWindow::append_to_log(const std::string& text, const Srgb8& color)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        append_to_text_edit(ui.text_log, text, color);
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

        m_model_tree.reset();
        m_view.reset();

        m_mesh_and_volume_events.reset();
}

bool MainWindow::stop_action(WorkerThreads::Action action)
{
        if (m_worker_threads->is_working(action))
        {
                bool yes;
                if (!dialog::message_question_default_no("There is work in progress.\nDo you want to continue?", &yes)
                    || !yes)
                {
                        return false;
                }
        }

        m_worker_threads->terminate_quietly(action);

        return true;
}

void MainWindow::progress_bars(
        WorkerThreads::Action action,
        bool permanent,
        const ProgressRatioList* progress_list,
        std::list<QProgressBar>* progress_bars)
{
        static_assert(limits<unsigned>::max() >= limits<int>::max());

        constexpr unsigned MAX_INT = limits<int>::max();

        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios = progress_list->ratios();

        while (ratios.size() > progress_bars->size())
        {
                QProgressBar& bar = progress_bars->emplace_back();

                bar.setContextMenuPolicy(Qt::CustomContextMenu);

                connect(&bar, &QProgressBar::customContextMenuRequested,
                        [action, &bar, ptr_this = QPointer(this)](const QPoint&) {
                                QtObjectInDynamicMemory<QMenu> menu(&bar);
                                menu->addAction("Terminate");

                                if (!menu->exec(QCursor::pos()) || menu.isNull() || ptr_this.isNull())
                                {
                                        return;
                                }

                                ptr_this->m_worker_threads->terminate_with_message(action);
                        });
        }

        auto bar = progress_bars->begin();

        for (unsigned i = 0; i < ratios.size(); ++i, ++bar)
        {
                if (!bar->isVisible())
                {
                        if (permanent)
                        {
                                ui.statusBar->insertPermanentWidget(0, &(*bar));
                        }
                        else
                        {
                                ui.statusBar->addWidget(&(*bar));
                        }
                        bar->show();
                }

                bar->setFormat(std::get<2>(ratios[i]).c_str());

                unsigned v = std::get<0>(ratios[i]);
                unsigned m = std::get<1>(ratios[i]);

                if (m > 0)
                {
                        m = std::min(m, MAX_INT);
                        v = std::min(v, m);

                        bar->setMaximum(m);
                        bar->setValue(v);
                }
                else
                {
                        bar->setMaximum(0);
                        bar->setValue(0);
                }
        }

        while (bar != progress_bars->end())
        {
                ui.statusBar->removeWidget(&(*bar));
                bar = progress_bars->erase(bar);
        }
}

void MainWindow::timer_progress_bar()
{
        for (const WorkerThreads::Progress& t : m_worker_threads->progresses())
        {
                progress_bars(t.action, t.permanent, t.progress_list, t.progress_bars);
        }
}

void MainWindow::set_background_color(const QColor& c)
{
        m_background_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetBackgroundColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_background_color, c);
}

void MainWindow::set_specular_color(const QColor& c)
{
        m_specular_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetSpecularColor(qcolor_to_rgb(c)));
        }
}

void MainWindow::set_wireframe_color(const QColor& c)
{
        m_wireframe_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetWireframeColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_wireframe_color, c);
}

void MainWindow::set_clip_plane_color(const QColor& c)
{
        m_clip_plane_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetClipPlaneColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_clip_plane_color, c);
}

void MainWindow::set_normal_color_positive(const QColor& c)
{
        m_normal_color_positive = c;
        if (m_view)
        {
                m_view->send(view::command::SetNormalColorPositive(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_normal_color_positive, c);
}

void MainWindow::set_normal_color_negative(const QColor& c)
{
        m_normal_color_negative = c;
        if (m_view)
        {
                m_view->send(view::command::SetNormalColorNegative(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_normal_color_negative, c);
}

void MainWindow::set_dft_background_color(const QColor& c)
{
        m_dft_background_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetDftBackgroundColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_dft_background_color, c);
}

void MainWindow::set_dft_color(const QColor& c)
{
        m_dft_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetDftColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_dft_color, c);
}

void MainWindow::set_dependent_interface()
{
        {
                bool enabled_and_checked = ui.checkBox_shadow->isEnabled() && ui.checkBox_shadow->isChecked();

                ui.label_shadow_quality->setEnabled(enabled_and_checked);
                ui.slider_shadow_quality->setEnabled(enabled_and_checked);
        }

        {
                bool enabled_and_checked = ui.checkBox_dft->isEnabled() && ui.checkBox_dft->isChecked();

                ui.label_dft_brightness->setEnabled(enabled_and_checked);
                ui.slider_dft_brightness->setEnabled(enabled_and_checked);
        }
}

void MainWindow::showEvent(QShowEvent* /*event*/)
{
        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        // Окно ещё не видно, поэтому небольшая задержка, чтобы окно реально появилось.
        QTimer::singleShot(WINDOW_SHOW_DELAY_MSEC, this, &MainWindow::first_shown);
}

void MainWindow::first_shown()
{
        m_timer_progress_bar.start(TIMER_PROGRESS_BAR_INTERVAL);

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

        try
        {
                const CommandLineOptions options = command_line_options();

                //

                std::vector<view::Command> view_initial_commands{
                        view::command::SetBackgroundColor(qcolor_to_rgb(m_background_color)),
                        view::command::SetSpecularColor(qcolor_to_rgb(m_specular_color)),
                        view::command::SetWireframeColor(qcolor_to_rgb(m_wireframe_color)),
                        view::command::SetClipPlaneColor(qcolor_to_rgb(m_clip_plane_color)),
                        view::command::SetNormalLength(normal_length()),
                        view::command::SetNormalColorPositive(qcolor_to_rgb(m_normal_color_positive)),
                        view::command::SetNormalColorNegative(qcolor_to_rgb(m_normal_color_negative)),
                        view::command::ShowSmooth(ui.checkBox_smooth->isChecked()),
                        view::command::ShowWireframe(ui.checkBox_wireframe->isChecked()),
                        view::command::ShowShadow(ui.checkBox_shadow->isChecked()),
                        view::command::ShowFog(ui.checkBox_fog->isChecked()),
                        view::command::ShowMaterials(ui.checkBox_materials->isChecked()),
                        view::command::ShowFps(ui.checkBox_fps->isChecked()),
                        view::command::ShowPencilSketch(ui.checkBox_pencil_sketch->isChecked()),
                        view::command::ShowDft(ui.checkBox_dft->isChecked()),
                        view::command::ShowConvexHull2D(ui.checkBox_convex_hull_2d->isChecked()),
                        view::command::ShowOpticalFlow(ui.checkBox_optical_flow->isChecked()),
                        view::command::ShowNormals(ui.checkBox_normals->isChecked()),
                        view::command::SetAmbient(ambient_light()),
                        view::command::SetDiffuse(diffuse_light()),
                        view::command::SetSpecular(specular_light()),
                        view::command::SetDftBrightness(dft_brightness()),
                        view::command::SetDftBackgroundColor(qcolor_to_rgb(m_dft_background_color)),
                        view::command::SetDftColor(qcolor_to_rgb(m_dft_color)),
                        view::command::SetDefaultNs(default_ns()),
                        view::command::SetVerticalSync(ui.checkBox_vertical_sync->isChecked()),
                        view::command::SetShadowZoom(shadow_zoom())};

                m_view = view::create_view(
                        widget_window_id(m_graphics_widget), widget_pixels_per_inch(m_graphics_widget),
                        std::move(view_initial_commands));

                //

                self_test(test::SelfTestType::Essential, false);

                if (!options.file_name.empty())
                {
                        load_from_file(options.file_name, !options.no_object_selection_dialog);
                }
        }
        catch (const std::exception& e)
        {
                MESSAGE_ERROR_FATAL(e.what());
        }
        catch (...)
        {
                MESSAGE_ERROR_FATAL("Error first show");
        }
}

void MainWindow::load_from_file(std::string file_name, bool use_object_selection_dialog)
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "Loading from file";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }
                WorkerThreads::Function f = process::action_load_from_file(file_name, use_object_selection_dialog);
                //m_model_tree->clear();
                //m_view->send(view::command::ResetView());
                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_actionLoad_triggered()
{
        load_from_file("", true);
}

void MainWindow::action_mesh_repository(int dimension, std::string object_name)
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "Load from mesh repository";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }
                WorkerThreads::Function f =
                        process::action_load_from_mesh_repository(m_repository.get(), dimension, object_name);
                //m_model_tree->clear();
                //m_view->send(view::command::ResetView());
                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::action_volume_repository(int dimension, std::string object_name)
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "Load from volume repository";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }
                WorkerThreads::Function f =
                        process::action_load_from_volume_repository(m_repository.get(), dimension, object_name);
                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_actionExport_triggered()
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "Export";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }
                std::optional<storage::MeshObjectConst> object = m_model_tree->current_mesh_const();
                if (!object)
                {
                        MESSAGE_WARNING("No object to export");
                        return;
                }
                WorkerThreads::Function f = process::action_export(*object);
                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_actionBoundCocone_triggered()
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "BoundCocone";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }
                std::optional<storage::MeshObjectConst> object = m_model_tree->current_mesh_const();
                if (!object)
                {
                        MESSAGE_WARNING("No object to compute BoundCocone");
                        return;
                }
                WorkerThreads::Function f = process::action_bound_cocone(*object);
                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_actionExit_triggered()
{
        close();
}

void MainWindow::on_actionHelp_triggered()
{
        dialog::application_help();
}

void MainWindow::self_test(test::SelfTestType test_type, bool with_confirmation)
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::SelfTest;
        static constexpr const char* DESCRIPTION = "Self-Test";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }
                WorkerThreads::Function f = process::action_self_test(test_type, with_confirmation);
                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_actionSelfTest_triggered()
{
        self_test(test::SelfTestType::Extended, true);
}

void MainWindow::on_actionPainter_triggered()
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "Painter";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }

                std::optional<storage::MeshObjectConst> object = m_model_tree->current_mesh_const();
                if (!object)
                {
                        MESSAGE_WARNING("No object to paint");
                        return;
                }

                view::info::Camera camera;
                m_view->receive({&camera});

                WorkerThreads::Function f = process::action_painter(
                        *object, camera, QMainWindow::windowTitle().toStdString(), qcolor_to_rgb(m_background_color),
                        diffuse_light());

                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_actionAbout_triggered()
{
        dialog::application_about();
}

void MainWindow::on_pushButton_reset_view_clicked()
{
        m_view->send(view::command::ResetView());
}

void MainWindow::graphics_widget_mouse_wheel(QWheelEvent* e)
{
        if (m_view)
        {
                m_view->send(view::command::MouseWheel(e->x(), e->y(), e->angleDelta().ry() / 120.0));
        }
}

void MainWindow::graphics_widget_mouse_move(QMouseEvent* e)
{
        if (m_view)
        {
                m_view->send(view::command::MouseMove(e->x(), e->y()));
        }
}

void MainWindow::graphics_widget_mouse_press(QMouseEvent* e)
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

void MainWindow::graphics_widget_mouse_release(QMouseEvent* e)
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

void MainWindow::graphics_widget_resize(QResizeEvent* e)
{
        if (m_view)
        {
                m_view->send(view::command::WindowResize(e->size().width(), e->size().height()));
        }
}

void MainWindow::model_tree_item_changed()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = m_model_tree->current_item();
        if (!id)
        {
                disable_mesh_parameters();
                disable_volume_parameters();
                return;
        }
        update_mesh_ui(*id);
        update_volume_ui(*id);
}

double MainWindow::lighting_slider_value(const QSlider* slider)
{
        double value = slider->value() - slider->minimum();
        double delta = slider->maximum() - slider->minimum();
        double ratio = 2.0 * value / delta;
        return (ratio <= 1) ? ratio : interpolation(1.0, MAXIMUM_COLOR_AMPLIFICATION, ratio - 1);
}

double MainWindow::ambient_light() const
{
        return lighting_slider_value(ui.slider_ambient);
}
double MainWindow::diffuse_light() const
{
        return lighting_slider_value(ui.slider_diffuse);
}

double MainWindow::specular_light() const
{
        return lighting_slider_value(ui.slider_specular);
}

double MainWindow::default_ns() const
{
        return ui.slider_default_ns->value();
}

void MainWindow::on_pushButton_reset_lighting_clicked()
{
        bool yes;
        if (!dialog::message_question_default_yes("Reset lighting?", &yes) || !yes)
        {
                return;
        }

        set_slider_to_middle(ui.slider_ambient);
        set_slider_to_middle(ui.slider_diffuse);
        set_slider_to_middle(ui.slider_specular);
        set_slider_to_middle(ui.slider_default_ns);
}

double MainWindow::dft_brightness() const
{
        double value = ui.slider_dft_brightness->value() - ui.slider_dft_brightness->minimum();
        double delta = ui.slider_dft_brightness->maximum() - ui.slider_dft_brightness->minimum();
        double value_gamma = std::pow(value / delta, DFT_GAMMA);
        return std::pow(DFT_MAX_BRIGHTNESS, value_gamma);
}

double MainWindow::shadow_zoom() const
{
        return ui.slider_shadow_quality->value();
}

double MainWindow::normal_length() const
{
        return interpolation(NORMAL_LENGTH_MINIMUM, NORMAL_LENGTH_MAXIMUM, slider_position(ui.slider_normals));
}

void MainWindow::on_slider_ambient_valueChanged(int)
{
        m_view->send(view::command::SetAmbient(ambient_light()));
}

void MainWindow::on_slider_diffuse_valueChanged(int)
{
        m_view->send(view::command::SetDiffuse(diffuse_light()));
}

void MainWindow::on_slider_specular_valueChanged(int)
{
        m_view->send(view::command::SetSpecular(specular_light()));
}

void MainWindow::on_slider_dft_brightness_valueChanged(int)
{
        m_view->send(view::command::SetDftBrightness(dft_brightness()));
}

void MainWindow::on_slider_default_ns_valueChanged(int)
{
        m_view->send(view::command::SetDefaultNs(default_ns()));
}

void MainWindow::on_slider_shadow_quality_valueChanged(int)
{
        if (m_view)
        {
                m_view->send(view::command::SetShadowZoom(shadow_zoom()));
        }
}

void MainWindow::on_slider_clip_plane_valueChanged(int)
{
        m_view->send(view::command::ClipPlanePosition(slider_position(ui.slider_clip_plane)));
}

void MainWindow::on_slider_normals_valueChanged(int)
{
        m_view->send(view::command::SetNormalLength(normal_length()));
}

void MainWindow::on_toolButton_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Background Color", m_background_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        ptr->set_background_color(c);
                }
        });
}

void MainWindow::on_toolButton_wireframe_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Wireframe Color", m_wireframe_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_wireframe_color(c);
                }
        });
}

void MainWindow::on_toolButton_clip_plane_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Clip Plane Color", m_clip_plane_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_clip_plane_color(c);
                }
        });
}

void MainWindow::on_toolButton_normal_color_positive_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Positive Normal Color", m_normal_color_positive, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_normal_color_positive(c);
                }
        });
}

void MainWindow::on_toolButton_normal_color_negative_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Negative Normal Color", m_normal_color_negative, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_normal_color_negative(c);
                }
        });
}

void MainWindow::on_toolButton_dft_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("DFT Background Color", m_dft_background_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_dft_background_color(c);
                }
        });
}

void MainWindow::on_toolButton_dft_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("DFT Color", m_dft_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_dft_color(c);
                }
        });
}

void MainWindow::on_checkBox_shadow_clicked()
{
        bool checked = ui.checkBox_shadow->isChecked();

        ui.label_shadow_quality->setEnabled(checked);
        ui.slider_shadow_quality->setEnabled(checked);

        m_view->send(view::command::ShowShadow(checked));
}

void MainWindow::on_checkBox_fog_clicked()
{
        m_view->send(view::command::ShowFog(ui.checkBox_fog->isChecked()));
}

void MainWindow::on_checkBox_wireframe_clicked()
{
        m_view->send(view::command::ShowWireframe(ui.checkBox_wireframe->isChecked()));
}

void MainWindow::on_checkBox_materials_clicked()
{
        m_view->send(view::command::ShowMaterials(ui.checkBox_materials->isChecked()));
}

void MainWindow::on_checkBox_smooth_clicked()
{
        m_view->send(view::command::ShowSmooth(ui.checkBox_smooth->isChecked()));
}

void MainWindow::on_checkBox_fps_clicked()
{
        m_view->send(view::command::ShowFps(ui.checkBox_fps->isChecked()));
}

void MainWindow::on_checkBox_pencil_sketch_clicked()
{
        m_view->send(view::command::ShowPencilSketch(ui.checkBox_pencil_sketch->isChecked()));
}

void MainWindow::on_checkBox_dft_clicked()
{
        bool checked = ui.checkBox_dft->isChecked();

        ui.label_dft_brightness->setEnabled(checked);
        ui.slider_dft_brightness->setEnabled(checked);

        m_view->send(view::command::ShowDft(checked));
}

void MainWindow::on_checkBox_clip_plane_clicked()
{
        constexpr double default_position = 0.5;

        bool checked = ui.checkBox_clip_plane->isChecked();

        ui.slider_clip_plane->setEnabled(checked);
        {
                QSignalBlocker blocker(ui.slider_clip_plane);
                set_slider_position(ui.slider_clip_plane, default_position);
        }
        if (checked)
        {
                m_view->send(view::command::ClipPlaneShow(slider_position(ui.slider_clip_plane)));
        }
        else
        {
                m_view->send(view::command::ClipPlaneHide());
        }
}

void MainWindow::on_checkBox_normals_clicked()
{
        bool checked = ui.checkBox_normals->isChecked();
        ui.slider_normals->setEnabled(checked);
        m_view->send(view::command::ShowNormals(checked));
}

void MainWindow::on_checkBox_convex_hull_2d_clicked()
{
        m_view->send(view::command::ShowConvexHull2D(ui.checkBox_convex_hull_2d->isChecked()));
}

void MainWindow::on_checkBox_optical_flow_clicked()
{
        m_view->send(view::command::ShowOpticalFlow(ui.checkBox_optical_flow->isChecked()));
}

void MainWindow::on_checkBox_vertical_sync_clicked()
{
        m_view->send(view::command::SetVerticalSync(ui.checkBox_vertical_sync->isChecked()));
}

void MainWindow::on_actionFullScreen_triggered()
{
}

void MainWindow::update_mesh_ui(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObjectConst> object_opt = m_model_tree->mesh_const_if_current(id);
        if (!object_opt)
        {
                disable_mesh_parameters();
                return;
        }

        ui.tabMesh->setEnabled(true);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& object) {
                        double alpha;
                        Color color;
                        {
                                mesh::Reading reading(*object);
                                alpha = object->alpha();
                                color = object->color();
                        }
                        double transparency_position = 1.0 - alpha;
                        {
                                QSignalBlocker blocker(ui.slider_mesh_transparency);

                                set_slider_position(ui.slider_mesh_transparency, transparency_position);
                        }
                        set_widget_color(ui.widget_mesh_color, color);
                },
                *object_opt);
}

void MainWindow::update_volume_ui(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObjectConst> volume_object_opt = m_model_tree->volume_const_if_current(id);
        if (!volume_object_opt)
        {
                disable_volume_parameters();
                return;
        }

        ui.tabVolume->setEnabled(true);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<const volume::VolumeObject<N>>& volume_object) {
                        double min;
                        double max;
                        double volume_alpha_coefficient;
                        double isosurface_alpha;
                        bool isosurface;
                        float isovalue;
                        Color color;
                        {
                                volume::Reading reading(*volume_object);
                                min = volume_object->level_min();
                                max = volume_object->level_max();
                                volume_alpha_coefficient = volume_object->volume_alpha_coefficient();
                                isosurface_alpha = volume_object->isosurface_alpha();
                                isosurface = volume_object->isosurface();
                                isovalue = volume_object->isovalue();
                                color = volume_object->color();
                        }

                        m_slider_volume_levels->set_range(min, max);

                        set_widget_color(ui.widget_volume_color, color);

                        double isosurface_transparency_position = 1.0 - isosurface_alpha;

                        volume_alpha_coefficient = std::clamp(
                                volume_alpha_coefficient, 1.0 / VOLUME_ALPHA_COEFFICIENT, VOLUME_ALPHA_COEFFICIENT);
                        double log_volume_alpha_coefficient =
                                std::log(volume_alpha_coefficient) / std::log(VOLUME_ALPHA_COEFFICIENT);
                        double volume_transparency_position = 0.5 * (1.0 - log_volume_alpha_coefficient);

                        {
                                QSignalBlocker blocker_1(ui.checkBox_isosurface);
                                QSignalBlocker blocker_2(ui.slider_volume_transparency);
                                QSignalBlocker blocker_3(ui.slider_isosurface_transparency);
                                QSignalBlocker blocker_4(ui.slider_isovalue);

                                ui.checkBox_isosurface->setChecked(isosurface);
                                ui.slider_isovalue->setEnabled(isosurface);
                                ui.slider_isosurface_transparency->setEnabled(isosurface);
                                ui.slider_volume_transparency->setEnabled(!isosurface);
                                set_slider_position(ui.slider_isovalue, isovalue);
                                set_slider_position(
                                        ui.slider_isosurface_transparency, isosurface_transparency_position);
                                set_slider_position(ui.slider_volume_transparency, volume_transparency_position);
                        }
                },
                *volume_object_opt);
}

void MainWindow::slider_volume_levels_range_changed(double min, double max)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::WritingUpdates updates(volume_object.get(), {volume::Update::Parameters});
                        volume_object->set_levels(min, max);
                },
                *volume_object_opt);
}

void MainWindow::on_slider_volume_transparency_valueChanged(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        double log_alpha_coefficient = 1.0 - 2.0 * slider_position(ui.slider_volume_transparency);
        double alpha_coefficient = std::pow(VOLUME_ALPHA_COEFFICIENT, log_alpha_coefficient);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::WritingUpdates updates(volume_object.get(), {volume::Update::Parameters});
                        volume_object->set_volume_alpha_coefficient(alpha_coefficient);
                },
                *volume_object_opt);
}

void MainWindow::on_slider_isosurface_transparency_valueChanged(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        double alpha = 1.0 - slider_position(ui.slider_isosurface_transparency);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::WritingUpdates updates(volume_object.get(), {volume::Update::Parameters});
                        volume_object->set_isosurface_alpha(alpha);
                },
                *volume_object_opt);
}

void MainWindow::on_checkBox_isosurface_clicked()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        bool checked = ui.checkBox_isosurface->isChecked();
        ui.slider_isovalue->setEnabled(checked);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::WritingUpdates updates(volume_object.get(), {volume::Update::Parameters});
                        volume_object->set_isosurface(checked);
                },
                *volume_object_opt);
}

void MainWindow::on_slider_isovalue_valueChanged(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        float isovalue = slider_position(ui.slider_isovalue);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::WritingUpdates updates(volume_object.get(), {volume::Update::Parameters});
                        volume_object->set_isovalue(isovalue);
                },
                *volume_object_opt);
}

void MainWindow::on_slider_mesh_transparency_valueChanged(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double alpha = 1.0 - slider_position(ui.slider_mesh_transparency);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                        mesh::Writing updates(object.get(), {mesh::Update::Alpha});
                        object->set_alpha(alpha);
                },
                *object_opt);
}

void MainWindow::on_toolButton_mesh_color_clicked()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        Color color;
        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                        mesh::Reading reading(*object);
                        color = object->color();
                },
                *object_opt);

        QPointer ptr(this);
        dialog::color_dialog("Mesh Color", rgb_to_qcolor(color), [&](const QColor& c) {
                if (ptr.isNull())
                {
                        return;
                }
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                                set_widget_color(ui.widget_mesh_color, c);
                                mesh::Writing updates(object.get(), {mesh::Update::Parameters});
                                object->set_color(qcolor_to_rgb(c));
                        },
                        *object_opt);
        });
}

void MainWindow::on_toolButton_volume_color_clicked()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        Color color;
        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Reading reading(*object);
                        color = object->color();
                },
                *object_opt);

        QPointer ptr(this);
        dialog::color_dialog("Volume Color", rgb_to_qcolor(color), [&](const QColor& c) {
                if (ptr.isNull())
                {
                        return;
                }
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                                set_widget_color(ui.widget_volume_color, c);
                                volume::WritingUpdates updates(object.get(), {volume::Update::Parameters});
                                object->set_color(qcolor_to_rgb(c));
                        },
                        *object_opt);
        });
}
}
