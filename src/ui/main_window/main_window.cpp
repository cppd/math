/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "application/application_name.h"
#include "com/alg.h"
#include "com/error.h"
#include "com/file/file_sys.h"
#include "com/log.h"
#include "com/math.h"
#include "com/names.h"
#include "com/print.h"
#include "ui/command_line/command_line.h"
#include "ui/dialogs/messages/application_about.h"
#include "ui/dialogs/messages/application_help.h"
#include "ui/dialogs/messages/color_dialog.h"
#include "ui/dialogs/messages/file_dialog.h"
#include "ui/dialogs/messages/message_box.h"
#include "ui/dialogs/messages/source_error.h"
#include "ui/dialogs/parameters/bound_cocone.h"
#include "ui/dialogs/parameters/object_selection.h"
#include "ui/dialogs/parameters/point_object.h"
#include "ui/support/support.h"

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPointer>

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)
constexpr bool SHOW_VULKAN_WINDOW = false;
#endif

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
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна.
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

// увеличение текстуры тени по сравнению с размером окна.
constexpr int SHADOW_ZOOM = 2;

// Для трассировки пути. Количество лучей на один пиксель на одно измерение
// в одном проходе. Тогда для количества измерений D в пространстве экрана
// количество лучей равно std::pow(эта_величина, D).
constexpr int PATH_TRACING_DEFAULT_SAMPLES_PER_DIMENSION = 5;
constexpr int PATH_TRACING_MAX_SAMPLES_PER_DIMENSION = 10;

// Для трассировки пути для 3 измерений. Максимальный размер экрана в пикселях.
constexpr int PATH_TRACING_3D_MAX_SCREEN_SIZE = 10000;

// Для трассировки пути для 4 и более измерений. Размеры экрана в пикселях.
constexpr int PATH_TRACING_DEFAULT_SCREEN_SIZE = 500;
constexpr int PATH_TRACING_MINIMUM_SCREEN_SIZE = 50;
constexpr int PATH_TRACING_MAXIMUM_SCREEN_SIZE = 5000;

// Сколько потоков не надо использовать от максимума для создания октадеревьев.
constexpr int MESH_OBJECT_NOT_USED_THREAD_COUNT = 2;

// Максимальное увеличение для освещений ambient, diffuse, specular.
constexpr double MAXIMUM_COLOR_AMPLIFICATION = 3;

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          m_window_thread_id(std::this_thread::get_id()),
          m_threads([this](const std::exception_ptr& ptr, const std::string& msg) noexcept {
                  exception_handler(ptr, msg, true);
          }),
          m_objects(create_main_objects(
                  std::max(1, hardware_concurrency() - MESH_OBJECT_NOT_USED_THREAD_COUNT), m_event_emitter,
                  [this](const std::exception_ptr& ptr, const std::string& msg) noexcept { exception_handler(ptr, msg, true); })),
          m_first_show(true),
          m_dimension(0),
          m_close_without_confirmation(false),
          m_objects_to_load(default_objects_to_load())
{
        static_assert(std::is_same_v<decltype(ui.graphics_widget_opengl), GraphicsWidget*>);
        static_assert(std::is_same_v<decltype(ui.graphics_widget_vulkan), GraphicsWidget*>);

        LOG(command_line_description() + "\n");

        ui.setupUi(this);

        constructor_connect();
        constructor_interface();
        constructor_repository();
        constructor_buttons();

        set_log_callback(&m_event_emitter);
}

