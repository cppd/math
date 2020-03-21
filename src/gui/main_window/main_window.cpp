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

#include "../command_line/command_line.h"
#include "../dialogs/messages/application_about.h"
#include "../dialogs/messages/application_help.h"
#include "../dialogs/messages/color_dialog.h"
#include "../dialogs/messages/file_dialog.h"
#include "../dialogs/messages/message_box.h"
#include "../dialogs/messages/source_error.h"
#include "../dialogs/parameters/bound_cocone.h"
#include "../dialogs/parameters/object_selection.h"
#include "../dialogs/parameters/point_object.h"
#include "../painter_window/painting.h"
#include "../support/support.h"

#include <src/application/name.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/show/create.h>
#include <src/storage/manage.h>
#include <src/utility/file/sys.h>

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPointer>
#include <QSignalBlocker>

// Размер окна по сравнению с экраном.
constexpr double WINDOW_SIZE_COEF = 0.7;
// Если true, то размер для графики, если false, то размер всего окна.
constexpr bool WINDOW_SIZE_GRAPHICS = true;

constexpr double DFT_MAX_BRIGHTNESS = 50000;
constexpr double DFT_GAMMA = 0.5;

constexpr int BOUND_COCONE_MINIMUM_RHO_EXPONENT = -3;
constexpr int BOUND_COCONE_MINIMUM_ALPHA_EXPONENT = -3;
constexpr double BOUND_COCONE_DEFAULT_RHO = 0.3;
constexpr double BOUND_COCONE_DEFAULT_ALPHA = 0.14;

// Таймер отображения хода расчётов. Величина в миллисекундах.
constexpr int TIMER_PROGRESS_BAR_INTERVAL = 100;

// Количество точек для готовых объектов.
constexpr int POINT_COUNT_MINIMUM = 100;
constexpr int POINT_COUNT_DEFAULT = 10000;
constexpr int POINT_COUNT_MAXIMUM = 1000000;

// Цвета по умолчанию
constexpr QRgb BACKGROUND_COLOR = qRgb(50, 100, 150);
constexpr QRgb DEFAULT_COLOR = qRgb(150, 170, 150);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);
constexpr QRgb CLIP_PLANE_COLOR = qRgb(250, 230, 150);
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна.
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

// увеличение текстуры тени по сравнению с размером окна.
constexpr int SHADOW_ZOOM = 2;

// Количество лучей на один пиксель на одно измерение в одном проходе.
// Тогда для количества измерений D в пространстве экрана количество
// лучей равно std::pow(эта_величина, D).
constexpr int PAINTER_DEFAULT_SAMPLES_PER_DIMENSION = 5;
constexpr int PAINTER_MAX_SAMPLES_PER_DIMENSION = 10;

// Максимальный размер экрана в пикселях для 3 измерений
constexpr int PAINTER_3D_MAX_SCREEN_SIZE = 10000;

// Размеры экрана в пикселях для 4 и более измерений
constexpr int PAINTER_DEFAULT_SCREEN_SIZE = 500;
constexpr int PAINTER_MINIMUM_SCREEN_SIZE = 50;
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE = 5000;

// Сколько потоков не надо использовать от максимума для создания октадеревьев.
constexpr int MESH_OBJECT_NOT_USED_THREAD_COUNT = 2;

// Максимальное увеличение для освещений ambient, diffuse, specular.
constexpr double MAXIMUM_COLOR_AMPLIFICATION = 3;

constexpr float NORMAL_LENGTH_MINIMUM = 0.001;
constexpr float NORMAL_LENGTH_DEFAULT = 0.05;
constexpr float NORMAL_LENGTH_MAXIMUM = 0.2;
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(200, 200, 0);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(50, 150, 50);

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          m_window_thread_id(std::this_thread::get_id()),
          m_event_emitter(this),
          m_first_show(true),
          m_bound_cocone_rho(BOUND_COCONE_DEFAULT_RHO),
          m_bound_cocone_alpha(BOUND_COCONE_DEFAULT_ALPHA),
          m_dimension(0),
          m_close_without_confirmation(false),
          m_mesh_threads(std::max(1, hardware_concurrency() - MESH_OBJECT_NOT_USED_THREAD_COUNT))
{
        static_assert(std::is_same_v<decltype(ui.graphics_widget), GraphicsWidget*>);
        static_assert(std::is_same_v<decltype(ui.model_tree), ModelTree*>);

        LOG(command_line_description() + "\n");

        ui.setupUi(this);

        constructor_threads();
        constructor_connect();
        constructor_interface();
        constructor_objects_and_repository();

        set_log_events(&m_event_emitter);
}

void MainWindow::constructor_threads()
{
        auto handler = [this](const std::exception_ptr& ptr, const std::string& msg) {
                exception_handler(ptr, msg, true);
        };

        m_worker_threads = create_worker_threads(handler);
}

