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

constexpr double MAXIMUM_LIGHTING_INTENSITY = 3.0;

// Максимальный коэффициент для умножения и деления α на него.
constexpr double VOLUME_ALPHA_COEFFICIENT = 20;

constexpr double MAXIMUM_SPECULAR_POWER = 1000.0;
constexpr double MAXIMUM_MODEL_LIGHTING = 2.0;

constexpr float NORMAL_LENGTH_MINIMUM = 0.001;
constexpr float NORMAL_LENGTH_DEFAULT = 0.05;
constexpr float NORMAL_LENGTH_MAXIMUM = 0.2;
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(200, 200, 0);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(50, 150, 50);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
        ui.setupUi(this);

        this->setWindowTitle(settings::APPLICATION_NAME);

        constructor_graphics_widget();
        constructor_objects();
        constructor_interface();
        constructor_connect();
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
        m_worker_threads = create_worker_threads();

        m_repository = std::make_unique<storage::Repository>();

        m_repository_actions = std::make_unique<RepositoryActions>(
                ui.menuCreate, *m_repository,
                [this](int dimension, const std::string& name) { on_repository_mesh(dimension, name); },
                [this](int dimension, const std::string& name) { on_repository_volume(dimension, name); });

        m_model_tree = std::make_unique<ModelTree>(ui.model_tree, [this]() { on_model_tree_update(); });
        m_model_events.set_tree(m_model_tree.get());

        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);
}

void MainWindow::constructor_interface()
{
        set_widgets_enabled(QMainWindow::layout(), true);

        m_slider_volume_levels = std::make_unique<RangeSlider>(ui.slider_volume_level_min, ui.slider_volume_level_max);

        QMainWindow::addAction(ui.action_full_screen);

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
        {
                // Должно быть точное среднее положение
                ASSERT(((ui.slider_lighting_intensity->maximum() - ui.slider_lighting_intensity->minimum()) & 1) == 0);
                QSignalBlocker blocker(ui.slider_lighting_intensity);
                set_slider_to_middle(ui.slider_lighting_intensity);
        }

        mesh_ui_disable();
        volume_ui_disable();

        on_dft_clicked();
        on_shadow_clicked();

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

        ui.action_help->setText(QString(settings::APPLICATION_NAME) + " Help");
        ui.action_about->setText("About " + QString(settings::APPLICATION_NAME));

        ui.slider_shadow_quality->setSliderPosition(SHADOW_ZOOM);

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.statusBar->setFixedHeight(ui.statusBar->height());
}

