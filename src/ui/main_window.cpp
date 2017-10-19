/*
Copyright (C) 2017 Topological Manifold

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

#include "painter_window.h"
#include "support.h"
#include "dialogs/application_about.h"
#include "dialogs/application_help.h"
#include "dialogs/bound_cocone_parameters.h"
#include "dialogs/message_box.h"
#include "dialogs/path_tracing_parameters.h"
#include "dialogs/source_error.h"

#include "application/application_name.h"
#include "com/error.h"
#include "com/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "com/vec_glm.h"
#include "geometry/cocone/reconstruction.h"
#include "geometry/objects/points.h"
#include "obj/obj_alg.h"
#include "obj/obj_convex_hull.h"
#include "obj/obj_file_load.h"
#include "obj/obj_file_save.h"
#include "obj/obj_points_load.h"
#include "obj/obj_surface.h"
#include "path_tracing/lights/light_source.h"
#include "path_tracing/projectors/projector.h"
#include "path_tracing/scenes.h"
#include "path_tracing/visible_mesh.h"
#include "progress/progress.h"

#include <QDesktopWidget>
#include <QFileDialog>

// Размер окна по сравнению с экраном
constexpr double WINDOW_SIZE_COEF = 0.7;
// Если true, то размер для графики, если false, то размер всего окна
constexpr bool WINDOW_SIZE_GRAPHICS = true;

constexpr double DFT_MAX_BRIGHTNESS = 50000;
constexpr double DFT_GAMMA = 0.5;

constexpr double BOUND_COCONE_DEFAULT_RHO = 0.3;
constexpr double BOUND_COCONE_DEFAULT_ALPHA = 0.14;
constexpr int BOUND_COCONE_DISPLAY_DIGITS = 3;

// Таймер отображения хода расчётов. Величина в миллисекундах.
constexpr int TIMER_PROGRESS_BAR_INTERVAL = 100;

// Количество точек для готовых объектов.
constexpr int POINT_COUNT = 10000;

// Цвета по умолчанию
constexpr QRgb CLEAR_COLOR = qRgb(50, 100, 150);
constexpr QRgb DEFAULT_COLOR = qRgb(150, 170, 150);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

// увеличение текстуры тени по сравнению с размером окна
constexpr float SHADOW_ZOOM = 2;

// Разделение пикселя по горизонтали и вертикали для трассировки пути.
// Количество лучей на один пиксель в одном проходе равно этому числу в квадрате.
constexpr int PROJECTOR_PIXEL_RESOLUTION = 5;

// Сколько потоков не надо использовать от максимума для создания октадеревьев.
constexpr unsigned MESH_OBJECT_NOT_USED_THREAD_COUNT = 2;

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          m_window_thread_id(std::this_thread::get_id()),
          m_object_repository(create_object_repository<3>()),
          m_mesh_object_threads(std::max(1u, get_hardware_concurrency() - MESH_OBJECT_NOT_USED_THREAD_COUNT))
{
        static_assert(std::is_same_v<decltype(ui.graphics_widget), GraphicsWidget*>);

        ui.setupUi(this);

        // После вызова ui.setupUi(this)
        set_log_callback(&m_event_emitter);

        QMainWindow::setWindowTitle(APPLICATION_NAME);

        QMainWindow::addAction(ui.actionFullScreen);

        qRegisterMetaType<WindowEvent>("WindowEvent");
        connect(&m_event_emitter, SIGNAL(window_event(WindowEvent)), this, SLOT(slot_window_event(WindowEvent)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        ui.graphics_widget->setText("");
        connect(ui.graphics_widget, SIGNAL(wheel(double)), this, SLOT(slot_widget_under_window_mouse_wheel(double)));
        connect(ui.graphics_widget, SIGNAL(resize()), this, SLOT(slot_widget_under_window_resize()));

        connect(&m_timer_progress_bar, SIGNAL(timeout()), this, SLOT(slot_timer_progress_bar()));

        set_widgets_enabled(this->layout(), true);
        set_dependent_interface();
        strike_out_all_objects_buttons();

        set_bound_cocone_parameters(BOUND_COCONE_DEFAULT_RHO, BOUND_COCONE_DEFAULT_ALPHA);

        set_clear_color(CLEAR_COLOR);
        set_default_color(DEFAULT_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);

        ui.mainWidget->layout()->setContentsMargins(3, 3, 3, 3);
        ui.mainWidget->layout()->setSpacing(3);

        ui.radioButton_Model->setChecked(true);

        ui.tabWidget->setCurrentIndex(0);

        ui.actionHelp->setText(QString(APPLICATION_NAME) + " Help");
        ui.actionAbout->setText("About " + QString(APPLICATION_NAME));

        ui.Slider_ShadowQuality->setSliderPosition(SHADOW_ZOOM);

        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);
        for (const std::string& object_name : m_object_repository->get_list_of_point_objects())
        {
                QAction* action = ui.menuCreate->addAction(object_name.c_str());
                m_action_to_object_name_map.emplace(action, object_name);
                connect(action, SIGNAL(triggered()), this, SLOT(slot_object_repository()));
        }

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.statusBar->setFixedHeight(ui.statusBar->height());

        m_threads.try_emplace(ThreadAction::OpenObject);
        m_threads.try_emplace(ThreadAction::ExportCocone);
        m_threads.try_emplace(ThreadAction::ExportBoundCocone);
        m_threads.try_emplace(ThreadAction::ReloadBoundCocone);
        m_threads.try_emplace(ThreadAction::SelfTest);

        m_meshes.try_emplace(MeshType::Model);
        m_meshes.try_emplace(MeshType::ModelCH);
        m_meshes.try_emplace(MeshType::Cocone);
        m_meshes.try_emplace(MeshType::CoconeCH);
        m_meshes.try_emplace(MeshType::BoundCocone);
        m_meshes.try_emplace(MeshType::BoundCoconeCH);
}

MainWindow::~MainWindow()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        for (auto& t : m_threads)
        {
                t.second.stop();
        }

        set_log_callback(nullptr);
}

void MainWindow::ThreadPack::stop() noexcept
{
        try
        {
                progress_ratio_list.stop_all();
                if (thread.joinable())
                {
                        thread.join();
                }
                progress_ratio_list.enable();
        }
        catch (...)
        {
                error_fatal("thread stop error");
        }
}

bool MainWindow::thread_action_allowed(ThreadAction action) const
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        ASSERT(m_threads.count(ThreadAction::OpenObject) && m_threads.count(ThreadAction::ExportCocone) &&
               m_threads.count(ThreadAction::ExportBoundCocone) && m_threads.count(ThreadAction::ReloadBoundCocone) &&
               m_threads.count(ThreadAction::SelfTest));

        bool allowed = true;

        switch (action)
        {
        case ThreadAction::OpenObject:
                // ThreadAction::OpenObject
                allowed = allowed && !m_threads.find(ThreadAction::ExportCocone)->second.working;
                allowed = allowed && !m_threads.find(ThreadAction::ExportBoundCocone)->second.working;
                // ThreadAction::ReloadBoundCocone
                // ThreadAction::SelfTest
                return allowed;
        case ThreadAction::ExportCocone:
                allowed = allowed && !m_threads.find(ThreadAction::OpenObject)->second.working;
                allowed = allowed && !m_threads.find(ThreadAction::ExportCocone)->second.working;
                // ThreadAction::ExportBoundCocone
                // ThreadAction::ReloadBoundCocone
                // ThreadAction::SelfTest
                return allowed;
        case ThreadAction::ExportBoundCocone:
                allowed = allowed && !m_threads.find(ThreadAction::OpenObject)->second.working;
                // ThreadAction::ExportCocone
                allowed = allowed && !m_threads.find(ThreadAction::ExportBoundCocone)->second.working;
                allowed = allowed && !m_threads.find(ThreadAction::ReloadBoundCocone)->second.working;
                // ThreadAction::SelfTest
                return allowed;
        case ThreadAction::ReloadBoundCocone:
                allowed = allowed && !m_threads.find(ThreadAction::OpenObject)->second.working;
                // ThreadAction::ExportCocone
                allowed = allowed && !m_threads.find(ThreadAction::ExportBoundCocone)->second.working;
                // ThreadAction::ReloadBoundCocone
                // ThreadAction::SelfTest
                return allowed;
        case ThreadAction::SelfTest:
                return true;
        }

        error_fatal("Unknown thread action");
}

template <MainWindow::ThreadAction thread_action, typename... Args>
void MainWindow::start_thread(const Args&... args)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);
        ASSERT(thread_action_allowed(thread_action));

        switch (thread_action)
        {
        case ThreadAction::OpenObject:
                m_threads.find(ThreadAction::ReloadBoundCocone)->second.stop();
                break;
        case ThreadAction::ExportCocone:
                break;
        case ThreadAction::ExportBoundCocone:
                break;
        case ThreadAction::ReloadBoundCocone:
                break;
        case ThreadAction::SelfTest:
                break;
        }

        ThreadPack& thread = m_threads.find(thread_action)->second;

        thread.stop();

        thread.working = true;

        thread.thread = std::thread([ =, &thread ]() noexcept {

                if constexpr (thread_action == ThreadAction::OpenObject)
                {
                        thread_open_object(&thread.progress_ratio_list, args...);
                }
                else if constexpr (thread_action == ThreadAction::ExportCocone ||
                                   thread_action == ThreadAction::ExportBoundCocone)
                {
                        thread_export(&thread.progress_ratio_list, args...);
                }
                else if constexpr (thread_action == ThreadAction::ReloadBoundCocone)
                {
                        thread_bound_cocone(&thread.progress_ratio_list, args...);
                }
                else if constexpr (thread_action == ThreadAction::SelfTest)
                {
                        thread_self_test(&thread.progress_ratio_list, args...);
                }
                else
                {
                        error_fatal("Unknown thread action in start thread");
                }

                thread.working = false;
        });
}

template <typename T>
void MainWindow::catch_all(const T& a) noexcept
{
        try
        {
                std::string error_message = "Error";
                try
                {
                        a(&error_message);
                }
                catch (TerminateRequestException&)
                {
                }
                catch (ErrorSourceException& e)
                {
                        m_event_emitter.message_error_source(error_message + ":\n" + e.get_msg(), e.get_src());
                }
                catch (std::exception& e)
                {
                        m_event_emitter.message_error(error_message + ":\n" + e.what());
                }
                catch (...)
                {
                        m_event_emitter.message_error(error_message + ":\n" + "Unknown error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in catch all.");
        }
}

void MainWindow::thread_self_test(ProgressRatioList* progress_ratio_list, SelfTestType test_type) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        catch_all([&](std::string* message) {

                *message = "Self-Test";

                self_test(test_type, progress_ratio_list, message);

        });
}

std::tuple<std::string, MainWindow::ObjectType, MainWindow::ObjectType> MainWindow::parameters_for_add_object(
        AddObjectType add_object_type)
{
        switch (add_object_type)
        {
        case AddObjectType::Model:
                return std::make_tuple("Convex hull 3D", MODEL, MODEL_CONVEX_HULL);
        case AddObjectType::Cocone:
                return std::make_tuple("COCONE convex hull 3D", SURFACE_COCONE, SURFACE_COCONE_CONVEX_HULL);
        case AddObjectType::BoundCocone:
                return std::make_tuple("BOUND COCONE convex hull 3D", SURFACE_BOUND_COCONE, SURFACE_BOUND_COCONE_CONVEX_HULL);
        }
        error_fatal("Unknown object type for add object");
}

std::tuple<std::string, MainWindow::MeshType> MainWindow::parameters_for_mesh_object(AddObjectType add_object_type)
{
        switch (add_object_type)
        {
        case AddObjectType::Model:
                return std::make_tuple("Mesh object", MeshType::Model);
        case AddObjectType::Cocone:
                return std::make_tuple("COCONE mesh object", MeshType::Cocone);
        case AddObjectType::BoundCocone:
                return std::make_tuple("BOUND COCONE mesh object", MeshType::BoundCocone);
        }
        error_fatal("Unknown object type for mesh object");
}

void MainWindow::thread_add_object(ProgressRatioList* progress_ratio_list, AddObjectType add_object_type,
                                   std::shared_ptr<IObj> obj) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        std::thread t1([&]() noexcept {
                catch_all([&](std::string* message) {

                        ObjectType object_type, object_type_convex_hull;

                        std::tie(*message, object_type, object_type_convex_hull) = parameters_for_add_object(add_object_type);

                        if (obj->get_faces().size() > 0 ||
                            (add_object_type == AddObjectType::Model && obj->get_points().size() > 0))
                        {
                                m_show->add_object(obj, object_type, MODEL);

                                ProgressRatio progress(progress_ratio_list);
                                progress.set_text(*message + ": %v of %m");

                                std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(obj.get(), &progress);

                                if (convex_hull->get_faces().size() != 0)
                                {
                                        m_show->add_object(convex_hull, object_type_convex_hull, MODEL);
                                }
                        }
                });
        });

        std::thread t2([&]() noexcept {
                catch_all([&](std::string* message) {

                        MeshType mesh_type;

                        std::tie(*message, mesh_type) = parameters_for_mesh_object(add_object_type);

                        if (obj->get_faces().size() > 0)
                        {
                                ProgressRatio progress(progress_ratio_list);
                                m_meshes[mesh_type] = std::make_unique<VisibleMesh>(
                                        obj.get(), m_mesh_object_size, m_mesh_object_position, m_mesh_object_threads, &progress);
                        }

                });
        });

        t1.join();
        t2.join();
}

void MainWindow::thread_cocone(ProgressRatioList* progress_ratio_list) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        catch_all([&](std::string* message) {

                *message = "COCONE reconstruction";

                {
                        ProgressRatio progress(progress_ratio_list);

                        double start_time = get_time_seconds();

                        std::vector<vec<3>> normals;
                        std::vector<std::array<int, 3>> facets;

                        m_surface_constructor->cocone(&normals, &facets, &progress);

                        m_surface_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                        LOG("Surface reconstruction second phase, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
                }

                thread_add_object(progress_ratio_list, AddObjectType::Cocone, m_surface_cocone);

        });
}

void MainWindow::thread_bound_cocone(ProgressRatioList* progress_ratio_list, double rho, double alpha) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        catch_all([&](std::string* message) {

                *message = "BOUND COCONE reconstruction";

                {
                        ProgressRatio progress(progress_ratio_list);

                        double start_time = get_time_seconds();

                        std::vector<vec<3>> normals;
                        std::vector<std::array<int, 3>> facets;

                        m_surface_constructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                        m_surface_bound_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                        LOG("Surface reconstruction second phase, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
                }

                m_show->delete_object(SURFACE_BOUND_COCONE);
                m_show->delete_object(SURFACE_BOUND_COCONE_CONVEX_HULL);

                m_meshes[MeshType::BoundCocone].reset();
                m_meshes[MeshType::BoundCoconeCH].reset();

                m_event_emitter.bound_cocone_loaded(rho, alpha);

                thread_add_object(progress_ratio_list, AddObjectType::BoundCocone, m_surface_bound_cocone);

        });
}

void MainWindow::thread_surface_constructor(ProgressRatioList* progress_ratio_list) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        catch_all([&](std::string* message) {

                *message = "Surface constructor";

                {
                        ProgressRatio progress(progress_ratio_list);

                        double start_time = get_time_seconds();

                        m_surface_constructor = create_manifold_constructor(to_vector<float>(m_surface_points), &progress);

                        LOG("Surface reconstruction first phase, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
                }

                std::thread cocone([=]() noexcept { thread_cocone(progress_ratio_list); });
                std::thread bound_cocone([=]() noexcept {
                        thread_bound_cocone(progress_ratio_list, m_bound_cocone_rho, m_bound_cocone_alpha);
                });

                cocone.join();
                bound_cocone.join();

        });
}

void MainWindow::thread_open_object(ProgressRatioList* progress_ratio_list, const std::string& object_name,
                                    OpenObjectType object_type) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        catch_all([&](std::string* message) {

                *message = "Load " + object_name;

                std::shared_ptr<IObj> obj;

                {
                        ProgressRatio progress(progress_ratio_list);
                        switch (object_type)
                        {
                        case OpenObjectType::File:
                                progress.set_text("Load file: %p%");
                                obj = load_obj_from_file(object_name, &progress);
                                break;
                        case OpenObjectType::Repository:
                                progress.set_text("Load object: %p%");
                                obj = load_obj_from_points(
                                        to_glm(m_object_repository->get_point_object(object_name, POINT_COUNT)));
                                break;
                        }
                }

                if (obj->get_faces().size() == 0 && obj->get_points().size() == 0)
                {
                        error("Faces or points not found");
                }
                if (obj->get_faces().size() != 0 && obj->get_points().size() != 0)
                {
                        error("Faces and points together in one object are not supported");
                }

                m_show->delete_all_objects();

                m_surface_constructor.reset();
                m_surface_cocone.reset();
                m_surface_bound_cocone.reset();

                for (auto& mesh : m_meshes)
                {
                        mesh.second.reset();
                }

                m_event_emitter.file_loaded(object_name);

                m_surface_points = (obj->get_faces().size() > 0) ? get_unique_face_vertices(obj.get()) :
                                                                   get_unique_point_vertices(obj.get());

                std::thread model([=]() noexcept { thread_add_object(progress_ratio_list, AddObjectType::Model, obj); });
                std::thread surface([=]() noexcept { thread_surface_constructor(progress_ratio_list); });

                model.join();
                surface.join();

        });
}

void MainWindow::thread_export(ProgressRatioList* /*progress_ratio_list*/, const IObj* obj, const std::string& file_name,
                               const std::string& cocone_type) noexcept
{
        ASSERT(std::this_thread::get_id() != m_window_thread_id);

        catch_all([&](std::string* message) {

                *message = "Export " + cocone_type + " to " + file_name;

                save_obj_geometry_to_file(obj, file_name, cocone_type);

                m_event_emitter.message_information(cocone_type + " exported to file\n" + file_name);

        });
}