void MainWindow::constructor_connect()
{
        connect(ui.graphics_widget, SIGNAL(mouse_wheel(QWheelEvent*)), this,
                SLOT(graphics_widget_mouse_wheel(QWheelEvent*)));
        connect(ui.graphics_widget, SIGNAL(mouse_move(QMouseEvent*)), this,
                SLOT(graphics_widget_mouse_move(QMouseEvent*)));
        connect(ui.graphics_widget, SIGNAL(mouse_press(QMouseEvent*)), this,
                SLOT(graphics_widget_mouse_press(QMouseEvent*)));
        connect(ui.graphics_widget, SIGNAL(mouse_release(QMouseEvent*)), this,
                SLOT(graphics_widget_mouse_release(QMouseEvent*)));
        connect(ui.graphics_widget, SIGNAL(resize(QResizeEvent*)), this, SLOT(graphics_widget_resize(QResizeEvent*)));

        connect(ui.model_tree, SIGNAL(item_changed()), this, SLOT(model_tree_item_changed()));

        connect(&m_timer_progress_bar, SIGNAL(timeout()), this, SLOT(slot_timer_progress_bar()));
}

void MainWindow::constructor_interface()
{
        set_window_title_file("");

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

        set_widgets_enabled(QMainWindow::layout(), true);
        set_dependent_interface();

        set_background_color(BACKGROUND_COLOR);
        set_default_color(DEFAULT_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);
        set_clip_plane_color(CLIP_PLANE_COLOR);
        set_normal_color_positive(NORMAL_COLOR_POSITIVE);
        set_normal_color_negative(NORMAL_COLOR_NEGATIVE);

        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

        ui.mainWidget->layout()->setContentsMargins(3, 3, 3, 3);
        ui.mainWidget->layout()->setSpacing(3);

        ui.tabWidget->setCurrentIndex(0);

        ui.actionHelp->setText(QString(APPLICATION_NAME) + " Help");
        ui.actionAbout->setText("About " + QString(APPLICATION_NAME));

        ui.slider_shadow_quality->setSliderPosition(SHADOW_ZOOM);

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.statusBar->setFixedHeight(ui.statusBar->height());

        // Должно быть точное среднее положение
        ASSERT(((ui.slider_ambient->maximum() - ui.slider_ambient->minimum()) & 1) == 0);
        ASSERT(((ui.slider_diffuse->maximum() - ui.slider_diffuse->minimum()) & 1) == 0);
        ASSERT(((ui.slider_specular->maximum() - ui.slider_specular->minimum()) & 1) == 0);
}

void MainWindow::constructor_objects_and_repository()
{
        m_objects_to_load.insert(ComputationType::Mst);
        m_objects_to_load.insert(ComputationType::ConvexHull);
        m_objects_to_load.insert(ComputationType::Cocone);
        m_objects_to_load.insert(ComputationType::BoundCocone);

        m_storage = std::make_unique<MultiStorage>(m_event_emitter);

        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);

        std::vector<MultiStorage::RepositoryObjects> repository_objects = m_storage->repository_point_object_names();

        std::sort(repository_objects.begin(), repository_objects.end(), [](const auto& a, const auto& b) {
                return a.dimension < b.dimension;
        });

        for (const auto& dimension_objects : repository_objects)
        {
                ASSERT(dimension_objects.dimension > 0);

                QMenu* sub_menu = ui.menuCreate->addMenu(space_name(dimension_objects.dimension).c_str());
                for (const std::string& name : dimension_objects.names)
                {
                        ASSERT(!name.empty());

                        std::string text = name + "...";
                        QAction* action = sub_menu->addAction(text.c_str());
                        m_action_to_dimension_and_object_name.try_emplace(action, dimension_objects.dimension, name);
                        connect(action, SIGNAL(triggered()), this, SLOT(slot_object_repository()));
                }
        }
}

void MainWindow::set_window_title_file(const std::string& file_name)
{
        std::string title = APPLICATION_NAME;

        if (!file_name.empty())
        {
                title += " - " + file_name;
        }

        QMainWindow::setWindowTitle(title.c_str());
}

MainWindow::~MainWindow()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        terminate_all_threads();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!m_close_without_confirmation)
        {
                QPointer ptr(this);

                if (!dialog::message_question_default_no(this, "Do you want to close the main window?"))
                {
                        if (!ptr.isNull())
                        {
                                event->ignore();
                        }
                        return;
                }

                if (ptr.isNull())
                {
                        return;
                }
        }

        terminate_all_threads();

        event->accept();
}

void MainWindow::close_without_confirmation()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        m_close_without_confirmation = true;

        close();
}

void MainWindow::terminate_all_threads()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        m_worker_threads->terminate_all();

        m_show = nullptr;
        m_show_object.reset();

        set_log_events(nullptr);
}