void MainWindow::constructor_connect()
{
        connect(ui.action_about, &QAction::triggered, this, &MainWindow::on_about_triggered);
        connect(ui.action_bound_cocone, &QAction::triggered, this, &MainWindow::on_bound_cocone_triggered);
        connect(ui.action_exit, &QAction::triggered, this, &MainWindow::on_exit_triggered);
        connect(ui.action_export, &QAction::triggered, this, &MainWindow::on_export_triggered);
        connect(ui.action_full_screen, &QAction::triggered, this, &MainWindow::on_full_screen_triggered);
        connect(ui.action_help, &QAction::triggered, this, &MainWindow::on_help_triggered);
        connect(ui.action_load, &QAction::triggered, this, &MainWindow::on_load_triggered);
        connect(ui.action_painter, &QAction::triggered, this, &MainWindow::on_painter_triggered);
        connect(ui.action_self_test, &QAction::triggered, this, &MainWindow::on_self_test_triggered);

        connect(ui.checkBox_clip_plane, &QCheckBox::clicked, this, &MainWindow::on_clip_plane_clicked);
        connect(ui.checkBox_convex_hull_2d, &QCheckBox::clicked, this, &MainWindow::on_convex_hull_2d_clicked);
        connect(ui.checkBox_dft, &QCheckBox::clicked, this, &MainWindow::on_dft_clicked);
        connect(ui.checkBox_fog, &QCheckBox::clicked, this, &MainWindow::on_fog_clicked);
        connect(ui.checkBox_fps, &QCheckBox::clicked, this, &MainWindow::on_fps_clicked);
        connect(ui.checkBox_isosurface, &QCheckBox::clicked, this, &MainWindow::on_isosurface_clicked);
        connect(ui.checkBox_materials, &QCheckBox::clicked, this, &MainWindow::on_materials_clicked);
        connect(ui.checkBox_normals, &QCheckBox::clicked, this, &MainWindow::on_normals_clicked);
        connect(ui.checkBox_optical_flow, &QCheckBox::clicked, this, &MainWindow::on_optical_flow_clicked);
        connect(ui.checkBox_pencil_sketch, &QCheckBox::clicked, this, &MainWindow::on_pencil_sketch_clicked);
        connect(ui.checkBox_shadow, &QCheckBox::clicked, this, &MainWindow::on_shadow_clicked);
        connect(ui.checkBox_smooth, &QCheckBox::clicked, this, &MainWindow::on_smooth_clicked);
        connect(ui.checkBox_vertical_sync, &QCheckBox::clicked, this, &MainWindow::on_vertical_sync_clicked);
        connect(ui.checkBox_wireframe, &QCheckBox::clicked, this, &MainWindow::on_wireframe_clicked);

        connect(ui.pushButton_reset_lighting, &QPushButton::clicked, this, &MainWindow::on_reset_lighting_clicked);
        connect(ui.pushButton_reset_view, &QPushButton::clicked, this, &MainWindow::on_reset_view_clicked);

        connect(ui.slider_clip_plane, &QSlider::valueChanged, this, &MainWindow::on_clip_plane_changed);
        connect(ui.slider_dft_brightness, &QSlider::valueChanged, this, &MainWindow::on_dft_brightness_changed);
        connect(ui.slider_isosurface_transparency, &QSlider::valueChanged, this,
                &MainWindow::on_isosurface_transparency_changed);
        connect(ui.slider_isovalue, &QSlider::valueChanged, this, &MainWindow::on_isovalue_changed);
        connect(ui.slider_lighting_intensity, &QSlider::valueChanged, this, &MainWindow::on_lighting_intensity_changed);
        connect(ui.slider_mesh_ambient, &QSlider::valueChanged, this, &MainWindow::on_mesh_ambient_changed);
        connect(ui.slider_mesh_diffuse, &QSlider::valueChanged, this, &MainWindow::on_mesh_diffuse_changed);
        connect(ui.slider_mesh_specular_power, &QSlider::valueChanged, this,
                &MainWindow::on_mesh_specular_power_changed);
        connect(ui.slider_mesh_specular, &QSlider::valueChanged, this, &MainWindow::on_mesh_specular_changed);
        connect(ui.slider_mesh_transparency, &QSlider::valueChanged, this, &MainWindow::on_mesh_transparency_changed);
        connect(ui.slider_normals, &QSlider::valueChanged, this, &MainWindow::on_normals_changed);
        connect(ui.slider_shadow_quality, &QSlider::valueChanged, this, &MainWindow::on_shadow_quality_changed);
        connect(ui.slider_volume_ambient, &QSlider::valueChanged, this, &MainWindow::on_volume_ambient_changed);
        connect(ui.slider_volume_diffuse, &QSlider::valueChanged, this, &MainWindow::on_volume_diffuse_changed);
        connect(ui.slider_volume_specular_power, &QSlider::valueChanged, this,
                &MainWindow::on_volume_specular_power_changed);
        connect(ui.slider_volume_specular, &QSlider::valueChanged, this, &MainWindow::on_volume_specular_changed);
        connect(ui.slider_volume_transparency, &QSlider::valueChanged, this,
                &MainWindow::on_volume_transparency_changed);

        connect(ui.toolButton_background_color, &QToolButton::clicked, this, &MainWindow::on_background_color_clicked);
        connect(ui.toolButton_clip_plane_color, &QToolButton::clicked, this, &MainWindow::on_clip_plane_color_clicked);
        connect(ui.toolButton_dft_background_color, &QToolButton::clicked, this,
                &MainWindow::on_dft_background_color_clicked);
        connect(ui.toolButton_dft_color, &QToolButton::clicked, this, &MainWindow::on_dft_color_clicked);
        connect(ui.toolButton_mesh_color, &QToolButton::clicked, this, &MainWindow::on_mesh_color_clicked);
        connect(ui.toolButton_normal_color_negative, &QToolButton::clicked, this,
                &MainWindow::on_normal_color_negative_clicked);
        connect(ui.toolButton_normal_color_positive, &QToolButton::clicked, this,
                &MainWindow::on_normal_color_positive_clicked);
        connect(ui.toolButton_volume_color, &QToolButton::clicked, this, &MainWindow::on_volume_color_clicked);
        connect(ui.toolButton_wireframe_color, &QToolButton::clicked, this, &MainWindow::on_wireframe_color_clicked);

        connect(m_slider_volume_levels.get(), &RangeSlider::changed, this, &MainWindow::on_volume_levels_changed);

        connect(&m_timer_progress_bar, &QTimer::timeout, this, &MainWindow::on_timer_progress_bar);
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

        m_model_events.clear();

        m_model_tree.reset();
        m_view.reset();
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

void MainWindow::set_progress_bars(
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

void MainWindow::on_timer_progress_bar()
{
        for (const WorkerThreads::Progress& t : m_worker_threads->progresses())
        {
                set_progress_bars(t.action, t.permanent, t.progress_list, t.progress_bars);
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

void MainWindow::showEvent(QShowEvent* /*event*/)
{
        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        // Окно ещё не видно, поэтому небольшая задержка, чтобы окно реально появилось.
        QTimer::singleShot(WINDOW_SHOW_DELAY_MSEC, this, &MainWindow::on_first_shown);
}

void MainWindow::on_first_shown()
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
                        view::command::SetLightingIntensity(lighting_intensity()),
                        view::command::SetDftBrightness(dft_brightness()),
                        view::command::SetDftBackgroundColor(qcolor_to_rgb(m_dft_background_color)),
                        view::command::SetDftColor(qcolor_to_rgb(m_dft_color)),
                        view::command::SetVerticalSync(ui.checkBox_vertical_sync->isChecked()),
                        view::command::SetShadowZoom(shadow_zoom())};

                m_view = view::create_view(
                        widget_window_id(m_graphics_widget), widget_pixels_per_inch(m_graphics_widget),
                        std::move(view_initial_commands));
                m_model_events.set_view(m_view.get());

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

void MainWindow::load_from_file(const std::string& file_name, bool use_object_selection_dialog)
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

void MainWindow::on_load_triggered()
{
        load_from_file("", true);
}

void MainWindow::on_repository_mesh(int dimension, const std::string& object_name)
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

void MainWindow::on_repository_volume(int dimension, const std::string& object_name)
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

void MainWindow::on_export_triggered()
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

void MainWindow::on_bound_cocone_triggered()
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

void MainWindow::on_exit_triggered()
{
        close();
}

void MainWindow::on_help_triggered()
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

void MainWindow::on_self_test_triggered()
{
        self_test(test::SelfTestType::Extended, true);
}

void MainWindow::on_painter_triggered()
{
        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;
        static constexpr const char* DESCRIPTION = "Painter";

        catch_all(DESCRIPTION, [&]() {
                if (!stop_action(ACTION))
                {
                        return;
                }

                std::vector<storage::MeshObjectConst> objects = m_model_tree->const_mesh_objects();
                if (objects.empty())
                {
                        MESSAGE_WARNING("No objects to paint");
                        return;
                }

                view::info::Camera camera;
                m_view->receive({&camera});

                WorkerThreads::Function f = process::action_painter(
                        objects, camera, QMainWindow::windowTitle().toStdString(), qcolor_to_rgb(m_background_color),
                        lighting_intensity());

                m_worker_threads->start(ACTION, DESCRIPTION, std::move(f));
        });
}

void MainWindow::on_about_triggered()
{
        dialog::application_about();
}

void MainWindow::on_reset_view_clicked()
{
        m_view->send(view::command::ResetView());
}

void MainWindow::on_graphics_widget_mouse_wheel(QWheelEvent* e)
{
        if (m_view)
        {
                m_view->send(view::command::MouseWheel(e->x(), e->y(), e->angleDelta().ry() / 120.0));
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

void MainWindow::on_model_tree_update()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = m_model_tree->current_item();
        if (!id)
        {
                mesh_ui_disable();
                volume_ui_disable();
                return;
        }

        std::optional<storage::MeshObjectConst> mesh = m_model_tree->mesh_const_if_current(*id);
        if (mesh)
        {
                mesh_ui_set(*mesh);
        }
        else
        {
                mesh_ui_disable();
        }

        std::optional<storage::VolumeObjectConst> volume = m_model_tree->volume_const_if_current(*id);
        if (volume)
        {
                volume_ui_set(*volume);
        }
        else
        {
                volume_ui_disable();
        }
}

double MainWindow::lighting_intensity() const
{
        double v = 2.0 * slider_position(ui.slider_lighting_intensity);
        return (v <= 1.0) ? v : interpolation(1.0, MAXIMUM_LIGHTING_INTENSITY, v - 1.0);
}

void MainWindow::on_reset_lighting_clicked()
{
        bool yes;
        if (!dialog::message_question_default_yes("Reset lighting?", &yes) || !yes)
        {
                return;
        }

        set_slider_to_middle(ui.slider_lighting_intensity);
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

void MainWindow::on_lighting_intensity_changed(int)
{
        m_view->send(view::command::SetLightingIntensity(lighting_intensity()));
}

void MainWindow::on_dft_brightness_changed(int)
{
        m_view->send(view::command::SetDftBrightness(dft_brightness()));
}

void MainWindow::on_shadow_quality_changed(int)
{
        if (m_view)
        {
                m_view->send(view::command::SetShadowZoom(shadow_zoom()));
        }
}

void MainWindow::on_clip_plane_changed(int)
{
        m_view->send(view::command::ClipPlanePosition(slider_position(ui.slider_clip_plane)));
}

void MainWindow::on_normals_changed(int)
{
        m_view->send(view::command::SetNormalLength(normal_length()));
}

void MainWindow::on_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Background Color", m_background_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        ptr->set_background_color(c);
                }
        });
}

void MainWindow::on_wireframe_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Wireframe Color", m_wireframe_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_wireframe_color(c);
                }
        });
}