void MainWindow::constructor_connect()
{
        qRegisterMetaType<WindowEvent>("WindowEvent");
        connect(&m_event_emitter, SIGNAL(window_event(WindowEvent)), this, SLOT(slot_window_event(WindowEvent)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        ui.graphics_widget_opengl->setText("");
        connect(ui.graphics_widget_opengl, SIGNAL(wheel(double)), this, SLOT(slot_widget_opengl_mouse_wheel(double)));
        connect(ui.graphics_widget_opengl, SIGNAL(resize()), this, SLOT(slot_widget_opengl_resize()));

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)
        ui.graphics_widget_vulkan->setText("");
        connect(ui.graphics_widget_vulkan, SIGNAL(wheel(double)), this, SLOT(slot_widget_vulkan_mouse_wheel(double)));
        connect(ui.graphics_widget_vulkan, SIGNAL(resize()), this, SLOT(slot_widget_vulkan_resize()));
        if (!SHOW_VULKAN_WINDOW)
        {
                ui.graphics_widget_vulkan->setVisible(false);
        }
#else
        ui.graphics_widget_vulkan->setVisible(false);
#endif

        connect(&m_timer_progress_bar, SIGNAL(timeout()), this, SLOT(slot_timer_progress_bar()));
}

void MainWindow::constructor_interface()
{
        set_window_title_file("");

        QMainWindow::addAction(ui.actionFullScreen);

        set_widgets_enabled(QMainWindow::layout(), true);
        set_dependent_interface();
        reset_all_object_buttons(m_objects_to_load);

        set_bound_cocone_parameters(BOUND_COCONE_DEFAULT_RHO, BOUND_COCONE_DEFAULT_ALPHA);

        set_background_color(BACKGROUND_COLOR);
        set_default_color(DEFAULT_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);

        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

        ui.mainWidget->layout()->setContentsMargins(3, 3, 3, 3);
        ui.mainWidget->layout()->setSpacing(3);

        ui.radioButton_Model->setChecked(true);

        ui.tabWidget->setCurrentIndex(0);

        ui.actionHelp->setText(QString(APPLICATION_NAME) + " Help");
        ui.actionAbout->setText("About " + QString(APPLICATION_NAME));

        ui.slider_ShadowQuality->setSliderPosition(SHADOW_ZOOM);

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.statusBar->setFixedHeight(ui.statusBar->height());

        // Должно быть точное среднее положение
        ASSERT(((ui.slider_Ambient->maximum() - ui.slider_Ambient->minimum()) & 1) == 0);
        ASSERT(((ui.slider_Diffuse->maximum() - ui.slider_Diffuse->minimum()) & 1) == 0);
        ASSERT(((ui.slider_Specular->maximum() - ui.slider_Specular->minimum()) & 1) == 0);
}

void MainWindow::constructor_repository()
{
        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);
        for (const auto& [dimension, object_names] : m_objects->repository_point_object_names())
        {
                QMenu* sub_menu = ui.menuCreate->addMenu(space_name(dimension).c_str());
                for (const std::string& object_name : object_names)
                {
                        std::string text = object_name + "...";
                        QAction* action = sub_menu->addAction(text.c_str());
                        m_action_to_object_name_map.try_emplace(action, dimension, object_name);
                        connect(action, SIGNAL(triggered()), this, SLOT(slot_object_repository()));
                }
        }
}

void MainWindow::constructor_buttons()
{
        m_object_buttons.push_back({ui.radioButton_Model, ObjectId::Model});
        m_object_buttons.push_back({ui.radioButton_ModelMST, ObjectId::ModelMst});
        m_object_buttons.push_back({ui.radioButton_ModelConvexHull, ObjectId::ModelConvexHull});
        m_object_buttons.push_back({ui.radioButton_Cocone, ObjectId::Cocone});
        m_object_buttons.push_back({ui.radioButton_CoconeConvexHull, ObjectId::CoconeConvexHull});
        m_object_buttons.push_back({ui.radioButton_BoundCocone, ObjectId::BoundCocone});
        m_object_buttons.push_back({ui.radioButton_BoundCoconeConvexHull, ObjectId::BoundCoconeConvexHull});
}

std::unordered_set<ObjectId> MainWindow::default_objects_to_load()
{
        std::unordered_set<ObjectId> objects_to_load;

        // ObjectId::Model добавлять не нужно, так как загружается обязательно
        objects_to_load.insert(ObjectId::ModelMst);
        objects_to_load.insert(ObjectId::ModelConvexHull);
        objects_to_load.insert(ObjectId::Cocone);
        objects_to_load.insert(ObjectId::CoconeConvexHull);
        objects_to_load.insert(ObjectId::BoundCocone);
        objects_to_load.insert(ObjectId::BoundCoconeConvexHull);

        return objects_to_load;
}

void MainWindow::set_window_title_file(const std::string& file_name)
{
        std::string title = APPLICATION_NAME;

        if (file_name.size() > 0)
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

        m_threads.terminate_all_threads();

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)
        m_show_vulkan.reset();
#endif
        m_show.reset();

        set_log_callback(nullptr);
}

void MainWindow::exception_handler(const std::exception_ptr& ptr, const std::string& msg, bool window_exists) const noexcept
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

bool MainWindow::find_object(std::string* object_name, ObjectId* object_id)
{
        bool found = false;

        for (const auto& [button, id] : m_object_buttons)
        {
                if (button->isChecked())
                {
                        *object_name = button->text().toStdString();
                        *object_id = id;
                        found = true;
                        break;
                }
        }

        if (!found)
        {
                m_event_emitter.message_warning("No object button is checked");
                return false;
        }

        return true;
}