void MainWindow::exception_handler(const std::exception_ptr& ptr, const std::string& msg, bool window_exists) const
        noexcept
{
        try
        {
                ASSERT(window_exists || std::this_thread::get_id() == m_window_thread_id);

                try
                {
                        std::rethrow_exception(ptr);
                }
                catch (TerminateRequestException&)
                {
                }
                catch (ErrorSourceException& e)
                {
                        std::string s = !msg.empty() ? (msg + ":\n") : std::string();

                        if (window_exists)
                        {
                                m_event_emitter.message_error_source(s + e.msg(), e.src());
                        }
                        else
                        {
                                error_fatal("Exception caught.\n" + s + e.msg() + "\n" + e.src());
                        }
                }
                catch (std::exception& e)
                {
                        std::string s = !msg.empty() ? (msg + ":\n") : std::string();

                        if (window_exists)
                        {
                                m_event_emitter.message_error(s + e.what());
                        }
                        else
                        {
                                LOG("Exception caught.\n" + s + e.what());
                        }
                }
                catch (...)
                {
                        std::string s = !msg.empty() ? (msg + ":\n") : std::string();

                        if (window_exists)
                        {
                                m_event_emitter.message_error(s + "Unknown error");
                        }
                        else
                        {
                                LOG("Exception caught.\n" + s + "Unknown error");
                        }
                }
        }
        catch (...)
        {
                error_fatal("Exception in the main window exception handler");
        }
}

template <typename F>
void MainWindow::catch_all(const F& function) const noexcept
{
        try
        {
                ASSERT(std::this_thread::get_id() == m_window_thread_id);

                std::string message;
                QPointer ptr(this);
                try
                {
                        function(&message);
                }
                catch (...)
                {
                        exception_handler(std::current_exception(), message, !ptr.isNull());
                }
        }
        catch (...)
        {
                error_fatal("Exception in the main window catch all");
        }
}

bool MainWindow::dialog_object_selection(std::unordered_set<ComputationType>* objects_to_load)
{
        ASSERT(objects_to_load);

        bool model_convex_hull = objects_to_load->count(ComputationType::ConvexHull) != 0u;
        bool model_minumum_spanning_tree = objects_to_load->count(ComputationType::Mst) != 0u;
        bool cocone = objects_to_load->count(ComputationType::Cocone) != 0u;
        bool bound_cocone = objects_to_load->count(ComputationType::BoundCocone) != 0u;

        QPointer ptr(this);
        if (!dialog::object_selection(this, &model_convex_hull, &model_minumum_spanning_tree, &cocone, &bound_cocone))
        {
                return false;
        }
        if (ptr.isNull())
        {
                return false;
        }

        insert_or_erase(model_convex_hull, ComputationType::ConvexHull, objects_to_load);
        insert_or_erase(model_minumum_spanning_tree, ComputationType::Mst, objects_to_load);
        insert_or_erase(cocone, ComputationType::Cocone, objects_to_load);
        insert_or_erase(bound_cocone, ComputationType::BoundCocone, objects_to_load);

        return true;
}

bool MainWindow::stop_action(WorkerThreads::Action action)
{
        if (m_worker_threads->is_working(action))
        {
                QPointer ptr(this);
                if (!dialog::message_question_default_no(this, "There is work in progress.\nDo you want to continue?"))
                {
                        return false;
                }
                if (ptr.isNull())
                {
                        return false;
                }
        }

        m_worker_threads->terminate_quietly(action);

        return true;
}

void MainWindow::thread_load_from_file(std::string file_name, bool use_object_selection_dialog)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;

        catch_all([&](std::string* msg) {
                *msg = "Load from file";

                if (!stop_action(ACTION))
                {
                        return;
                }

                if (file_name.empty())
                {
                        ASSERT(use_object_selection_dialog);

                        std::string caption = "Open";
                        bool read_only = true;

                        std::vector<std::string> filters;
                        for (const FileFormat& v : formats_for_load<MultiStorage>())
                        {
                                filters.push_back(file_filter(v.name, v.extensions));
                        }

                        QPointer ptr(this);
                        if (!dialog::open_file(this, caption, filters, read_only, &file_name))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                if (use_object_selection_dialog)
                {
                        if (!dialog_object_selection(&m_objects_to_load))
                        {
                                return;
                        }
                }

                double rho = m_bound_cocone_rho;
                double alpha = m_bound_cocone_alpha;
                bool build_convex_hull = m_objects_to_load.count(ComputationType::ConvexHull) > 0;
                bool build_cocone = m_objects_to_load.count(ComputationType::Cocone) > 0;
                bool build_bound_cocone = m_objects_to_load.count(ComputationType::BoundCocone) > 0;
                bool build_mst = m_objects_to_load.count(ComputationType::Mst) > 0;

                auto f = [=, this](ProgressRatioList* progress_list, std::string* message) {
                        *message = "Load " + file_name;
                        load_from_file(
                                build_convex_hull, build_cocone, build_bound_cocone, build_mst, progress_list,
                                file_name, m_show->object_size(), m_show->object_position(), rho, alpha, m_mesh_threads,
                                [&](size_t dimension) { m_event_emitter.file_loaded(file_name, dimension); },
                                m_storage.get());
                };

                m_worker_threads->start(ACTION, std::move(f));
        });
}