void MainWindow::on_clip_plane_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Clip Plane Color", m_clip_plane_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_clip_plane_color(c);
                }
        });
}

void MainWindow::on_normal_color_positive_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Positive Normal Color", m_normal_color_positive, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_normal_color_positive(c);
                }
        });
}

void MainWindow::on_normal_color_negative_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Negative Normal Color", m_normal_color_negative, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_normal_color_negative(c);
                }
        });
}

void MainWindow::on_dft_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("DFT Background Color", m_dft_background_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_dft_background_color(c);
                }
        });
}

void MainWindow::on_dft_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("DFT Color", m_dft_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_dft_color(c);
                }
        });
}

void MainWindow::on_shadow_clicked()
{
        bool checked = ui.checkBox_shadow->isChecked();

        ui.label_shadow_quality->setEnabled(checked);
        ui.slider_shadow_quality->setEnabled(checked);

        if (m_view)
        {
                m_view->send(view::command::ShowShadow(checked));
        }
}

void MainWindow::on_fog_clicked()
{
        m_view->send(view::command::ShowFog(ui.checkBox_fog->isChecked()));
}

void MainWindow::on_wireframe_clicked()
{
        m_view->send(view::command::ShowWireframe(ui.checkBox_wireframe->isChecked()));
}

void MainWindow::on_materials_clicked()
{
        m_view->send(view::command::ShowMaterials(ui.checkBox_materials->isChecked()));
}