bool MainWindow::dialog_object_selection(QWidget* parent, std::unordered_set<ObjectId>* objects_to_load)
{
        ASSERT(objects_to_load);

        bool model_convex_hull = objects_to_load->count(ObjectId::ModelConvexHull);
        bool model_minumum_spanning_tree = objects_to_load->count(ObjectId::ModelMst);
        bool cocone = objects_to_load->count(ObjectId::Cocone);
        bool cocone_convex_hull = objects_to_load->count(ObjectId::CoconeConvexHull);
        bool bound_cocone = objects_to_load->count(ObjectId::BoundCocone);
        bool bound_cocone_convex_hull = objects_to_load->count(ObjectId::BoundCoconeConvexHull);

        if (!dialog::object_selection(parent, &model_convex_hull, &model_minumum_spanning_tree, &cocone, &cocone_convex_hull,
                                      &bound_cocone, &bound_cocone_convex_hull))
        {
                return false;
        }

        insert_or_erase(model_convex_hull, ObjectId::ModelConvexHull, objects_to_load);
        insert_or_erase(model_minumum_spanning_tree, ObjectId::ModelMst, objects_to_load);
        insert_or_erase(cocone, ObjectId::Cocone, objects_to_load);
        insert_or_erase(cocone_convex_hull, ObjectId::CoconeConvexHull, objects_to_load);
        insert_or_erase(bound_cocone, ObjectId::BoundCocone, objects_to_load);
        insert_or_erase(bound_cocone_convex_hull, ObjectId::BoundCoconeConvexHull, objects_to_load);

        return true;
}

void MainWindow::thread_load_from_file(std::string file_name, bool use_object_selection_dialog)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!m_threads.action_allowed(ThreadAction::LoadObject))
        {
                m_event_emitter.message_warning("File loading is not available at this time (thread working)");
                return;
        }

        catch_all([&](std::string* msg) {
                *msg = "Open file";

                if (file_name.size() == 0)
                {
                        ASSERT(use_object_selection_dialog);

                        std::string caption = "Open";
                        std::string filter =
                                file_filter("OBJ and Point files", m_objects->obj_extensions(), m_objects->txt_extensions());
                        bool read_only = true;

                        QPointer ptr(this);
                        if (!dialog::open_file(this, caption, filter, read_only, &file_name))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                std::unordered_set<ObjectId> objects_to_load = m_objects_to_load;

                if (use_object_selection_dialog)
                {
                        QPointer ptr(this);
                        if (!dialog_object_selection(this, &objects_to_load))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                m_threads.start_thread(ThreadAction::LoadObject, [=, rho = m_bound_cocone_rho, alpha = m_bound_cocone_alpha](
                                                                         ProgressRatioList* progress_list, std::string* message) {
                        *message = "Load " + file_name;

                        m_objects->load_from_file(objects_to_load, progress_list, file_name, rho, alpha);
                });
        });
}

void MainWindow::thread_load_from_repository(const std::tuple<int, std::string>& object)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!m_threads.action_allowed(ThreadAction::LoadObject))
        {
                m_event_emitter.message_warning("Creation of object is not available at this time (thread working)");
                return;
        }

        if (std::get<1>(object).size() == 0)
        {
                m_event_emitter.message_error("Empty repository object name");
                return;
        }

        catch_all([&](std::string* msg) {
                *msg = "Load from repository";

                int point_count;

                {
                        QPointer ptr(this);
                        if (!dialog::point_object_parameters(this, std::get<0>(object), std::get<1>(object), POINT_COUNT_DEFAULT,
                                                             POINT_COUNT_MINIMUM, POINT_COUNT_MAXIMUM, &point_count))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                std::unordered_set<ObjectId> objects_to_load = m_objects_to_load;

                {
                        QPointer ptr(this);
                        if (!dialog_object_selection(this, &objects_to_load))
                        {
                                return;
                        }
                        if (ptr.isNull())
                        {
                                return;
                        }
                }

                m_threads.start_thread(ThreadAction::LoadObject, [=, rho = m_bound_cocone_rho, alpha = m_bound_cocone_alpha](
                                                                         ProgressRatioList* progress_list, std::string* message) {
                        *message = "Load " + space_name(std::get<0>(object)) + " " + std::get<1>(object);

                        m_objects->load_from_repository(objects_to_load, progress_list, object, rho, alpha, point_count);
                });
        });
}

void MainWindow::thread_self_test(SelfTestType test_type, bool with_confirmation)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!m_threads.action_allowed(ThreadAction::SelfTest))
        {
                m_event_emitter.message_warning("Self-Test is not available at this time (thread working)");
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

        m_threads.start_thread(ThreadAction::SelfTest, [=](ProgressRatioList* progress_list, std::string* message) {
                *message = "Self-Test";

                self_test(test_type, progress_list, [&](const std::exception_ptr& ptr, const std::string& msg) noexcept {
                        exception_handler(ptr, msg, true);
                });
        });
}