void MainWindow::thread_load_from_repository(int dimension, const std::string& object_name)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;

        catch_all([&](std::string* msg) {
                *msg = "Load from repository";

                if (object_name.empty())
                {
                        m_event_emitter.message_error("Empty repository object name");
                        return;
                }

                if (!stop_action(ACTION))
                {
                        return;
                }

                int point_count;

                {
                        QPointer ptr(this);
                        if (!dialog::point_object_parameters(
                                    this, dimension, object_name, POINT_COUNT_DEFAULT, POINT_COUNT_MINIMUM,
                                    POINT_COUNT_MAXIMUM, &point_count))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                if (!dialog_object_selection(&m_objects_to_load))
                {
                        return;
                }

                double rho = m_bound_cocone_rho;
                double alpha = m_bound_cocone_alpha;
                bool build_convex_hull = m_objects_to_load.count(ComputationType::ConvexHull) > 0;
                bool build_cocone = m_objects_to_load.count(ComputationType::Cocone) > 0;
                bool build_bound_cocone = m_objects_to_load.count(ComputationType::BoundCocone) > 0;
                bool build_mst = m_objects_to_load.count(ComputationType::Mst) > 0;

                auto f = [=, this](ProgressRatioList* progress_list, std::string* message) {
                        *message = "Load " + space_name(dimension) + " " + object_name;
                        load_from_repository(
                                build_convex_hull, build_cocone, build_bound_cocone, build_mst, progress_list,
                                dimension, object_name, m_show->object_size(), m_show->object_position(), rho, alpha,
                                m_mesh_threads, point_count,
                                [&]() { m_event_emitter.file_loaded(object_name, dimension); }, m_storage.get());
                };

                m_worker_threads->start(ACTION, std::move(f));
        });
}