void MainWindow::on_smooth_clicked()
{
        m_view->send(view::command::ShowSmooth(ui.checkBox_smooth->isChecked()));
}

void MainWindow::on_fps_clicked()
{
        m_view->send(view::command::ShowFps(ui.checkBox_fps->isChecked()));
}

void MainWindow::on_pencil_sketch_clicked()
{
        m_view->send(view::command::ShowPencilSketch(ui.checkBox_pencil_sketch->isChecked()));
}

void MainWindow::on_dft_clicked()
{
        bool checked = ui.checkBox_dft->isChecked();

        ui.label_dft_brightness->setEnabled(checked);
        ui.slider_dft_brightness->setEnabled(checked);

        if (m_view)
        {
                m_view->send(view::command::ShowDft(checked));
        }
}

void MainWindow::on_clip_plane_clicked()
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

void MainWindow::on_normals_clicked()
{
        bool checked = ui.checkBox_normals->isChecked();
        ui.slider_normals->setEnabled(checked);
        m_view->send(view::command::ShowNormals(checked));
}

void MainWindow::on_convex_hull_2d_clicked()
{
        m_view->send(view::command::ShowConvexHull2D(ui.checkBox_convex_hull_2d->isChecked()));
}

void MainWindow::on_optical_flow_clicked()
{
        m_view->send(view::command::ShowOpticalFlow(ui.checkBox_optical_flow->isChecked()));
}