void MainWindow::thread_export(const std::string& name, ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!m_threads.action_allowed(ThreadAction::ExportObject))
        {
                m_event_emitter.message_warning("Export " + name + " to file is not available at this time (thread working)");
                return;
        }

        if (!m_objects->object_exists(id))
        {
                m_event_emitter.message_warning("No object to export");
                return;
        }

        if (id == ObjectId::Model)
        {
                QPointer ptr(this);
                if (!dialog::message_question_default_no(this, "Only export of geometry is supported.\nDo you want to continue?"))
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

        catch_all([&](std::string* msg) {
                *msg = "Export to file";

                std::string file_name;

                std::string caption = "Export " + name + " to OBJ";
                std::string filter = file_filter("OBJ files", m_objects->obj_extension(m_dimension));
                bool read_only = true;

                QPointer ptr(this);
                if (!dialog::save_file(this, caption, filter, read_only, &file_name))
                {
                        return;
                }
                if (ptr.isNull())
                {
                        return;
                }

                m_threads.start_thread(ThreadAction::ExportObject, [=](ProgressRatioList*, std::string* message) {
                        *message = "Export " + name + " to " + file_name;

                        m_objects->save_to_file(id, file_name, name);
                        m_event_emitter.message_information(name + " exported to file " + file_name);
                });
        });
}

void MainWindow::thread_reload_bound_cocone()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (m_objects_to_load.count(ObjectId::BoundCocone) == 0 && m_objects_to_load.count(ObjectId::BoundCoconeConvexHull) == 0)
        {
                m_event_emitter.message_warning("Neither BoundCocone nor BoundCocone Convex Hull was selected for loading");
                return;
        }

        if (!m_threads.action_allowed(ThreadAction::ReloadBoundCocone))
        {
                m_event_emitter.message_warning("BoundCocone is not available at this time (thread working)");
                return;
        }

        if (!m_objects->manifold_constructor_exists())
        {
                m_event_emitter.message_warning("No manifold constructor");
                return;
        }

        catch_all([&](std::string* msg) {
                *msg = "Reload BoundCocone";

                double rho = m_bound_cocone_rho;
                double alpha = m_bound_cocone_alpha;

                QPointer ptr(this);
                if (!dialog::bound_cocone_parameters(this, BOUND_COCONE_MINIMUM_RHO_EXPONENT, BOUND_COCONE_MINIMUM_ALPHA_EXPONENT,
                                                     &rho, &alpha))
                {
                        return;
                }
                if (ptr.isNull())
                {
                        return;
                }

                m_threads.start_thread(
                        ThreadAction::ReloadBoundCocone,
                        [=, objects_to_load = m_objects_to_load](ProgressRatioList* progress_list, std::string* message) {
                                *message = "BoundCocone reconstruction";

                                m_objects->compute_bound_cocone(objects_to_load, progress_list, rho, alpha);
                        });
        });
}