void MainWindow::thread_export(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;

        catch_all([&](std::string* msg) {
                *msg = "Export";

                if (!stop_action(ACTION))
                {
                        return;
                }

                std::optional<MultiStorage::ObjectVariant> object = m_storage->object(id);
                if (!object)
                {
                        m_event_emitter.message_warning("No object to export");
                        return;
                }

                {
                        QPointer ptr(this);
                        if (!dialog::message_question_default_no(
                                    this, "Only export of geometry is supported.\nDo you want to continue?"))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                if (m_dimension < 3)
                {
                        m_event_emitter.message_error("No dimension information");
                        return;
                }

                std::string file_name;

                std::string name;
                std::visit([&](const auto& v) { name = v->name(); }, *object);

                std::string caption = "Export " + name + " to OBJ";
                bool read_only = true;

                std::vector<std::string> filters;
                for (const FileFormat& v : formats_for_save(m_dimension))
                {
                        filters.push_back(file_filter(v.name, v.extensions));
                }

                QPointer ptr(this);
                if (!dialog::save_file(this, caption, filters, read_only, &file_name))
                {
                        return;
                }
                if (ptr.isNull())
                {
                        return;
                }

                auto f = [=, this](ProgressRatioList*, std::string* message) {
                        *message = "Export " + name + " to " + file_name;
                        save_to_file(id, file_name, name, *m_storage);
                        m_event_emitter.message_information(name + " exported to file " + file_name);
                };

                m_worker_threads->start(ACTION, std::move(f));
        });
}

void MainWindow::thread_bound_cocone(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::Work;

        catch_all([&](std::string* msg) {
                *msg = "Reload BoundCocone";

                if (!stop_action(ACTION))
                {
                        return;
                }

                if (!m_storage->object(id))
                {
                        m_event_emitter.message_warning("No object to compute BoundCocone");
                        return;
                }

                double rho = m_bound_cocone_rho;
                double alpha = m_bound_cocone_alpha;

                QPointer ptr(this);
                if (!dialog::bound_cocone_parameters(
                            this, BOUND_COCONE_MINIMUM_RHO_EXPONENT, BOUND_COCONE_MINIMUM_ALPHA_EXPONENT, &rho, &alpha))
                {
                        return;
                }
                if (ptr.isNull())
                {
                        return;
                }

                m_bound_cocone_rho = rho;
                m_bound_cocone_alpha = alpha;

                auto f = [=, this](ProgressRatioList* progress_list, std::string* message) {
                        *message = "BoundCocone Reconstruction";
                        compute_bound_cocone(progress_list, id, rho, alpha, m_mesh_threads, m_storage.get());
                };

                m_worker_threads->start(ACTION, std::move(f));
        });
}

void MainWindow::thread_self_test(SelfTestType test_type, bool with_confirmation)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        static constexpr WorkerThreads::Action ACTION = WorkerThreads::Action::SelfTest;

        catch_all([&](std::string* msg) {
                *msg = "Self-Test";

                if (!stop_action(ACTION))
                {
                        return;
                }

                if (with_confirmation)
                {
                        QPointer ptr(this);
                        if (!dialog::message_question_default_yes(this, "Run the Self-Test?"))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                auto f = [=, this](ProgressRatioList* progress_list, std::string* message) {
                        *message = "Self-Test";
                        self_test(test_type, progress_list, [&](const std::exception_ptr& ptr, const std::string& m) {
                                exception_handler(ptr, m, true);
                        });
                };

                m_worker_threads->start(ACTION, std::move(f));
        });
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

                                if (!menu->exec(QCursor::pos()) || menu.isNull())
                                {
                                        return;
                                }

                                if (ptr_this.isNull())
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

void MainWindow::slot_timer_progress_bar()
{
        for (const WorkerThreads::Progress& t : m_worker_threads->progresses())
        {
                progress_bars(t.action, t.permanent, t.progress_list, t.progress_bars);
        }
}

void MainWindow::set_background_color(const QColor& c)
{
        m_background_color = c;
        if (m_show)
        {
                m_show->set_background_color(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_background_color);
        ui.widget_background_color->setPalette(palette);
}

void MainWindow::set_default_color(const QColor& c)
{
        m_default_color = c;
        if (m_show)
        {
                m_show->set_default_color(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_default_color);
        ui.widget_default_color->setPalette(palette);
}

void MainWindow::set_wireframe_color(const QColor& c)
{
        m_wireframe_color = c;
        if (m_show)
        {
                m_show->set_wireframe_color(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_wireframe_color);
        ui.widget_wireframe_color->setPalette(palette);
}

void MainWindow::set_clip_plane_color(const QColor& c)
{
        m_clip_plane_color = c;
        if (m_show)
        {
                m_show->set_clip_plane_color(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_clip_plane_color);
        ui.widget_clip_plane_color->setPalette(palette);
}

void MainWindow::set_normal_color_positive(const QColor& c)
{
        m_normal_color_positive = c;
        if (m_show)
        {
                m_show->set_normal_color_positive(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_normal_color_positive);
        ui.widget_normal_color_positive->setPalette(palette);
}

void MainWindow::set_normal_color_negative(const QColor& c)
{
        m_normal_color_negative = c;
        if (m_show)
        {
                m_show->set_normal_color_negative(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_normal_color_negative);
        ui.widget_normal_color_negative->setPalette(palette);
}

void MainWindow::set_dft_background_color(const QColor& c)
{
        m_dft_background_color = c;
        if (m_show)
        {
                m_show->set_dft_background_color(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_dft_background_color);
        ui.widget_dft_background_color->setPalette(palette);
}

void MainWindow::set_dft_color(const QColor& c)
{
        m_dft_color = c;
        if (m_show)
        {
                m_show->set_dft_color(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_dft_color);
        ui.widget_dft_color->setPalette(palette);
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

void MainWindow::message_error(const std::string& msg)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(msg), TextEditMessageType::Error);
        dialog::message_critical(this, msg);
}

void MainWindow::message_error_fatal(const std::string& msg)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        std::string message = !msg.empty() ? msg : "Unknown Error. Exit failure.";

        add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Error);

        QPointer ptr(this);
        dialog::message_critical(this, message);
        if (ptr.isNull())
        {
                return;
        }

        close_without_confirmation();
}

void MainWindow::message_error_source(const std::string& msg, const std::string& src)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        std::string source = source_with_line_numbers(src);

        add_to_text_edit_and_to_stderr(
                ui.text_log, format_log_message(msg + "\n" + source), TextEditMessageType::Error);

        QPointer ptr(this);
        dialog::message_source_error(this, msg, source);
        if (ptr.isNull())
        {
                return;
        }

        close_without_confirmation();
}

void MainWindow::message_information(const std::string& msg)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(msg), TextEditMessageType::Information);

        dialog::message_information(this, msg);
}

void MainWindow::message_warning(const std::string& msg)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(msg), TextEditMessageType::Warning);

        dialog::message_warning(this, msg);
}

void MainWindow::show_object_loaded(ObjectId /*id*/)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        ASSERT(m_dimension == 3);
}

template <size_t N>
void MainWindow::loaded_object(const std::shared_ptr<const MeshObject<N>>& object, int dimension)
{
        if constexpr (N == 3)
        {
                if (m_show && dimension == 3)
                {
                        m_show->add_object(object);
                }
        }
        ui.model_tree->add_item(object->id(), object->name());
}

void MainWindow::loaded_object(ObjectId id, size_t dimension)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        std::optional<MultiStorage::ObjectVariant> object = m_storage->object(id);
        if (!object)
        {
                m_event_emitter.message_warning("No loaded object");
                return;
        }

        std::visit([&](const auto& v) { loaded_object(v, dimension); }, *object);
}

void MainWindow::loaded_mesh(ObjectId id, size_t /*dimension*/)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        //if (m_dimension != 3)
        //{
        //}

        std::optional<MultiStorage::ObjectVariant> object = m_storage->object(id);
        if (!object)
        {
                m_event_emitter.message_warning("No loaded object for mesh");
                return;
        }

        std::string object_name;
        std::visit([&](const auto& v) { object_name = v->name(); }, *object);

        ui.model_tree->add_item(id, object_name);
}

