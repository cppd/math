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

#include "graphics_widget.h"
#include "support.h"

#include "application/application_name.h"
#include "com/error.h"
#include "com/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "dft_test/dft_test.h"
#include "geometry/vec_glm.h"
#include "geometry_cocone/reconstruction.h"
#include "geometry_objects/points.h"
#include "geometry_test/convex_hull_test.h"
#include "geometry_test/reconstruction_test.h"
#include "obj/obj_alg.h"
#include "obj/obj_convex_hull.h"
#include "obj/obj_file_load.h"
#include "obj/obj_file_save.h"
#include "obj/obj_points_load.h"
#include "obj/obj_surface.h"
#include "progress/progress.h"
#include "qt_dialog/application_about.h"
#include "qt_dialog/application_help.h"
#include "qt_dialog/bound_cocone_parameters.h"
#include "qt_dialog/message_box.h"
#include "qt_dialog/source_error.h"

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
constexpr QRgb CLEAR_COLOR = qRgb(20, 50, 80);
constexpr QRgb DEFAULT_COLOR = qRgb(150, 170, 150);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

// Идентификаторы объектов для взаимодействия с модулем рисования,
// куда передаются как числа, а не как enum
enum ObjectType
{
        MODEL,
        MODEL_CONVEX_HULL,
        SURFACE_COCONE,
        SURFACE_COCONE_CONVEX_HULL,
        SURFACE_BOUND_COCONE,
        SURFACE_BOUND_COCONE_CONVEX_HULL
};

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          m_first_show(true),
          m_working_open_object(false),
          m_working_bound_cocone(false),
          m_object_repository(create_object_repository<3>())
{
        static_assert(std::is_same_v<decltype(ui.graphics_widget), GraphicsWidget*>);

        ui.setupUi(this);

        QMainWindow::setWindowTitle(APPLICATION_NAME);

        QMainWindow::addAction(ui.actionFullScreen);

        qRegisterMetaType<WindowEvent>("WindowEvent");
        connect(&m_event_emitter, SIGNAL(window_event(WindowEvent)), this, SLOT(on_window_event(WindowEvent)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        ui.graphics_widget->setText("");
        connect(ui.graphics_widget, SIGNAL(wheel(double)), this, SLOT(on_widget_under_window_mouse_wheel(double)));
        connect(ui.graphics_widget, SIGNAL(resize()), this, SLOT(on_widget_under_window_resize()));

        connect(&m_timer_progress_bar, SIGNAL(timeout()), this, SLOT(on_timer_progress_bar()));

        set_widgets_enabled(this->layout(), true);
        set_dependent_interface();
        strike_out_all_objects_buttons();

        set_bound_cocone_parameters(BOUND_COCONE_DEFAULT_RHO, BOUND_COCONE_DEFAULT_ALPHA);

        set_clear_color(CLEAR_COLOR);
        set_default_color(DEFAULT_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);

        ui.mainWidget->layout()->setContentsMargins(1, 1, 1, 1);
        ui.mainWidget->layout()->setSpacing(1);

        ui.radioButton_Model->setChecked(true);

        ui.tabWidget->setCurrentIndex(0);

        ui.actionHelp->setText(QString(APPLICATION_NAME) + " Help");
        ui.actionAbout->setText("About " + QString(APPLICATION_NAME));

        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);
        for (const std::string& object_name : m_object_repository->get_list_of_point_objects())
        {
                QAction* action = ui.menuCreate->addAction(object_name.c_str());
                m_action_to_object_name_map.emplace(action, object_name);
                connect(action, SIGNAL(triggered()), this, SLOT(on_action_object_repository()));
        }
}

MainWindow::~MainWindow()
{
        stop_main_threads();
        stop_test_threads();
}

void MainWindow::thread_test() noexcept
{
        try
        {
                {
                        ProgressRatio progress(&m_progress_ratio_list_tests, "Test DFT in 2D");
                        progress.set(0);
                        dft_test();
                }
                {
                        ProgressRatio progress(&m_progress_ratio_list_tests, "Test convex hull in 4D");
                        convex_hull_test(4, &progress);
                }
                {
                        ProgressRatio progress(&m_progress_ratio_list_tests, "Test convex hull in 5D");
                        convex_hull_test(5, &progress);
                }
                {
                        ProgressRatio progress(&m_progress_ratio_list_tests, "Test 1-manifold reconstruction in 2D");
                        reconstruction_test(2, &progress);
                }
                {
                        ProgressRatio progress(&m_progress_ratio_list_tests, "Test 2-manifold reconstruction in 3D");
                        reconstruction_test(3, &progress);
                }
                {
                        ProgressRatio progress(&m_progress_ratio_list_tests, "Test 3-manifold reconstruction in 4D");
                        reconstruction_test(4, &progress);
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (ErrorSourceException& e)
        {
                m_event_emitter.error_source_message(e.get_msg(), e.get_src());
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message(e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while testing");
        }

        m_working_test = false;
}

void MainWindow::thread_model(std::shared_ptr<IObj> obj) noexcept
{
        try
        {
                if (obj->get_faces().size() == 0 && obj->get_points().size() == 0)
                {
                        return;
                }

                m_show->add_object(obj, MODEL, MODEL);

                ProgressRatio progress(&m_progress_ratio_list);
                progress.set_text("Convex hull 3D: %v of %m");

                std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(obj.get(), &progress);

                if (convex_hull->get_faces().size() != 0)
                {
                        m_show->add_object(convex_hull, MODEL_CONVEX_HULL, MODEL);
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message(std::string("Convex hull 3D:\n") + e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while convex hull creating");
        }
}

void MainWindow::thread_cocone() noexcept
{
        try
        {
                {
                        ProgressRatio progress(&m_progress_ratio_list);

                        double start_time = get_time_seconds();

                        std::vector<vec<3>> normals;
                        std::vector<std::array<int, 3>> facets;

                        m_surface_constructor->cocone(&normals, &facets, &progress);

                        m_surface_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                        LOG("Surface reconstruction second phase, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
                }

                if (m_surface_cocone->get_faces().size() != 0)
                {
                        m_show->add_object(m_surface_cocone, SURFACE_COCONE, MODEL);

                        ProgressRatio progress(&m_progress_ratio_list);
                        progress.set_text("COCONE convex hull 3D: %v of %m");

                        std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(m_surface_cocone.get(), &progress);

                        if (convex_hull->get_faces().size() != 0)
                        {
                                m_show->add_object(convex_hull, SURFACE_COCONE_CONVEX_HULL, MODEL);
                        }
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message(std::string("COCONE reconstruction:\n") + e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while COCONE reconstructing");
        }
}

void MainWindow::thread_bound_cocone(double rho, double alpha) noexcept
{
        try
        {
                {
                        ProgressRatio progress(&m_progress_ratio_list);

                        double start_time = get_time_seconds();

                        std::vector<vec<3>> normals;
                        std::vector<std::array<int, 3>> facets;

                        m_surface_constructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                        m_surface_bound_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                        LOG("Surface reconstruction second phase, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
                }

                m_show->delete_object(SURFACE_BOUND_COCONE);
                m_show->delete_object(SURFACE_BOUND_COCONE_CONVEX_HULL);

                m_event_emitter.bound_cocone_loaded(rho, alpha);

                if (m_surface_bound_cocone->get_faces().size() != 0)
                {
                        m_show->add_object(m_surface_bound_cocone, SURFACE_BOUND_COCONE, MODEL);

                        ProgressRatio progress(&m_progress_ratio_list);
                        progress.set_text("BOUND COCONE convex hull 3D: %v of %m");

                        std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(m_surface_bound_cocone.get(), &progress);

                        if (convex_hull->get_faces().size() != 0)
                        {
                                m_show->add_object(convex_hull, SURFACE_BOUND_COCONE_CONVEX_HULL, MODEL);
                        }
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message(std::string("BOUND COCONE reconstruction:\n") + e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while BOUND COCONE reconstructing");
        }

        m_working_bound_cocone = false;
}

void MainWindow::thread_surface_constructor() noexcept
{
        try
        {
                ProgressRatio progress(&m_progress_ratio_list);

                double start_time = get_time_seconds();

                m_surface_constructor = create_manifold_constructor(to_vector<float>(m_surface_points), &progress);

                LOG("Surface reconstruction first phase, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");

                std::vector<std::thread> threads(2);
                std::vector<std::string> msg(2);

                launch_class_thread(&threads[0], &msg[0], &MainWindow::thread_cocone, this);
                launch_class_thread(&threads[1], &msg[1], &MainWindow::thread_bound_cocone, this, m_bound_cocone_rho,
                                    m_bound_cocone_alpha);

                join_threads(&threads, &msg);
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message(std::string("Surface reconstructing:\n") + e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while surface reconstructing");
        }
}

void MainWindow::thread_open_object(const std::string& object_name, OpenObjectType object_type) noexcept
{
        try
        {
                std::shared_ptr<IObj> obj;

                {
                        ProgressRatio progress(&m_progress_ratio_list);
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

                m_event_emitter.file_loaded(object_name);

                m_surface_points = (obj->get_faces().size() > 0) ? get_unique_face_vertices(obj.get()) :
                                                                   get_unique_point_vertices(obj.get());

                std::vector<std::thread> threads(2);
                std::vector<std::string> msg(2);

                launch_class_thread(&threads[0], &msg[0], &MainWindow::thread_model, this, obj);
                launch_class_thread(&threads[1], &msg[1], &MainWindow::thread_surface_constructor, this);

                join_threads(&threads, &msg);
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message("loading " + object_name + ":\n" + e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while loading " + object_name);
        }

        m_working_open_object = false;
}

void MainWindow::start_thread_open_object(const std::string& object_name, OpenObjectType object_type)
{
        stop_main_threads();

        m_working_open_object = true;

        launch_class_thread(&m_thread_open_object, nullptr, &MainWindow::thread_open_object, this, object_name, object_type);
}

void MainWindow::start_thread_bound_cocone(double rho, double alpha)
{
        if (m_thread_bound_cocone.joinable())
        {
                m_thread_bound_cocone.join();
        }

        m_working_bound_cocone = true;

        launch_class_thread(&m_thread_bound_cocone, nullptr, &MainWindow::thread_bound_cocone, this, rho, alpha);
}

void MainWindow::start_thread_test()
{
        stop_test_threads();

        m_working_test = true;

        launch_class_thread(&m_thread_test, nullptr, &MainWindow::thread_test, this);
}

void MainWindow::stop_main_threads()
{
        m_progress_ratio_list.stop_all();

        if (m_thread_open_object.joinable())
        {
                m_thread_open_object.join();
        }
        if (m_thread_bound_cocone.joinable())
        {
                m_thread_bound_cocone.join();
        }

        m_progress_ratio_list.enable();
}

void MainWindow::stop_test_threads()
{
        m_progress_ratio_list_tests.stop_all();

        if (m_thread_test.joinable())
        {
                m_thread_test.join();
        }

        m_progress_ratio_list_tests.enable();
}

bool MainWindow::main_threads_busy_with_message()
{
        if (m_working_open_object)
        {
                message_warning(this, "Busy loading object");
                return true;
        }
        if (m_working_bound_cocone)
        {
                message_warning(this, "Busy loading BOUND COCONE");
                return true;
        }
        return false;
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

void MainWindow::on_timer_progress_bar()
{
        progress_bars(false, &m_progress_ratio_list, &m_progress_bars);
        progress_bars(true, &m_progress_ratio_list_tests, &m_progress_bars_tests);
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

void MainWindow::on_window_event(const WindowEvent& event)
{
        switch (event.get_type())
        {
        case WindowEvent::EventType::ERROR_FATAL_MESSAGE:
        {
                const WindowEvent::error_fatal_message& d = event.get<WindowEvent::error_fatal_message>();

                const char* message = (d.msg.size() != 0) ? d.msg.c_str() : "Unknown Error. Exit failure.";

                LOG_ERROR(message);

                message_critical(this, message);

                close();

                break;
        }
        case WindowEvent::EventType::ERROR_SOURCE_MESSAGE:
        {
                const WindowEvent::error_source_message& d = event.get<WindowEvent::error_source_message>();

                std::string message = d.msg;
                std::string source = source_with_line_numbers(d.src);

                LOG_ERROR(message);
                LOG_ERROR(source);

                SourceError(this).show(message.c_str(), source.c_str());

                close();

                break;
        }
        case WindowEvent::EventType::ERROR_MESSAGE:
        {
                const WindowEvent::error_message& d = event.get<WindowEvent::error_message>();

                LOG_ERROR(d.msg);

                message_critical(this, d.msg.c_str());

                break;
        }
        case WindowEvent::EventType::WINDOW_READY:
        {
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
        QTimer::singleShot(WINDOW_SHOW_DELAY_MSEC, this, SLOT(on_window_first_shown()));
}

void MainWindow::on_window_first_shown()
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

        start_thread_test();

        try
        {
                m_show = create_show(&m_event_emitter, get_widget_window_id(ui.graphics_widget), qcolor_to_vec3(m_clear_color),
                                     qcolor_to_vec3(m_default_color), qcolor_to_vec3(m_wireframe_color),
                                     ui.checkBox_Smooth->isChecked(), ui.checkBox_Wireframe->isChecked(),
                                     ui.checkBox_Shadow->isChecked(), ui.checkBox_Materials->isChecked(),
                                     ui.checkBox_ShowEffect->isChecked(), ui.checkBox_show_dft->isChecked(),
                                     ui.checkBox_convex_hull_2d->isChecked(), ui.checkBox_OpticalFlow->isChecked(), get_ambient(),
                                     get_diffuse(), get_specular(), get_dft_brightness(), get_default_ns());
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_fatal_message(e.what());
                return;
        }
        catch (...)
        {
                m_event_emitter.error_fatal_message("");
                return;
        }

        if (QCoreApplication::arguments().count() == 2)
        {
                start_thread_open_object(QCoreApplication::arguments().at(1).toStdString(), OpenObjectType::File);
        }
}

void MainWindow::on_actionLoad_triggered()
{
        QString file_name = QFileDialog::getOpenFileName(this, "Open", "", "OBJ and Point files (*.obj *.txt)", nullptr,
                                                         QFileDialog::ReadOnly | QFileDialog::DontUseNativeDialog);
        if (file_name.size() > 0)
        {
                start_thread_open_object(file_name.toStdString(), OpenObjectType::File);
        }
}

void MainWindow::on_action_object_repository()
{
        auto iter = m_action_to_object_name_map.find(sender());
        if (iter != m_action_to_object_name_map.cend())
        {
                start_thread_open_object(iter->second, OpenObjectType::Repository);
        }
        else
        {
                m_event_emitter.error_message("open object sender not found in map");
        }
}

void MainWindow::on_actionExport_triggered()
{
        if (main_threads_busy_with_message())
        {
                return;
        }

        if (!(ui.radioButton_Cocone->isChecked() || ui.radioButton_BoundCocone->isChecked()))
        {
                message_warning(this, "Select COCONE or BOUND COCONE");
                return;
        }

        if (ui.radioButton_Cocone->isChecked() && (!m_surface_cocone || m_surface_cocone->get_faces().size() == 0))
        {
                message_warning(this, "COCONE not created");
                return;
        }

        if (ui.radioButton_BoundCocone->isChecked() &&
            (!m_surface_bound_cocone || m_surface_bound_cocone->get_faces().size() == 0))
        {
                message_warning(this, "BOUND COCONE not created");
                return;
        }

        QString cocone_type;
        const IObj* obj;

        if (ui.radioButton_Cocone->isChecked())
        {
                cocone_type = "COCONE";
                obj = m_surface_cocone.get();
        }
        else if (ui.radioButton_BoundCocone->isChecked())
        {
                cocone_type = "BOUND COCONE";
                obj = m_surface_bound_cocone.get();
        }
        else
        {
                return;
        }

        QString file_name = QFileDialog::getSaveFileName(this, "Export " + cocone_type + " to OBJ", "", "OBJ files (*.obj)",
                                                         nullptr, QFileDialog::DontUseNativeDialog);
        if (file_name.size() == 0)
        {
                return;
        }

        // Здесь запись в файл делается в потоке интерфейса, поэтому во время записи
        // не может начаться загрузка файла с удалением объекта obj
        try
        {
                save_obj_geometry_to_file(obj, file_name.toStdString(), cocone_type.toStdString());
        }
        catch (std::exception& e)
        {
                m_event_emitter.error_message("Export " + cocone_type.toStdString() + " to file:\n" + e.what());
        }
        catch (...)
        {
                m_event_emitter.error_message("Unknown error while exporting " + cocone_type.toStdString() + " to file");
        }

        message_information(this, cocone_type + " exported to file " + file_name);
}

void MainWindow::on_Button_LoadBoundCocone_clicked()
{
        if (main_threads_busy_with_message())
        {
                return;
        }

        if (!m_surface_constructor)
        {
                message_warning(this, "No surface constructor");
                return;
        }

        double rho = m_bound_cocone_rho;
        double alpha = m_bound_cocone_alpha;

        if (!BoundCoconeParameters(this).show(BOUND_COCONE_DISPLAY_DIGITS, &rho, &alpha))
        {
                return;
        }

        start_thread_bound_cocone(rho, alpha);
}

void MainWindow::on_actionExit_triggered()
{
        close();
}

void MainWindow::on_actionHelp_triggered()
{
        application_help(this);
}

void MainWindow::on_actionAbout_triggered()
{
        application_about(this);
}

void MainWindow::on_Button_ResetView_clicked()
{
        m_show->reset_view();
}

void MainWindow::on_widget_under_window_mouse_wheel(double delta)
{
        if (m_show)
        {
                m_show->mouse_wheel(delta);
        }
}

void MainWindow::on_widget_under_window_resize()
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