void MainWindow::progress_bars(const ThreadAction& thread_action, bool permanent, const ProgressRatioList* progress_list,
                               std::list<QProgressBar>* progress_bars)
{
        static_assert(std::numeric_limits<unsigned>::max() >= std::numeric_limits<int>::max());

        constexpr unsigned MAX_INT = std::numeric_limits<int>::max();

        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios = progress_list->ratios();

        while (ratios.size() > progress_bars->size())
        {
                QProgressBar& bar = progress_bars->emplace_back();

                bar.setContextMenuPolicy(Qt::CustomContextMenu);

                connect(&bar, &QProgressBar::customContextMenuRequested,
                        [thread_action, &bar, ptr_this = QPointer(this)](const QPoint&) {
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

                                ptr_this->m_threads.terminate_thread_with_message(thread_action);
                        });
        }

        std::list<QProgressBar>::iterator bar = progress_bars->begin();

        for (unsigned i = 0; i < ratios.size(); ++i, ++bar)
        {
                if (!bar->isVisible())
                {
                        if (permanent)
                        {
                                ui.statusBar->addPermanentWidget(&(*bar));
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
        for (const ThreadProgress& t : m_threads.thread_progress())
        {
                progress_bars(t.thread_action, t.permanent, t.progress_list, t.progress_bars);
        }
}

void MainWindow::set_bound_cocone_parameters(double rho, double alpha)
{
        static_assert(BOUND_COCONE_MINIMUM_RHO_EXPONENT < 0);
        static_assert(BOUND_COCONE_MINIMUM_ALPHA_EXPONENT < 0);

        m_bound_cocone_rho = rho;
        m_bound_cocone_alpha = alpha;

        std::string label;
        label += u8"ρ " + to_string_fixed(rho, -BOUND_COCONE_MINIMUM_RHO_EXPONENT);
        label += "; ";
        label += u8"α " + to_string_fixed(alpha, -BOUND_COCONE_MINIMUM_ALPHA_EXPONENT);
        ui.BoundCocone_label->setText(label.c_str());
}

void MainWindow::set_background_color(const QColor& c)
{
        m_background_color = c;
        if (m_show)
        {
                m_show->set_background_color_rgb(qcolor_to_rgb(c));
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
                m_show->set_default_color_rgb(qcolor_to_rgb(c));
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
                m_show->set_wireframe_color_rgb(qcolor_to_rgb(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_wireframe_color);
        ui.widget_wireframe_color->setPalette(palette);
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
                bool enabled_and_checked = ui.checkBox_Shadow->isEnabled() && ui.checkBox_Shadow->isChecked();

                ui.label_ShadowQuality->setEnabled(enabled_and_checked);
                ui.slider_ShadowQuality->setEnabled(enabled_and_checked);
        }

        {
                bool enabled_and_checked = ui.checkBox_show_dft->isEnabled() && ui.checkBox_show_dft->isChecked();

                ui.label_DFT_Brightness->setEnabled(enabled_and_checked);
                ui.slider_DFT_Brightness->setEnabled(enabled_and_checked);
        }
}

void MainWindow::reset_object_button(QRadioButton* button, bool object_to_load)
{
        if (object_to_load)
        {
                button_strike_out(button, true);
                button->setEnabled(true);
        }
        else
        {
                button_strike_out(button, false);
                button->setEnabled(false);
        }
}

void MainWindow::show_object_button(QRadioButton* button)
{
        if (!button->isEnabled())
        {
                error_fatal("Loaded disabled object for button " + button->text().toStdString());
        }

        button_strike_out(button, false);

        if (button->isChecked())
        {
                button->click();
        }
}

void MainWindow::reset_all_object_buttons(const std::unordered_set<ObjectId>& objects_to_load)
{
        reset_object_button(ui.radioButton_Model, true);
        reset_object_button(ui.radioButton_ModelConvexHull, objects_to_load.count(ObjectId::ModelConvexHull) > 0);
        reset_object_button(ui.radioButton_ModelMST, objects_to_load.count(ObjectId::ModelMst) > 0);
        reset_object_button(ui.radioButton_Cocone, objects_to_load.count(ObjectId::Cocone) > 0);
        reset_object_button(ui.radioButton_CoconeConvexHull, objects_to_load.count(ObjectId::CoconeConvexHull) > 0);
        reset_object_button(ui.radioButton_BoundCocone, objects_to_load.count(ObjectId::BoundCocone) > 0);
        reset_object_button(ui.radioButton_BoundCoconeConvexHull, objects_to_load.count(ObjectId::BoundCoconeConvexHull) > 0);
}

void MainWindow::reset_bound_cocone_buttons(const std::unordered_set<ObjectId>& objects_to_load)
{
        reset_object_button(ui.radioButton_BoundCocone, objects_to_load.count(ObjectId::BoundCocone) > 0);
        reset_object_button(ui.radioButton_BoundCoconeConvexHull, objects_to_load.count(ObjectId::BoundCoconeConvexHull) > 0);
}

void MainWindow::slot_window_event(const WindowEvent& event)
{
        switch (event.type())
        {
        case WindowEvent::Type::MessageError:
        {
                const WindowEvent::message_error& d = event.get<WindowEvent::message_error>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Error);

                dialog::message_critical(this, message);

                break;
        }
        case WindowEvent::Type::MessageErrorFatal:
        {
                const WindowEvent::message_error_fatal& d = event.get<WindowEvent::message_error_fatal>();
                std::string message = (d.msg.size() != 0) ? d.msg : "Unknown Error. Exit failure.";

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Error);

                QPointer ptr(this);
                dialog::message_critical(this, message);
                if (ptr.isNull())
                {
                        return;
                }

                close_without_confirmation();

                break;
        }
        case WindowEvent::Type::MessageErrorSource:
        {
                const WindowEvent::message_error_source& d = event.get<WindowEvent::message_error_source>();
                std::string message = d.msg;
                std::string source = source_with_line_numbers(d.src);

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message + "\n" + source),
                                               TextEditMessageType::Error);

                QPointer ptr(this);
                dialog::message_source_error(this, message, source);
                if (ptr.isNull())
                {
                        return;
                }

                close_without_confirmation();

                break;
        }
        case WindowEvent::Type::MessageInformation:
        {
                const WindowEvent::message_information& d = event.get<WindowEvent::message_information>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Information);

                dialog::message_information(this, message);

                break;
        }
        case WindowEvent::Type::MessageWarning:
        {
                const WindowEvent::message_warning& d = event.get<WindowEvent::message_warning>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Warning);

                dialog::message_warning(this, message);

                break;
        }
        case WindowEvent::Type::WriteToLog:
        {
                // Здесь без вызовов функции LOG, так как начнёт вызывать сама себя

                const WindowEvent::write_to_log& d = event.get<WindowEvent::write_to_log>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Normal);

                break;
        }
        case WindowEvent::Type::LoadedObject:
        case WindowEvent::Type::LoadedMesh:
        {
                ObjectId id;

                if (event.type() == WindowEvent::Type::LoadedObject)
                {
                        ASSERT(m_dimension == 3);
                        id = int_to_object_id(event.get<WindowEvent::loaded_object>().id);
                }
                else
                {
                        if (m_dimension == 3)
                        {
                                break;
                        }
                        id = event.get<WindowEvent::loaded_mesh>().id;
                }

                switch (id)
                {
                case ObjectId::Model:
                        show_object_button(ui.radioButton_Model);
                        break;
                case ObjectId::ModelConvexHull:
                        show_object_button(ui.radioButton_ModelConvexHull);
                        break;
                case ObjectId::ModelMst:
                        show_object_button(ui.radioButton_ModelMST);
                        break;
                case ObjectId::Cocone:
                        show_object_button(ui.radioButton_Cocone);
                        break;
                case ObjectId::CoconeConvexHull:
                        show_object_button(ui.radioButton_CoconeConvexHull);
                        break;
                case ObjectId::BoundCocone:
                        show_object_button(ui.radioButton_BoundCocone);
                        break;
                case ObjectId::BoundCoconeConvexHull:
                        show_object_button(ui.radioButton_BoundCoconeConvexHull);
                        break;
                }

                break;
        }
        case WindowEvent::Type::LoadedFile:
        {
                const WindowEvent::loaded_file& d = event.get<WindowEvent::loaded_file>();

                std::string file_name = file_base_name(d.file_name);
                set_window_title_file(file_name + " [" + space_name(d.dimension) + "]");
                reset_all_object_buttons(d.objects);
                ui.radioButton_Model->setChecked(true);
                m_dimension = d.dimension;
                m_objects_to_load = d.objects;

                break;
        }
        case WindowEvent::Type::LoadedBoundCocone:
        {
                const WindowEvent::loaded_bound_cocone& d = event.get<WindowEvent::loaded_bound_cocone>();

                set_bound_cocone_parameters(d.rho, d.alpha);
                reset_bound_cocone_buttons(m_objects_to_load);

                break;
        }
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
        QTimer::singleShot(WINDOW_SHOW_DELAY_MSEC, this, SLOT(slot_window_first_shown()));
}