void MainWindow::deleted_object(ObjectId id, size_t dimension)
{
        if (m_show && dimension == 3)
        {
                m_show->delete_object(id);
        }
        ui.model_tree->delete_item(id);
}

void MainWindow::deleted_all(size_t dimension)
{
        if (m_show && dimension == 3)
        {
                m_show->delete_all_objects();
        }
        ui.model_tree->delete_all();
}

void MainWindow::file_loaded(const std::string& file_name, size_t dimension)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        std::string base_name = file_base_name(file_name);
        set_window_title_file(base_name + " [" + space_name(dimension) + "]");
        m_dimension = dimension;
}

void MainWindow::log(const std::string& msg)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        // Здесь без вызовов функции LOG, так как начнёт вызывать сама себя
        add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(msg), TextEditMessageType::Normal);
}

void MainWindow::showEvent(QShowEvent* /*event*/)
{
        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        // Окно ещё не видно, поэтому небольшая задержка, чтобы окно реально появилось.
        QTimer::singleShot(WINDOW_SHOW_DELAY_MSEC, this, SLOT(slot_window_first_shown()));
}

void MainWindow::slot_window_first_shown()
{
        m_timer_progress_bar.start(TIMER_PROGRESS_BAR_INTERVAL);

        if (WINDOW_SIZE_GRAPHICS)
        {
                QSize size = QDesktopWidget().screenGeometry(this).size() * WINDOW_SIZE_COEF;
                resize_window_widget(this, ui.graphics_widget, size);
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

                thread_self_test(SelfTestType::Essential, false);

                //

                ShowCreateInfo info;
                info.events = &m_event_emitter;
                info.window = widget_window_id(ui.graphics_widget);
                info.window_ppi = widget_pixels_per_inch(ui.graphics_widget);
                info.background_color = qcolor_to_rgb(m_background_color);
                info.default_color = qcolor_to_rgb(m_default_color);
                info.wireframe_color = qcolor_to_rgb(m_wireframe_color);
                info.clip_plane_color = qcolor_to_rgb(m_clip_plane_color);
                info.normal_length = normal_length();
                info.normal_color_positive = qcolor_to_rgb(m_normal_color_positive);
                info.normal_color_negative = qcolor_to_rgb(m_normal_color_negative);
                info.with_smooth = ui.checkBox_smooth->isChecked();
                info.with_wireframe = ui.checkBox_wireframe->isChecked();
                info.with_shadow = ui.checkBox_shadow->isChecked();
                info.with_fog = ui.checkBox_fog->isChecked();
                info.with_materials = ui.checkBox_materials->isChecked();
                info.with_fps = ui.checkBox_fps->isChecked();
                info.with_pencil_sketch = ui.checkBox_pencil_sketch->isChecked();
                info.with_dft = ui.checkBox_dft->isChecked();
                info.with_convex_hull = ui.checkBox_convex_hull_2d->isChecked();
                info.with_optical_flow = ui.checkBox_optical_flow->isChecked();
                info.with_normals = ui.checkBox_normals->isChecked();
                info.ambient = ambient_light();
                info.diffuse = diffuse_light();
                info.specular = specular_light();
                info.dft_brightness = dft_brightness();
                info.dft_background_color = qcolor_to_rgb(m_dft_background_color);
                info.dft_color = qcolor_to_rgb(m_dft_color);
                info.default_ns = default_ns();
                info.vertical_sync = ui.checkBox_vertical_sync->isChecked();
                info.shadow_zoom = shadow_zoom();

                m_show_object = create_show_object(info);
                m_show = &m_show_object->show();

                //

                if (!options.file_name.empty())
                {
                        thread_load_from_file(options.file_name, !options.no_object_selection_dialog);
                }
        }
        catch (std::exception& e)
        {
                m_event_emitter.message_error_fatal(e.what());
        }
        catch (...)
        {
                m_event_emitter.message_error_fatal("Error first show");
        }
}

void MainWindow::on_actionLoad_triggered()
{
        thread_load_from_file("", true);
}

void MainWindow::slot_object_repository()
{
        auto iter = m_action_to_dimension_and_object_name.find(sender());
        if (iter == m_action_to_dimension_and_object_name.cend())
        {
                m_event_emitter.message_error("Open object sender not found in map");
                return;
        }

        thread_load_from_repository(std::get<0>(iter->second), std::get<1>(iter->second));
}

void MainWindow::on_actionExport_triggered()
{
        std::optional<ObjectId> item = ui.model_tree->current_item();
        if (!item)
        {
                m_event_emitter.message_warning("No item selected to export");
                return;
        }
        thread_export(*item);
}

void MainWindow::on_actionBoundCocone_triggered()
{
        std::optional<ObjectId> item = ui.model_tree->current_item();
        if (!item)
        {
                m_event_emitter.message_warning("No item selected to export");
                return;
        }
        thread_bound_cocone(*item);
}

void MainWindow::on_actionExit_triggered()
{
        close();
}

void MainWindow::on_actionHelp_triggered()
{
        dialog::application_help(this);
}