void MainWindow::on_vertical_sync_clicked()
{
        m_view->send(view::command::SetVerticalSync(ui.checkBox_vertical_sync->isChecked()));
}

void MainWindow::on_full_screen_triggered()
{
}

void MainWindow::mesh_ui_disable()
{
        ui.tabMesh->setEnabled(false);

        {
                QSignalBlocker blocker(ui.widget_mesh_color);
                set_widget_color(ui.widget_mesh_color, QColor(255, 255, 255));
        }
        {
                QSignalBlocker blocker(ui.slider_mesh_transparency);
                set_slider_position(ui.slider_mesh_transparency, 0);
        }
        {
                QSignalBlocker blocker(ui.slider_mesh_ambient);
                set_slider_to_middle(ui.slider_mesh_ambient);
        }
        {
                QSignalBlocker blocker(ui.slider_mesh_diffuse);
                set_slider_to_middle(ui.slider_mesh_diffuse);
        }
        {
                QSignalBlocker blocker(ui.slider_mesh_specular);
                set_slider_to_middle(ui.slider_mesh_specular);
        }
        {
                QSignalBlocker blocker(ui.slider_mesh_specular_power);
                set_slider_to_middle(ui.slider_mesh_specular_power);
        }
}

void MainWindow::volume_ui_disable()
{
        ui.tabVolume->setEnabled(false);

        {
                QSignalBlocker blocker(m_slider_volume_levels.get());
                m_slider_volume_levels->set_range(0, 1);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_transparency);
                set_slider_to_middle(ui.slider_volume_transparency);
        }
        {
                QSignalBlocker blocker(ui.checkBox_isosurface);
                ui.checkBox_isosurface->setChecked(false);
        }
        {
                QSignalBlocker blocker(ui.slider_isovalue);
                set_slider_to_middle(ui.slider_isovalue);
        }
        {
                QSignalBlocker blocker(ui.slider_isosurface_transparency);
                set_slider_position(ui.slider_isosurface_transparency, 0);
        }
        {
                QSignalBlocker blocker(ui.widget_volume_color);
                set_widget_color(ui.widget_volume_color, QColor(255, 255, 255));
        }
        {
                QSignalBlocker blocker(ui.slider_volume_ambient);
                set_slider_to_middle(ui.slider_volume_ambient);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_diffuse);
                set_slider_to_middle(ui.slider_volume_diffuse);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_specular);
                set_slider_to_middle(ui.slider_volume_specular);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_specular_power);
                set_slider_to_middle(ui.slider_volume_specular_power);
        }
}