void MainWindow::slot_window_first_shown()
{
        m_timer_progress_bar.start(TIMER_PROGRESS_BAR_INTERVAL);

        if (WINDOW_SIZE_GRAPHICS)
        {
                QSize size = QDesktopWidget().screenGeometry(this).size() * WINDOW_SIZE_COEF;
                resize_window_widget(this, ui.graphics_widget_opengl, size);
        }
        else
        {
                QSize size = QDesktopWidget().availableGeometry(this).size() * WINDOW_SIZE_COEF;
                resize_window_frame(this, size);
        }

        move_window_to_desktop_center(this);

        thread_self_test(SelfTestType::Essential, false);

        try
        {
                m_show = create_show_opengl(
                        &m_event_emitter, widget_window_id(ui.graphics_widget_opengl), qcolor_to_rgb(m_background_color),
                        qcolor_to_rgb(m_default_color), qcolor_to_rgb(m_wireframe_color), ui.checkBox_Smooth->isChecked(),
                        ui.checkBox_Wireframe->isChecked(), ui.checkBox_Shadow->isChecked(), ui.checkBox_Fog->isChecked(),
                        ui.checkBox_Materials->isChecked(), ui.checkBox_ShowEffect->isChecked(),
                        ui.checkBox_show_dft->isChecked(), ui.checkBox_convex_hull_2d->isChecked(),
                        ui.checkBox_OpticalFlow->isChecked(), ambient_light(), diffuse_light(), specular_light(),
                        dft_brightness(), qcolor_to_rgb(m_dft_background_color), qcolor_to_rgb(m_dft_color), default_ns(),
                        ui.checkBox_VerticalSync->isChecked(), shadow_zoom());

                m_objects->set_show(m_show.get());

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)
                if (SHOW_VULKAN_WINDOW)
                {
                        m_show_vulkan = create_show_vulkan(widget_window_id(ui.graphics_widget_vulkan));
                }
#endif
        }
        catch (std::exception& e)
        {
                m_event_emitter.message_error_fatal(e.what());
                return;
        }
        catch (...)
        {
                m_event_emitter.message_error_fatal("");
                return;
        }

        std::string file_to_load;
        bool use_object_selection_dialog;

        if (parse_command_line([this](const std::string& message) { m_event_emitter.message_error(message); }, &file_to_load,
                               &use_object_selection_dialog))
        {
                thread_load_from_file(file_to_load, use_object_selection_dialog);
        }
}