void MainWindow::on_actionSelfTest_triggered()
{
        thread_self_test(SelfTestType::Extended, true);
}

void MainWindow::on_actionAbout_triggered()
{
        dialog::application_about(this);
}

void MainWindow::on_pushButton_reset_view_clicked()
{
        m_show->reset_view();
}

void MainWindow::graphics_widget_mouse_wheel(QWheelEvent* e)
{
        if (m_show)
        {
                m_show->mouse_wheel(e->x(), e->y(), e->angleDelta().ry() / 120.0);
        }
}

void MainWindow::graphics_widget_mouse_move(QMouseEvent* e)
{
        if (m_show)
        {
                m_show->mouse_move(e->x(), e->y());
        }
}

void MainWindow::graphics_widget_mouse_press(QMouseEvent* e)
{
        if (m_show)
        {
                if (e->button() == Qt::MouseButton::LeftButton)
                {
                        m_show->mouse_press(e->x(), e->y(), ShowMouseButton::Left);
                        return;
                }
                if (e->button() == Qt::MouseButton::RightButton)
                {
                        m_show->mouse_press(e->x(), e->y(), ShowMouseButton::Right);
                        return;
                }
        }
}

void MainWindow::graphics_widget_mouse_release(QMouseEvent* e)
{
        if (m_show)
        {
                if (e->button() == Qt::MouseButton::LeftButton)
                {
                        m_show->mouse_release(e->x(), e->y(), ShowMouseButton::Left);
                        return;
                }
                if (e->button() == Qt::MouseButton::RightButton)
                {
                        m_show->mouse_release(e->x(), e->y(), ShowMouseButton::Right);
                        return;
                }
        }
}

void MainWindow::graphics_widget_resize(QResizeEvent* e)
{
        if (m_show)
        {
                m_show->window_resize(e->size().width(), e->size().height());
        }
}

void MainWindow::model_tree_item_changed()
{
        std::optional<ObjectId> id = ui.model_tree->current_item();
        if (id && m_dimension == 3)
        {
                m_show->show_object(*id);
        }
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
        QPointer ptr(this);
        if (!dialog::message_question_default_yes(this, "Reset lighting?"))
        {
                return;
        }
        if (ptr.isNull())
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
        m_show->set_ambient(ambient_light());
}

void MainWindow::on_slider_diffuse_valueChanged(int)
{
        m_show->set_diffuse(diffuse_light());
}

void MainWindow::on_slider_specular_valueChanged(int)
{
        m_show->set_specular(specular_light());
}

void MainWindow::on_slider_dft_brightness_valueChanged(int)
{
        m_show->set_dft_brightness(dft_brightness());
}

void MainWindow::on_slider_default_ns_valueChanged(int)
{
        m_show->set_default_ns(default_ns());
}

void MainWindow::on_slider_shadow_quality_valueChanged(int)
{
        if (m_show)
        {
                m_show->set_shadow_zoom(shadow_zoom());
        }
}

void MainWindow::on_slider_clip_plane_valueChanged(int)
{
        m_show->clip_plane_position(slider_position(ui.slider_clip_plane));
}

void MainWindow::on_slider_normals_valueChanged(int)
{
        m_show->set_normal_length(normal_length());
}

void MainWindow::on_toolButton_background_color_clicked()
{
        dialog::color_dialog(
                this, "Background Color", m_background_color, [this](const QColor& c) { set_background_color(c); });
}

void MainWindow::on_toolButton_default_color_clicked()
{
        dialog::color_dialog(this, "Default Color", m_default_color, [this](const QColor& c) { set_default_color(c); });
}

void MainWindow::on_toolButton_wireframe_color_clicked()
{
        dialog::color_dialog(
                this, "Wireframe Color", m_wireframe_color, [this](const QColor& c) { set_wireframe_color(c); });
}

void MainWindow::on_toolButton_clip_plane_color_clicked()
{
        dialog::color_dialog(
                this, "Clip Plane Color", m_clip_plane_color, [this](const QColor& c) { set_clip_plane_color(c); });
}

void MainWindow::on_toolButton_normal_color_positive_clicked()
{
        dialog::color_dialog(this, "Positive Normal Color", m_normal_color_positive, [this](const QColor& c) {
                set_normal_color_positive(c);
        });
}

void MainWindow::on_toolButton_normal_color_negative_clicked()
{
        dialog::color_dialog(this, "Negative Normal Color", m_normal_color_negative, [this](const QColor& c) {
                set_normal_color_negative(c);
        });
}

void MainWindow::on_toolButton_dft_background_color_clicked()
{
        dialog::color_dialog(this, "DFT Background Color", m_dft_background_color, [this](const QColor& c) {
                set_dft_background_color(c);
        });
}

void MainWindow::on_toolButton_dft_color_clicked()
{
        dialog::color_dialog(this, "DFT Color", m_dft_color, [this](const QColor& c) { set_dft_color(c); });
}