void MainWindow::progress_bars(bool permanent, ProgressRatioList* progress_ratio_list, std::list<QProgressBar>* progress_bars)
{
        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios = progress_ratio_list->get_all();

        if (ratios.size() > progress_bars->size())
        {
                progress_bars->resize(ratios.size());
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

                unsigned v = std::get<0>(ratios[i]), m = std::get<1>(ratios[i]);

                if (m > 0)
                {
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
        for (auto& t : m_threads)
        {
                progress_bars(t.first == ThreadAction::SelfTest, &t.second.progress_ratio_list, &t.second.progress_bars);
        }
}

void MainWindow::set_bound_cocone_parameters(double rho, double alpha)
{
        m_bound_cocone_rho = rho;
        m_bound_cocone_alpha = alpha;

        QString label;
        label += u8"ρ " + QString(to_string_fixed(rho, BOUND_COCONE_DISPLAY_DIGITS).c_str());
        label += "; ";
        label += u8"α " + QString(to_string_fixed(alpha, BOUND_COCONE_DISPLAY_DIGITS).c_str());
        ui.BoundCocone_label->setText(label);
}

void MainWindow::set_clear_color(const QColor& c)
{
        m_clear_color = c;
        if (m_show)
        {
                m_show->set_clear_color(qcolor_to_vec3(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_clear_color);
        ui.widget_clear_color->setPalette(palette);
}

void MainWindow::set_default_color(const QColor& c)
{
        m_default_color = c;
        if (m_show)
        {
                m_show->set_default_color(qcolor_to_vec3(c));
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
                m_show->set_wireframe_color(qcolor_to_vec3(c));
        }
        QPalette palette;
        palette.setColor(QPalette::Window, m_wireframe_color);
        ui.widget_wireframe_color->setPalette(palette);
}

void MainWindow::set_dependent_interface()
{
        ui.Label_DFT_Brightness->setEnabled(ui.checkBox_show_dft->isEnabled() && ui.checkBox_show_dft->isChecked());
        ui.Slider_DFT_Brightness->setEnabled(ui.checkBox_show_dft->isEnabled() && ui.checkBox_show_dft->isChecked());
}

void MainWindow::strike_out_radio_button(QRadioButton* button)
{
        button_strike_out(button, true);
}

void MainWindow::enable_radio_button(QRadioButton* button)
{
        button_strike_out(button, false);

        if (button->isChecked())
        {
                button->click();
        }
}

void MainWindow::strike_out_all_objects_buttons()
{
        strike_out_radio_button(ui.radioButton_Model);
        strike_out_radio_button(ui.radioButton_ModelConvexHull);
        strike_out_radio_button(ui.radioButton_Cocone);
        strike_out_radio_button(ui.radioButton_CoconeConvexHull);
        strike_out_radio_button(ui.radioButton_BoundCocone);
        strike_out_radio_button(ui.radioButton_BoundCoconeConvexHull);
}

void MainWindow::strike_out_bound_cocone_buttons()
{
        strike_out_radio_button(ui.radioButton_BoundCocone);
        strike_out_radio_button(ui.radioButton_BoundCoconeConvexHull);
}

void MainWindow::slot_window_event(const WindowEvent& event)
{
        switch (event.get_type())
        {
        case WindowEvent::EventType::MESSAGE_ERROR:
        {
                const WindowEvent::message_error& d = event.get<WindowEvent::message_error>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Error);
                message_critical(this, message.c_str());

                break;
        }
        case WindowEvent::EventType::MESSAGE_ERROR_FATAL:
        {
                const WindowEvent::message_error_fatal& d = event.get<WindowEvent::message_error_fatal>();
                std::string message = (d.msg.size() != 0) ? d.msg : "Unknown Error. Exit failure.";

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Error);
                message_critical(this, message.c_str());

                close();

                break;
        }
        case WindowEvent::EventType::MESSAGE_ERROR_SOURCE:
        {
                const WindowEvent::message_error_source& d = event.get<WindowEvent::message_error_source>();
                std::string message = d.msg;
                std::string source = source_with_line_numbers(d.src);

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message + "\n" + source),
                                               TextEditMessageType::Error);
                SourceError(this).show(message.c_str(), source.c_str());

                close();

                break;
        }
        case WindowEvent::EventType::MESSAGE_INFORMATION:
        {
                const WindowEvent::message_information& d = event.get<WindowEvent::message_information>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Information);
                message_information(this, message.c_str());

                break;
        }
        case WindowEvent::EventType::MESSAGE_WARNING:
        {
                const WindowEvent::message_warning& d = event.get<WindowEvent::message_warning>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Warning);
                message_warning(this, message.c_str());

                break;
        }
        case WindowEvent::EventType::LOG:
        {
                // Здесь без вызовов функции LOG, так как начнёт вызывать сама себя

                const WindowEvent::log& d = event.get<WindowEvent::log>();
                std::string message = d.msg;

                add_to_text_edit_and_to_stderr(ui.text_log, format_log_message(message), TextEditMessageType::Normal);

                break;
        }
        case WindowEvent::EventType::OBJECT_LOADED:
        {
                const WindowEvent::object_loaded& d = event.get<WindowEvent::object_loaded>();

                switch (static_cast<ObjectType>(d.id))
                {
                case MODEL:
                        enable_radio_button(ui.radioButton_Model);
                        break;
                case MODEL_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_ModelConvexHull);
                        break;
                case SURFACE_COCONE:
                        enable_radio_button(ui.radioButton_Cocone);
                        break;
                case SURFACE_COCONE_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_CoconeConvexHull);
                        break;
                case SURFACE_BOUND_COCONE:
                        enable_radio_button(ui.radioButton_BoundCocone);
                        break;
                case SURFACE_BOUND_COCONE_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_BoundCoconeConvexHull);
                        break;
                }

                break;
        }
        case WindowEvent::EventType::FILE_LOADED:
        {
                const WindowEvent::file_loaded& d = event.get<WindowEvent::file_loaded>();

                std::string file_name = get_base_name(d.file_name);
                this->setWindowTitle(QString(APPLICATION_NAME) + " - " + file_name.c_str());
                strike_out_all_objects_buttons();
                ui.radioButton_Model->setChecked(true);

                break;
        }
        case WindowEvent::EventType::BOUND_COCONE_LOADED:
        {
                const WindowEvent::bound_cocone_loaded& d = event.get<WindowEvent::bound_cocone_loaded>();

                set_bound_cocone_parameters(d.rho, d.alpha);
                strike_out_bound_cocone_buttons();

                break;
        }
        }
}