void MainWindow::mesh_ui_set(const storage::MeshObjectConst& object)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        ui.tabMesh->setEnabled(true);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object) {
                        double alpha;
                        Color color;
                        double ambient, diffuse, specular, specular_power;
                        {
                                mesh::Reading reading(*mesh_object);
                                alpha = reading.alpha();
                                color = reading.color();
                                ambient = reading.ambient();
                                diffuse = reading.diffuse();
                                specular = reading.specular();
                                specular_power = reading.specular_power();
                        }
                        {
                                double position = 1.0 - alpha;
                                QSignalBlocker blocker(ui.slider_mesh_transparency);
                                set_slider_position(ui.slider_mesh_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.widget_mesh_color);
                                set_widget_color(ui.widget_mesh_color, color);
                        }
                        {
                                double position = ambient / MAXIMUM_MODEL_LIGHTING;
                                QSignalBlocker blocker(ui.slider_mesh_ambient);
                                set_slider_position(ui.slider_mesh_ambient, position);
                        }
                        {
                                double position = diffuse / MAXIMUM_MODEL_LIGHTING;
                                QSignalBlocker blocker(ui.slider_mesh_diffuse);
                                set_slider_position(ui.slider_mesh_diffuse, position);
                        }
                        {
                                double position = specular / MAXIMUM_MODEL_LIGHTING;
                                QSignalBlocker blocker(ui.slider_mesh_specular);
                                set_slider_position(ui.slider_mesh_specular, position);
                        }
                        {
                                double position = std::log(std::clamp(specular_power, 1.0, MAXIMUM_SPECULAR_POWER))
                                                  / std::log(MAXIMUM_SPECULAR_POWER);
                                QSignalBlocker blocker(ui.slider_mesh_specular_power);
                                set_slider_position(ui.slider_mesh_specular_power, position);
                        }
                },
                object);
}

void MainWindow::volume_ui_set(const storage::VolumeObjectConst& object)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

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
                        double ambient, diffuse, specular, specular_power;
                        {
                                volume::Reading reading(*volume_object);
                                min = reading.level_min();
                                max = reading.level_max();
                                volume_alpha_coefficient = reading.volume_alpha_coefficient();
                                isosurface_alpha = reading.isosurface_alpha();
                                isosurface = reading.isosurface();
                                isovalue = reading.isovalue();
                                color = reading.color();
                                ambient = reading.ambient();
                                diffuse = reading.diffuse();
                                specular = reading.specular();
                                specular_power = reading.specular_power();
                        }
                        {
                                QSignalBlocker blocker(m_slider_volume_levels.get());
                                m_slider_volume_levels->set_range(min, max);
                        }
                        {
                                volume_alpha_coefficient = std::clamp(
                                        volume_alpha_coefficient, 1.0 / VOLUME_ALPHA_COEFFICIENT,
                                        VOLUME_ALPHA_COEFFICIENT);
                                double log_volume_alpha_coefficient =
                                        std::log(volume_alpha_coefficient) / std::log(VOLUME_ALPHA_COEFFICIENT);
                                double position = 0.5 * (1.0 - log_volume_alpha_coefficient);
                                QSignalBlocker blocker(ui.slider_volume_transparency);
                                ui.slider_volume_transparency->setEnabled(!isosurface);
                                set_slider_position(ui.slider_volume_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.checkBox_isosurface);
                                ui.checkBox_isosurface->setChecked(isosurface);
                        }
                        {
                                double position = 1.0 - isosurface_alpha;
                                QSignalBlocker blocker(ui.slider_isosurface_transparency);
                                ui.slider_isosurface_transparency->setEnabled(isosurface);
                                set_slider_position(ui.slider_isosurface_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.slider_isovalue);
                                ui.slider_isovalue->setEnabled(isosurface);
                                set_slider_position(ui.slider_isovalue, isovalue);
                        }
                        {
                                QSignalBlocker blocker(ui.widget_volume_color);
                                set_widget_color(ui.widget_volume_color, color);
                        }
                        {
                                double position = ambient / MAXIMUM_MODEL_LIGHTING;
                                QSignalBlocker blocker(ui.slider_volume_ambient);
                                set_slider_position(ui.slider_volume_ambient, position);
                        }
                        {
                                double position = diffuse / MAXIMUM_MODEL_LIGHTING;
                                QSignalBlocker blocker(ui.slider_volume_diffuse);
                                set_slider_position(ui.slider_volume_diffuse, position);
                        }
                        {
                                double position = specular / MAXIMUM_MODEL_LIGHTING;
                                QSignalBlocker blocker(ui.slider_volume_specular);
                                set_slider_position(ui.slider_volume_specular, position);
                        }
                        {
                                double position = std::log(std::clamp(specular_power, 1.0, MAXIMUM_SPECULAR_POWER))
                                                  / std::log(MAXIMUM_SPECULAR_POWER);
                                QSignalBlocker blocker(ui.slider_volume_specular_power);
                                set_slider_position(ui.slider_volume_specular_power, position);
                        }
                },
                object);
}