void MainWindow::on_checkBox_shadow_clicked()
{
        bool checked = ui.checkBox_shadow->isChecked();

        ui.label_shadow_quality->setEnabled(checked);
        ui.slider_shadow_quality->setEnabled(checked);

        m_show->show_shadow(checked);
}

void MainWindow::on_checkBox_fog_clicked()
{
        m_show->show_fog(ui.checkBox_fog->isChecked());
}

void MainWindow::on_checkBox_wireframe_clicked()
{
        m_show->show_wireframe(ui.checkBox_wireframe->isChecked());
}

void MainWindow::on_checkBox_materials_clicked()
{
        m_show->show_materials(ui.checkBox_materials->isChecked());
}

void MainWindow::on_checkBox_smooth_clicked()
{
        m_show->show_smooth(ui.checkBox_smooth->isChecked());
}

void MainWindow::on_checkBox_fps_clicked()
{
        m_show->show_fps(ui.checkBox_fps->isChecked());
}

void MainWindow::on_checkBox_pencil_sketch_clicked()
{
        m_show->show_pencil_sketch(ui.checkBox_pencil_sketch->isChecked());
}

void MainWindow::on_checkBox_dft_clicked()
{
        bool checked = ui.checkBox_dft->isChecked();

        ui.label_dft_brightness->setEnabled(checked);
        ui.slider_dft_brightness->setEnabled(checked);

        m_show->show_dft(checked);
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
                m_show->clip_plane_show(slider_position(ui.slider_clip_plane));
        }
        else
        {
                m_show->clip_plane_hide();
        }
}

void MainWindow::on_checkBox_normals_clicked()
{
        bool checked = ui.checkBox_normals->isChecked();
        ui.slider_normals->setEnabled(checked);
        m_show->show_normals(checked);
}

void MainWindow::on_checkBox_convex_hull_2d_clicked()
{
        m_show->show_convex_hull_2d(ui.checkBox_convex_hull_2d->isChecked());
}

void MainWindow::on_checkBox_optical_flow_clicked()
{
        m_show->show_optical_flow(ui.checkBox_optical_flow->isChecked());
}

void MainWindow::on_checkBox_vertical_sync_clicked()
{
        m_show->set_vertical_sync(ui.checkBox_vertical_sync->isChecked());
}

void MainWindow::on_actionFullScreen_triggered()
{
}

template <template <size_t, typename> typename SpatialMeshModel, size_t N, typename T>
void MainWindow::paint(const std::shared_ptr<const SpatialMeshModel<N, T>>& mesh, const std::string& object_name)
{
        ASSERT(mesh);

        PaintingInformationAll info_all;

        info_all.parent_window = this;
        info_all.window_title = QMainWindow::windowTitle().toStdString();
        info_all.object_name = object_name;
        info_all.default_samples_per_dimension = PAINTER_DEFAULT_SAMPLES_PER_DIMENSION;
        info_all.max_samples_per_dimension = PAINTER_MAX_SAMPLES_PER_DIMENSION;
        info_all.background_color = qcolor_to_rgb(m_background_color);
        info_all.default_color = qcolor_to_rgb(m_default_color);
        info_all.diffuse = diffuse_light();

        if constexpr (N == 3)
        {
                ShowCameraInfo c = m_show->camera_information();

                PaintingInformation3d<T> info;

                info.camera_up = to_vector<T>(c.camera_up);
                info.camera_direction = to_vector<T>(c.camera_direction);
                info.light_direction = to_vector<T>(c.light_direction);
                info.view_center = to_vector<T>(c.view_center);
                info.view_width = c.view_width;
                info.paint_width = c.width;
                info.paint_height = c.height;
                info.object_position = to_vector<T>(m_show->object_position());
                info.object_size = m_show->object_size();
                info.max_screen_size = PAINTER_3D_MAX_SCREEN_SIZE;

                painting(mesh, info, info_all);
        }
        else
        {
                PaintingInformationNd info;

                info.default_screen_size = PAINTER_DEFAULT_SCREEN_SIZE;
                info.minimum_screen_size = PAINTER_MINIMUM_SCREEN_SIZE;
                info.maximum_screen_size = PAINTER_MAXIMUM_SCREEN_SIZE;

                painting(mesh, info, info_all);
        }
}

void MainWindow::on_actionPainter_triggered()
{
        std::optional<ObjectId> item = ui.model_tree->current_item();
        if (!item)
        {
                m_event_emitter.message_warning("No item selected to paint");
                return;
        }

        ObjectId object_id = *item;

        std::optional<MultiStorage::ObjectVariant> object = m_storage->object(object_id);
        if (!object)
        {
                m_event_emitter.message_warning("No object to paint");
                return;
        }

        std::optional<MultiStorage::MeshVariant> mesh = m_storage->mesh(object_id);
        if (!mesh)
        {
                m_event_emitter.message_warning("No object to paint");
                return;
        }

        std::string object_name;
        std::visit([&](const auto& v) { object_name = v->name(); }, *object);

        catch_all([&](std::string* message) {
                *message = "Painter";

                std::visit([&](const auto& v) { paint(v, object_name); }, *mesh);
        });
}