void MainWindow::showEvent(QShowEvent* e)
{
        QMainWindow::showEvent(e);

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

        start_thread<ThreadAction::SelfTest>(SelfTestType::Required);

        try
        {
                m_show = create_show(&m_event_emitter, get_widget_window_id(ui.graphics_widget), qcolor_to_vec3(m_clear_color),
                                     qcolor_to_vec3(m_default_color), qcolor_to_vec3(m_wireframe_color),
                                     ui.checkBox_Smooth->isChecked(), ui.checkBox_Wireframe->isChecked(),
                                     ui.checkBox_Shadow->isChecked(), ui.checkBox_Materials->isChecked(),
                                     ui.checkBox_ShowEffect->isChecked(), ui.checkBox_show_dft->isChecked(),
                                     ui.checkBox_convex_hull_2d->isChecked(), ui.checkBox_OpticalFlow->isChecked(), get_ambient(),
                                     get_diffuse(), get_specular(), get_dft_brightness(), get_default_ns(),
                                     ui.checkBox_VerticalSync->isChecked(), get_shadow_zoom());

                float size;
                glm::vec3 position;
                m_show->get_object_size_and_position(&size, &position);
                m_mesh_object_size = size;
                m_mesh_object_position = to_vector<double>(position);
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

        if (QCoreApplication::arguments().count() == 2)
        {
                if (!thread_action_allowed(ThreadAction::OpenObject))
                {
                        m_event_emitter.message_warning("File opening is not available at this time (thread working)");
                        return;
                }

                start_thread<ThreadAction::OpenObject>(QCoreApplication::arguments().at(1).toStdString(), OpenObjectType::File);
        }
}

void MainWindow::on_actionLoad_triggered()
{
        if (!thread_action_allowed(ThreadAction::OpenObject))
        {
                m_event_emitter.message_warning("File opening is not available at this time (thread working)");
                return;
        }

        QString file_name = QFileDialog::getOpenFileName(this, "Open", "", "OBJ and Point files (*.obj *.txt)", nullptr,
                                                         QFileDialog::ReadOnly | QFileDialog::DontUseNativeDialog);
        if (file_name.size() > 0)
        {
                start_thread<ThreadAction::OpenObject>(file_name.toStdString(), OpenObjectType::File);
        }
}

void MainWindow::slot_object_repository()
{
        if (!thread_action_allowed(ThreadAction::OpenObject))
        {
                m_event_emitter.message_warning("Creation of object is not available at this time (thread working)");
                return;
        }

        auto iter = m_action_to_object_name_map.find(sender());
        if (iter != m_action_to_object_name_map.cend())
        {
                start_thread<ThreadAction::OpenObject>(iter->second, OpenObjectType::Repository);
        }
        else
        {
                m_event_emitter.message_error("open object sender not found in map");
        }
}

template <MainWindow::ThreadAction thread_action>
void MainWindow::export_to_file()
{
        static_assert(thread_action == ThreadAction::ExportCocone || thread_action == ThreadAction::ExportBoundCocone);

        QString cocone_type;
        if (thread_action == ThreadAction::ExportCocone)
        {
                cocone_type = "COCONE";
        }
        else if (thread_action == ThreadAction::ExportBoundCocone)
        {
                cocone_type = "BOUND COCONE";
        }

        if (!thread_action_allowed(thread_action))
        {
                m_event_emitter.message_warning("Export " + cocone_type.toStdString() +
                                                " to file is not available at this time (thread working)");
                return;
        }

        const IObj* obj = nullptr;

        if (thread_action == ThreadAction::ExportCocone)
        {
                if (!m_surface_cocone || m_surface_cocone->get_faces().size() == 0)
                {
                        m_event_emitter.message_warning("COCONE not created");
                        return;
                }
                obj = m_surface_cocone.get();
        }
        else if (thread_action == ThreadAction::ExportBoundCocone)
        {
                if (!m_surface_bound_cocone || m_surface_bound_cocone->get_faces().size() == 0)
                {
                        m_event_emitter.message_warning("BOUND COCONE not created");
                        return;
                }
                obj = m_surface_bound_cocone.get();
        }

        ASSERT(obj);

        QString file_name = QFileDialog::getSaveFileName(this, "Export " + cocone_type + " to OBJ", "", "OBJ files (*.obj)",
                                                         nullptr, QFileDialog::DontUseNativeDialog);
        if (file_name.size() == 0)
        {
                return;
        }

        start_thread<thread_action>(obj, file_name.toStdString(), cocone_type.toStdString());
}

void MainWindow::on_actionExport_triggered()
{
        bool cocone = ui.radioButton_Cocone->isChecked();
        bool bound_cocone = ui.radioButton_BoundCocone->isChecked();

        if (int cnt = ((cocone ? 1 : 0) + (bound_cocone ? 1 : 0)); cnt > 1)
        {
                m_event_emitter.message_error("COCONE and BOUND COCONE select error");
                return;
        }
        else if (cnt < 1)
        {
                m_event_emitter.message_warning("Select COCONE or BOUND COCONE");
                return;
        }

        if (cocone)
        {
                export_to_file<ThreadAction::ExportCocone>();
        }
        else if (bound_cocone)
        {
                export_to_file<ThreadAction::ExportBoundCocone>();
        }
}

void MainWindow::on_actionBoundCocone_triggered()
{
        if (!thread_action_allowed(ThreadAction::ReloadBoundCocone))
        {
                m_event_emitter.message_warning("BOUND COCONE is not available at this time (thread working)");
                return;
        }

        if (!m_surface_constructor)
        {
                m_event_emitter.message_warning("No surface constructor");
                return;
        }

        double rho = m_bound_cocone_rho;
        double alpha = m_bound_cocone_alpha;

        if (!BoundCoconeParameters(this).show(BOUND_COCONE_DISPLAY_DIGITS, &rho, &alpha))
        {
                return;
        }

        start_thread<ThreadAction::ReloadBoundCocone>(rho, alpha);
}

void MainWindow::on_actionExit_triggered()
{
        close();
}

void MainWindow::on_actionHelp_triggered()
{
        application_help(this);
}

void MainWindow::on_actionSelfTest_triggered()
{
        if (!thread_action_allowed(ThreadAction::SelfTest))
        {
                m_event_emitter.message_warning("Self-Test is not available at this time (thread working)");
                return;
        }

        start_thread<ThreadAction::SelfTest>(SelfTestType::Extended);
}

void MainWindow::on_actionAbout_triggered()
{
        application_about(this);
}

void MainWindow::on_Button_ResetView_clicked()
{
        m_show->reset_view();
}

void MainWindow::slot_widget_under_window_mouse_wheel(double delta)
{
        if (m_show)
        {
                m_show->mouse_wheel(delta);
        }
}

void MainWindow::slot_widget_under_window_resize()
{
        if (m_show)
        {
                m_show->parent_resized();
        }
}

double MainWindow::get_ambient() const
{
        double value = ui.Slider_Ambient->value() - ui.Slider_Ambient->minimum();
        double delta = ui.Slider_Ambient->maximum() - ui.Slider_Ambient->minimum();
        return 2 * value / delta;
}
double MainWindow::get_diffuse() const
{
        double value = ui.Slider_Diffuse->value() - ui.Slider_Diffuse->minimum();
        double delta = ui.Slider_Diffuse->maximum() - ui.Slider_Diffuse->minimum();
        return 2 * value / delta;
}
double MainWindow::get_specular() const
{
        double value = ui.Slider_Specular->value() - ui.Slider_Specular->minimum();
        double delta = ui.Slider_Specular->maximum() - ui.Slider_Specular->minimum();
        return 2 * value / delta;
}
double MainWindow::get_dft_brightness() const
{
        double value = ui.Slider_DFT_Brightness->value() - ui.Slider_DFT_Brightness->minimum();
        double delta = ui.Slider_DFT_Brightness->maximum() - ui.Slider_DFT_Brightness->minimum();
        double value_gamma = std::pow(value / delta, DFT_GAMMA);
        return std::pow(DFT_MAX_BRIGHTNESS, value_gamma);
}
double MainWindow::get_default_ns() const
{
        return ui.Slider_Default_Ns->value();
}
double MainWindow::get_shadow_zoom() const
{
        return ui.Slider_ShadowQuality->value();
}

void MainWindow::on_Slider_Ambient_valueChanged(int)
{
        m_show->set_ambient(get_ambient());
}

void MainWindow::on_Slider_Diffuse_valueChanged(int)
{
        m_show->set_diffuse(get_diffuse());
}

void MainWindow::on_Slider_Specular_valueChanged(int)
{
        m_show->set_specular(get_specular());
}

void MainWindow::on_Slider_DFT_Brightness_valueChanged(int)
{
        m_show->set_dft_brightness(get_dft_brightness());
}

void MainWindow::on_Slider_Default_Ns_valueChanged(int)
{
        m_show->set_default_ns(get_default_ns());
}

void MainWindow::on_Slider_ShadowQuality_valueChanged(int)
{
        if (m_show)
        {
                m_show->set_shadow_zoom(get_shadow_zoom());
        }
}

void MainWindow::on_ButtonBackgroundColor_clicked()
{
        color_dialog(this, "Background color", m_clear_color, [this](const QColor& c) { set_clear_color(c); });
}

void MainWindow::on_ButtonDefaultColor_clicked()
{
        color_dialog(this, "Default color", m_default_color, [this](const QColor& c) { set_default_color(c); });
}

void MainWindow::on_ButtonWireframeColor_clicked()
{
        color_dialog(this, "Wireframe color", m_wireframe_color, [this](const QColor& c) { set_wireframe_color(c); });
}

void MainWindow::on_checkBox_Shadow_clicked()
{
        m_show->show_shadow(ui.checkBox_Shadow->isChecked());
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
        ui.Label_DFT_Brightness->setEnabled(ui.checkBox_show_dft->isChecked());
        ui.Slider_DFT_Brightness->setEnabled(ui.checkBox_show_dft->isChecked());

        m_show->show_dft(ui.checkBox_show_dft->isChecked());
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
        m_show->show_object(MODEL);
}

void MainWindow::on_radioButton_ModelConvexHull_clicked()
{
        m_show->show_object(MODEL_CONVEX_HULL);
}

void MainWindow::on_radioButton_Cocone_clicked()
{
        m_show->show_object(SURFACE_COCONE);
}

void MainWindow::on_radioButton_CoconeConvexHull_clicked()
{
        m_show->show_object(SURFACE_COCONE_CONVEX_HULL);
}

void MainWindow::on_radioButton_BoundCocone_clicked()
{
        m_show->show_object(SURFACE_BOUND_COCONE);
}

void MainWindow::on_radioButton_BoundCoconeConvexHull_clicked()
{
        m_show->show_object(SURFACE_BOUND_COCONE_CONVEX_HULL);
}

bool MainWindow::find_visible_mesh(std::shared_ptr<const VisibleMesh>* mesh_pointer, std::string* model_name) const
{
        if (ui.radioButton_Model->isChecked())
        {
                *model_name = ui.radioButton_Model->text().toStdString();
                auto i = m_meshes.find(MeshType::Model);
                ASSERT(i != m_meshes.cend());
                *mesh_pointer = i->second;
                return true;
        }

        if (ui.radioButton_Cocone->isChecked())
        {
                *model_name = ui.radioButton_Cocone->text().toStdString();
                auto i = m_meshes.find(MeshType::Cocone);
                ASSERT(i != m_meshes.cend());
                *mesh_pointer = i->second;
                return true;
        }

        if (ui.radioButton_BoundCocone->isChecked())
        {
                *model_name = ui.radioButton_BoundCocone->text().toStdString();
                auto i = m_meshes.find(MeshType::BoundCocone);
                ASSERT(i != m_meshes.cend());
                *mesh_pointer = i->second;
                return true;
        }

        return false;
}

std::unique_ptr<const Projector> MainWindow::create_projector(int paint_width, int paint_height) const
{
        glm::vec3 camera_up, camera_direction, view_center;
        float view_width;
        m_show->get_camera_information(&camera_up, &camera_direction, &view_center, &view_width);

        vec3 camera_position = to_vector<double>(view_center) - to_vector<double>(camera_direction) * 2.0 * m_mesh_object_size;

        return std::make_unique<const ParallelProjector>(camera_position, to_vector<double>(camera_direction),
                                                         to_vector<double>(camera_up), view_width, paint_width, paint_height,
                                                         PROJECTOR_PIXEL_RESOLUTION);
}

std::unique_ptr<const LightSource> MainWindow::create_light_source() const
{
        glm::vec3 light_direction;
        m_show->get_light_information(&light_direction);

        vec3 light_position = m_mesh_object_position - to_vector<double>(light_direction) * m_mesh_object_size * 1000.0;

        return std::make_unique<const ConstantLight>(light_position, vec3(1, 1, 1));
}

void MainWindow::on_pushButton_Painter_clicked()
{
        std::shared_ptr<const VisibleMesh> mesh_pointer;
        std::string model_name;

        if (!find_visible_mesh(&mesh_pointer, &model_name))
        {
                message_warning(this, "No painting support for this model type");
                return;
        }

        if (!mesh_pointer)
        {
                message_warning(this, "No object to paint");
                return;
        }

        int thread_count;
        double size_coef;

        if (!PathTracingParameters(this).show(get_hardware_concurrency(), ui.graphics_widget->width(),
                                              ui.graphics_widget->height(), &thread_count, &size_coef))
        {
                return;
        }

        catch_all([&](std::string* message) {

                *message = "Painter";

                int paint_width = std::round(ui.graphics_widget->width() * size_coef);
                int paint_height = std::round(ui.graphics_widget->height() * size_coef);

                std::string window_name = std::string(APPLICATION_NAME) + " - " + model_name;
                vec3 default_color = to_vector<double>(qcolor_to_rgb(m_default_color));
                double diffuse = float_to_rgb(get_diffuse());

                if ((true))
                {
                        vec3 background_color = to_vector<double>(qcolor_to_rgb(m_clear_color));

                        create_painter_window(window_name, thread_count,
                                              one_object_scene(background_color, default_color, diffuse,
                                                               create_projector(paint_width, paint_height), create_light_source(),
                                                               *mesh_pointer));
                }
                else
                {
                        glm::vec3 camera_up, camera_direction, view_center;
                        float view_width;
                        m_show->get_camera_information(&camera_up, &camera_direction, &view_center, &view_width);

                        create_painter_window(window_name + " (Cornell Box)", thread_count,
                                              cornell_box(paint_width, paint_height, *mesh_pointer, m_mesh_object_size,
                                                          default_color, diffuse, to_vector<double>(camera_direction),
                                                          to_vector<double>(camera_up)));
                }
        });
}