void MainWindow::on_actionLoad_triggered()
{
        thread_load_from_file("", true);
}

void MainWindow::slot_object_repository()
{
        auto iter = m_action_to_object_name_map.find(sender());
        if (iter == m_action_to_object_name_map.cend())
        {
                m_event_emitter.message_error("Open object sender not found in map");
                return;
        }
        if (std::get<1>(iter->second).size() == 0)
        {
                m_event_emitter.message_error("Empty repository object name");
                return;
        }

        thread_load_from_repository(iter->second);
}

void MainWindow::on_actionExport_triggered()
{
        std::string object_name;
        ObjectId object_id;

        if (!find_object(&object_name, &object_id))
        {
                return;
        }

        thread_export(object_name, object_id);
}

void MainWindow::on_actionBoundCocone_triggered()
{
        thread_reload_bound_cocone();
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

void MainWindow::on_pushButton_ResetView_clicked()
{
        m_show->reset_view();
}

void MainWindow::slot_widget_opengl_mouse_wheel(double delta)
{
        if (m_show)
        {
                m_show->mouse_wheel(delta);
        }
}

void MainWindow::slot_widget_opengl_resize()
{
        if (m_show)
        {
                m_show->parent_resized();
        }
}

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)
void MainWindow::slot_widget_vulkan_mouse_wheel(double delta)
{
        if (m_show_vulkan)
        {
                m_show_vulkan->mouse_wheel(delta);
        }
}

void MainWindow::slot_widget_vulkan_resize()
{
        if (m_show_vulkan)
        {
                m_show_vulkan->parent_resized();
        }
}
#endif

double MainWindow::lighting_slider_value(const QSlider* slider)
{
        double value = slider->value() - slider->minimum();
        double delta = slider->maximum() - slider->minimum();
        double ratio = 2.0 * value / delta;
        return (ratio <= 1) ? ratio : interpolation(1.0, MAXIMUM_COLOR_AMPLIFICATION, ratio - 1);
}

double MainWindow::ambient_light() const
{
        return lighting_slider_value(ui.slider_Ambient);
}
double MainWindow::diffuse_light() const
{
        return lighting_slider_value(ui.slider_Diffuse);
}

double MainWindow::specular_light() const
{
        return lighting_slider_value(ui.slider_Specular);
}

double MainWindow::default_ns() const
{
        return ui.slider_Default_Ns->value();
}

void MainWindow::on_pushButton_ResetLighting_clicked()
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

        set_slider_to_middle(ui.slider_Ambient);
        set_slider_to_middle(ui.slider_Diffuse);
        set_slider_to_middle(ui.slider_Specular);
        set_slider_to_middle(ui.slider_Default_Ns);
}

double MainWindow::dft_brightness() const
{
        double value = ui.slider_DFT_Brightness->value() - ui.slider_DFT_Brightness->minimum();
        double delta = ui.slider_DFT_Brightness->maximum() - ui.slider_DFT_Brightness->minimum();
        double value_gamma = std::pow(value / delta, DFT_GAMMA);
        return std::pow(DFT_MAX_BRIGHTNESS, value_gamma);
}

double MainWindow::shadow_zoom() const
{
        return ui.slider_ShadowQuality->value();
}

void MainWindow::on_slider_Ambient_valueChanged(int)
{
        m_show->set_ambient(ambient_light());
}

void MainWindow::on_slider_Diffuse_valueChanged(int)
{
        m_show->set_diffuse(diffuse_light());
}

void MainWindow::on_slider_Specular_valueChanged(int)
{
        m_show->set_specular(specular_light());
}

void MainWindow::on_slider_DFT_Brightness_valueChanged(int)
{
        m_show->set_dft_brightness(dft_brightness());
}

void MainWindow::on_slider_Default_Ns_valueChanged(int)
{
        m_show->set_default_ns(default_ns());
}

void MainWindow::on_slider_ShadowQuality_valueChanged(int)
{
        if (m_show)
        {
                m_show->set_shadow_zoom(shadow_zoom());
        }
}

void MainWindow::on_toolButton_BackgroundColor_clicked()
{
        dialog::color_dialog(this, "Background Color", m_background_color, [this](const QColor& c) { set_background_color(c); });
}

void MainWindow::on_toolButton_DefaultColor_clicked()
{
        dialog::color_dialog(this, "Default Color", m_default_color, [this](const QColor& c) { set_default_color(c); });
}

void MainWindow::on_toolButton_WireframeColor_clicked()
{
        dialog::color_dialog(this, "Wireframe Color", m_wireframe_color, [this](const QColor& c) { set_wireframe_color(c); });
}

void MainWindow::on_toolButton_dft_background_color_clicked()
{
        dialog::color_dialog(this, "DFT Background Color", m_dft_background_color,
                             [this](const QColor& c) { set_dft_background_color(c); });
}