void MainWindow::on_volume_levels_changed(double min, double max)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::Writing writing(volume_object.get());
                        writing.set_levels(min, max);
                },
                *volume_object_opt);
}

void MainWindow::on_volume_transparency_changed(int)
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
                        volume::Writing writing(volume_object.get());
                        writing.set_volume_alpha_coefficient(alpha_coefficient);
                },
                *volume_object_opt);
}

void MainWindow::on_isosurface_transparency_changed(int)
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
                        volume::Writing writing(volume_object.get());
                        writing.set_isosurface_alpha(alpha);
                },
                *volume_object_opt);
}

void MainWindow::on_isosurface_clicked()
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
                        volume::Writing writing(volume_object.get());
                        writing.set_isosurface(checked);
                },
                *volume_object_opt);
}

void MainWindow::on_isovalue_changed(int)
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
                        volume::Writing writing(volume_object.get());
                        writing.set_isovalue(isovalue);
                },
                *volume_object_opt);
}

void MainWindow::on_mesh_transparency_changed(int)
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
                        mesh::Writing writing(object.get());
                        writing.set_alpha(alpha);
                },
                *object_opt);
}

void MainWindow::on_mesh_color_clicked()
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
                        color = reading.color();
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
                                mesh::Writing writing(object.get());
                                writing.set_color(qcolor_to_rgb(c));
                        },
                        *object_opt);
        });
}

void MainWindow::on_volume_color_clicked()
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
                        color = reading.color();
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
                                volume::Writing writing(object.get());
                                writing.set_color(qcolor_to_rgb(c));
                        },
                        *object_opt);
        });
}

void MainWindow::on_mesh_ambient_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double ambient = MAXIMUM_MODEL_LIGHTING * slider_position(ui.slider_mesh_ambient);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                        mesh::Writing writing(object.get());
                        writing.set_ambient(ambient);
                },
                *object_opt);
}

void MainWindow::on_mesh_diffuse_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double diffuse = MAXIMUM_MODEL_LIGHTING * slider_position(ui.slider_mesh_diffuse);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                        mesh::Writing writing(object.get());
                        writing.set_diffuse(diffuse);
                },
                *object_opt);
}

void MainWindow::on_mesh_specular_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double specular = MAXIMUM_MODEL_LIGHTING * slider_position(ui.slider_mesh_specular);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                        mesh::Writing writing(object.get());
                        writing.set_specular(specular);
                },
                *object_opt);
}

void MainWindow::on_mesh_specular_power_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double specular_power = std::pow(MAXIMUM_SPECULAR_POWER, slider_position(ui.slider_mesh_specular_power));

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                        mesh::Writing writing(object.get());
                        writing.set_specular_power(specular_power);
                },
                *object_opt);
}

void MainWindow::on_volume_ambient_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double ambient = MAXIMUM_MODEL_LIGHTING * slider_position(ui.slider_volume_ambient);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_ambient(ambient);
                },
                *object_opt);
}

void MainWindow::on_volume_diffuse_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double diffuse = MAXIMUM_MODEL_LIGHTING * slider_position(ui.slider_volume_diffuse);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_diffuse(diffuse);
                },
                *object_opt);
}

void MainWindow::on_volume_specular_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double specular = MAXIMUM_MODEL_LIGHTING * slider_position(ui.slider_volume_specular);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_specular(specular);
                },
                *object_opt);
}

void MainWindow::on_volume_specular_power_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double specular_power = std::pow(MAXIMUM_SPECULAR_POWER, slider_position(ui.slider_volume_specular_power));

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_specular_power(specular_power);
                },
                *object_opt);
}
}