void MainWindow::on_toolButton_dft_color_clicked()
{
        dialog::color_dialog(this, "DFT Color", m_dft_color, [this](const QColor& c) { set_dft_color(c); });
}

void MainWindow::on_checkBox_Shadow_clicked()
{
        bool checked = ui.checkBox_Shadow->isChecked();

        ui.label_ShadowQuality->setEnabled(checked);
        ui.slider_ShadowQuality->setEnabled(checked);

        m_show->show_shadow(checked);
}

void MainWindow::on_checkBox_Fog_clicked()
{
        m_show->show_fog(ui.checkBox_Fog->isChecked());
}

void MainWindow::on_checkBox_Wireframe_clicked()
{
        m_show->show_wireframe(ui.checkBox_Wireframe->isChecked());
}

void MainWindow::on_checkBox_Materials_clicked()
{
        m_show->show_materials(ui.checkBox_Materials->isChecked());
}

void MainWindow::on_checkBox_Smooth_clicked()
{
        m_show->show_smooth(ui.checkBox_Smooth->isChecked());
}

void MainWindow::on_checkBox_ShowEffect_clicked()
{
        m_show->show_effect(ui.checkBox_ShowEffect->isChecked());
}

void MainWindow::on_checkBox_show_dft_clicked()
{
        bool checked = ui.checkBox_show_dft->isChecked();

        ui.label_DFT_Brightness->setEnabled(checked);
        ui.slider_DFT_Brightness->setEnabled(checked);

        m_show->show_dft(checked);
}

void MainWindow::on_checkBox_convex_hull_2d_clicked()
{
        m_show->show_convex_hull_2d(ui.checkBox_convex_hull_2d->isChecked());
}

void MainWindow::on_checkBox_OpticalFlow_clicked()
{
        m_show->show_optical_flow(ui.checkBox_OpticalFlow->isChecked());
}

void MainWindow::on_checkBox_VerticalSync_clicked()
{
        m_show->set_vertical_sync(ui.checkBox_VerticalSync->isChecked());
}

void MainWindow::on_actionFullScreen_triggered()
{
        m_show->toggle_fullscreen();
}

void MainWindow::on_radioButton_Model_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::Model));
}

void MainWindow::on_radioButton_ModelConvexHull_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::ModelConvexHull));
}

void MainWindow::on_radioButton_ModelMST_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::ModelMst));
}

void MainWindow::on_radioButton_Cocone_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::Cocone));
}

void MainWindow::on_radioButton_CoconeConvexHull_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::CoconeConvexHull));
}

void MainWindow::on_radioButton_BoundCocone_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::BoundCocone));
}

void MainWindow::on_radioButton_BoundCoconeConvexHull_clicked()
{
        m_show->show_object(object_id_to_int(ObjectId::BoundCoconeConvexHull));
}

void MainWindow::on_actionPainter_triggered()
{
        std::string object_name;
        ObjectId object_id;

        if (!find_object(&object_name, &object_id))
        {
                return;
        }

        if (!m_objects->mesh_exists(object_id))
        {
                m_event_emitter.message_warning("No object to paint " + object_name);
                return;
        }

        catch_all([&](std::string* message) {
                *message = "Painter";

                PaintingInformation3d info_3d;
                m_show->camera_information(&info_3d.camera_up, &info_3d.camera_direction, &info_3d.view_center,
                                           &info_3d.view_width, &info_3d.paint_width, &info_3d.paint_height);
                info_3d.object_position = m_show->object_position();
                info_3d.light_direction = m_show->light_direction();
                info_3d.object_size = m_show->object_size();
                info_3d.max_screen_size = PATH_TRACING_3D_MAX_SCREEN_SIZE;

                PaintingInformationNd info_nd;
                info_nd.default_screen_size = PATH_TRACING_DEFAULT_SCREEN_SIZE;
                info_nd.minimum_screen_size = PATH_TRACING_MINIMUM_SCREEN_SIZE;
                info_nd.maximum_screen_size = PATH_TRACING_MAXIMUM_SCREEN_SIZE;

                PaintingInformationAll info_all;
                info_all.parent_window = this;
                info_all.window_title = QMainWindow::windowTitle().toStdString();
                info_all.object_name = object_name;
                info_all.default_samples_per_dimension = PATH_TRACING_DEFAULT_SAMPLES_PER_DIMENSION;
                info_all.max_samples_per_dimension = PATH_TRACING_MAX_SAMPLES_PER_DIMENSION;
                info_all.background_color = qcolor_to_rgb(m_background_color);
                info_all.default_color = qcolor_to_rgb(m_default_color);
                info_all.diffuse = diffuse_light();

                m_objects->paint(object_id, info_3d, info_nd, info_all);
        });
}
