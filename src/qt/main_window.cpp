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

#include "qt_support.h"

#include "com/error.h"
#include "com/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "geometry/surface_cocone.h"
#include "geometry/vec_glm.h"
#include "obj/obj_alg.h"
#include "obj/obj_convex_hull.h"
#include "obj/obj_file_load.h"
#include "obj/obj_file_save.h"
#include "obj/obj_surface.h"
#include "progress/progress.h"
#include "qt_dialog/dialogs.h"

#include <QColorDialog>
#include <QDateTime>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QObjectList>
#include <array>
#include <cmath>
#include <thread>
#include <type_traits>

constexpr const char* APPLICATION_NAME = "OBJ Math Viewer";

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

// Идентификаторы объектов для взаимодействия с модулем рисования,
// куда передаются как числа, а не как enum
enum ObjectType
{
        FACES,
        FACES_CONVEX_HULL,
        SURFACE_COCONE,
        SURFACE_COCONE_CONVEX_HULL,
        SURFACE_BOUND_COCONE,
        SURFACE_BOUND_COCONE_CONVEX_HULL
};

#define CATCH_ALL_SEND_EVENT()                                                                               \
        catch (std::exception & e)                                                                           \
        {                                                                                                    \
                error_fatal(std::string("Critical program error. Send event exception: ") + e.what() + "."); \
        }                                                                                                    \
        catch (...)                                                                                          \
        {                                                                                                    \
                error_fatal("Critical program error. Send event exception.");                                \
        }

namespace
{
WindowID get_widget_window_id(QWidget* widget)
{
        static_assert(sizeof(WindowID) == sizeof(WId));
        static_assert(std::is_integral<WindowID>::value || std::is_pointer<WindowID>::value);
        static_assert(std::is_integral<WId>::value || std::is_pointer<WId>::value);

        union {
                WId w_id;
                WindowID window_id;
        };

        w_id = widget->winId();
        return window_id;
}
}

void MainWindow::message_about(const std::string& msg)
{
        QMessageBox::about(this, QString("About ") + APPLICATION_NAME, msg.c_str());
}
void MainWindow::message_critical(const std::string& msg)
{
        QMessageBox::critical(this, APPLICATION_NAME, msg.c_str());
}
void MainWindow::message_information(const std::string& msg)
{
        QMessageBox::information(this, APPLICATION_NAME, msg.c_str());
}
void MainWindow::message_warning(const std::string& msg)
{
        QMessageBox::warning(this, APPLICATION_NAME, msg.c_str());
}

void MainWindow::on_actionAbout_triggered()
{
#if defined(__linux__)
        message_about(
                "\n\n"
                "C++17\nGLSL 4.50\n"
                "\n"
                "Freetype\nGLM\nGMP\nOpenGL\nQt\nSFML\nX11");
#elif defined(_WIN32)
        message_about(
                "\n\n"
                "C++17\nGLSL 4.50\n"
                "\n"
                "Freetype\nGLM\nGMP\nOpenGL\nQt\nSFML");
#else
#error This operating system is not supported
#endif
}

void MainWindow::on_actionKeyboard_and_Mouse_triggered()
{
        message_information(
                "Move: left mouse button.\n\n"
                "Rotate: right mouse button.\n\n"
                "Zoom: mouse wheel.\n\n"
                "Toggle fullscreen: F11.");
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
        ui.setupUi(this);

        qRegisterMetaType<WindowEvent>("WindowEvent");

        connect(this, SIGNAL(SignalWindowEvent(WindowEvent)), this, SLOT(on_MainWindow_SignalWindowEvent(WindowEvent)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        connect(ui.widget_under_window, SIGNAL(wheel(double)), this, SLOT(widget_under_window_mouse_wheel_slot(double)));

        connect(&m_timer, SIGNAL(timeout()), this, SLOT(timer_slot()));

        m_first_show = true;

        m_file_loading = false;
        m_bound_cocone_loading = false;

        set_widgets_enabled(this->layout(), true);

        set_dependent_interface_enabled();

        disable_object_buttons();

        ui.widget_under_window->setText("");
        m_widget_under_window = ui.widget_under_window;

        m_clear_color = ui.widget_clear_color->palette().color(QPalette::Window);
        m_default_color = ui.widget_default_color->palette().color(QPalette::Window);
        m_wireframe_color = ui.widget_wireframe_color->palette().color(QPalette::Window);

        QLayout* l = ui.verticalLayoutWidget->layout();
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        ui.verticalLayoutWidget->setLayout(l);

        this->addAction(ui.actionFullScreen);

        this->setWindowTitle(APPLICATION_NAME);

        m_bound_cocone_rho = BOUND_COCONE_DEFAULT_RHO;
        m_bound_cocone_alpha = BOUND_COCONE_DEFAULT_ALPHA;
        set_bound_cocone_label(m_bound_cocone_rho, m_bound_cocone_alpha);

        ui.radioButton_Faces->setChecked(true);

        ui.tabWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
        stop_all_threads();
}

void MainWindow::thread_faces(std::shared_ptr<IObj> obj) noexcept
{
        try
        {
                if (obj->get_faces().size() == 0 && obj->get_points().size() == 0)
                {
                        return;
                }

                m_show->add_object(obj, FACES);

                ProgressRatio progress(&m_progress_ratio_list);
                progress.set_text("Convex hull 3D: %v of %m");

                std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(obj.get(), &progress);

                if (convex_hull->get_faces().size() != 0)
                {
                        m_show->add_object(convex_hull, FACES_CONVEX_HULL);
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                error_message(std::string("Convex hull 3D:\n") + e.what());
        }
        catch (...)
        {
                error_message("Unknown error while convex hull creating");
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

                        m_surface_reconstructor->cocone(&normals, &facets, &progress);

                        m_surface_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                        LOG("Surface reconstruction second phase, " + to_string(get_time_seconds() - start_time, 5) + " s");
                }

                if (m_surface_cocone->get_faces().size() != 0)
                {
                        m_show->add_object(m_surface_cocone, SURFACE_COCONE);

                        ProgressRatio progress(&m_progress_ratio_list);
                        progress.set_text("Cocone convex hull 3D: %v of %m");

                        std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(m_surface_cocone.get(), &progress);

                        if (convex_hull->get_faces().size() != 0)
                        {
                                m_show->add_object(convex_hull, SURFACE_COCONE_CONVEX_HULL);
                        }
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                error_message(std::string("COCONE reconstruction:\n") + e.what());
        }
        catch (...)
        {
                error_message("Unknown error while COCONE reconstruction");
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

                        m_surface_reconstructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                        m_surface_bound_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                        LOG("Surface reconstruction second phase, " + to_string(get_time_seconds() - start_time, 5) + " s");
                }

                m_show->delete_object(SURFACE_BOUND_COCONE);
                m_show->delete_object(SURFACE_BOUND_COCONE_CONVEX_HULL);

                bound_cocone_loaded(rho, alpha);

                if (m_surface_bound_cocone->get_faces().size() != 0)
                {
                        m_show->add_object(m_surface_bound_cocone, SURFACE_BOUND_COCONE);

                        ProgressRatio progress(&m_progress_ratio_list);
                        progress.set_text("Bound cocone convex hull 3D: %v of %m");

                        std::shared_ptr<IObj> convex_hull = create_convex_hull_for_obj(m_surface_bound_cocone.get(), &progress);

                        if (convex_hull->get_faces().size() != 0)
                        {
                                m_show->add_object(convex_hull, SURFACE_BOUND_COCONE_CONVEX_HULL);
                        }
                }
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                error_message(std::string("BOUND COCONE reconstruction:\n") + e.what());
        }
        catch (...)
        {
                error_message("Unknown error while BOUND COCONE reconstruction");
        }

        m_bound_cocone_loading = false;
}

void MainWindow::thread_surface_reconstructor() noexcept
{
        try
        {
                ProgressRatio progress(&m_progress_ratio_list);

                double start_time = get_time_seconds();

                m_surface_reconstructor = create_surface_reconstructor(to_vector<float>(m_surface_points), &progress);

                LOG("Surface reconstruction first phase, " + to_string(get_time_seconds() - start_time, 5) + " s");

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
                error_message(std::string("Surface reconstructing:\n") + e.what());
        }
        catch (...)
        {
                error_message("Unknown error while surface reconstructing");
        }
}

void MainWindow::thread_open_file(const std::string& file_name) noexcept
{
        try
        {
                std::shared_ptr<IObj> obj;

                {
                        ProgressRatio progress(&m_progress_ratio_list);
                        progress.set_text("Load file: %p%");
                        obj = load_obj_from_file(file_name, &progress);
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
                m_surface_reconstructor.reset();
                m_surface_cocone.reset();
                m_surface_bound_cocone.reset();

                file_loaded(file_name);

                m_surface_points = (obj->get_faces().size() > 0) ? get_unique_face_vertices(obj.get()) :
                                                                   get_unique_point_vertices(obj.get());

                std::vector<std::thread> threads(2);
                std::vector<std::string> msg(2);

                launch_class_thread(&threads[0], &msg[0], &MainWindow::thread_faces, this, obj);
                launch_class_thread(&threads[1], &msg[1], &MainWindow::thread_surface_reconstructor, this);

                join_threads(&threads, &msg);
        }
        catch (TerminateRequestException&)
        {
        }
        catch (std::exception& e)
        {
                error_message("loading " + file_name + ":\n" + e.what());
        }
        catch (...)
        {
                error_message("Unknown error while loading file " + file_name);
        }

        m_file_loading = false;
}

void MainWindow::start_thread_open_file(const std::string& file_name)
{
        stop_all_threads();

        m_file_loading = true;

        launch_class_thread(&m_open_file_thread, nullptr, &MainWindow::thread_open_file, this, file_name);
}

void MainWindow::stop_all_threads()
{
        m_progress_ratio_list.stop_all();

        if (m_open_file_thread.joinable())
        {
                m_open_file_thread.join();
        }
        if (m_bound_cocone_thread.joinable())
        {
                m_bound_cocone_thread.join();
        }

        m_progress_ratio_list.enable();
}

void MainWindow::on_Button_LoadBoundCocone_clicked()
{
        if (m_file_loading)
        {
                message_warning("Busy loading file");
                return;
        }
        if (m_bound_cocone_loading)
        {
                message_warning("Busy loading bound cocone");
                return;
        }
        if (!m_surface_reconstructor)
        {
                message_warning("No surface reconstructor");
                return;
        }

        double rho = m_bound_cocone_rho;
        double alpha = m_bound_cocone_alpha;

        if (!edit_bound_cocone_parameters(this, BOUND_COCONE_DISPLAY_DIGITS, &rho, &alpha))
        {
                return;
        }

        if (m_bound_cocone_thread.joinable())
        {
                m_bound_cocone_thread.join();
        }

        m_bound_cocone_loading = true;

        launch_class_thread(&m_bound_cocone_thread, nullptr, &MainWindow::thread_bound_cocone, this, rho, alpha);
}

void MainWindow::timer_slot()
{
        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios = m_progress_ratio_list.get_all();

        if (ratios.size() > m_progress_bars.size())
        {
                m_progress_bars.resize(ratios.size());
        }

        std::list<QProgressBar>::iterator bar = m_progress_bars.begin();

        for (unsigned i = 0; i < ratios.size(); ++i, ++bar)
        {
                if (!bar->isVisible())
                {
                        ui.statusBar->addWidget(&(*bar));
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

        while (bar != m_progress_bars.end())
        {
                ui.statusBar->removeWidget(&(*bar));
                bar = m_progress_bars.erase(bar);
        }
}

void MainWindow::set_bound_cocone_label(double rho, double alpha)
{
        std::string bc =
                "ρ " + to_string(rho, BOUND_COCONE_DISPLAY_DIGITS) + "; α " + to_string(alpha, BOUND_COCONE_DISPLAY_DIGITS);
        ui.BoundCocone_label->setText(bc.c_str());
}

void MainWindow::set_dependent_interface_enabled()
{
        ui.Label_DFT_Brightness->setEnabled(ui.checkBox_show_dft->isEnabled() && ui.checkBox_show_dft->isChecked());
        ui.Slider_DFT_Brightness->setEnabled(ui.checkBox_show_dft->isEnabled() && ui.checkBox_show_dft->isChecked());

        // ui.widget_wireframe_color->setEnabled(ui.checkBox_Wireframe->isEnabled() && ui.checkBox_Wireframe->isChecked());
        // ui.ButtonWireframeColor->setEnabled(ui.checkBox_Wireframe->isEnabled() && ui.checkBox_Wireframe->isChecked());
        // ui.label_wireframe_color->setEnabled(ui.checkBox_Wireframe->isEnabled() && ui.checkBox_Wireframe->isChecked());
}

void MainWindow::disable_radio_button(QRadioButton* button)
{
        button_strike_out(button, true);
}

void MainWindow::disable_object_buttons()
{
        disable_radio_button(ui.radioButton_Faces);
        disable_radio_button(ui.radioButton_FacesConvexHull);
        disable_radio_button(ui.radioButton_Cocone);
        disable_radio_button(ui.radioButton_CoconeConvexHull);
        disable_radio_button(ui.radioButton_BoundCocone);
        disable_radio_button(ui.radioButton_BoundCoconeConvexHull);
}

void MainWindow::disable_bound_cocone_buttons()
{
        disable_radio_button(ui.radioButton_BoundCocone);
        disable_radio_button(ui.radioButton_BoundCoconeConvexHull);
}

void MainWindow::enable_radio_button(QRadioButton* button)
{
        button_strike_out(button, false);

        if (button->isChecked())
        {
                button->click();
        }
}

void MainWindow::on_MainWindow_SignalWindowEvent(const WindowEvent& event)
{
        switch (event.get_type())
        {
        case WindowEvent::EventType::PROGRAM_ENDED:
        {
                const WindowEvent::program_ended& d = event.get<WindowEvent::program_ended>();

                const char* message = (d.msg.size() != 0) ? d.msg.c_str() : "Unknown Error. Exit failure.";

                LOG(message);

                message_critical(message);

                close();

                break;
        }
        case WindowEvent::EventType::ERROR_SRC_MESSAGE:
        {
                const WindowEvent::error_src_message& d = event.get<WindowEvent::error_src_message>();

                LOG(d.msg);

                show_source_error(this, d.msg, source_with_line_numbers(d.src));

                close();

                break;
        }
        case WindowEvent::EventType::ERROR_MESSAGE:
        {
                const WindowEvent::error_message& d = event.get<WindowEvent::error_message>();

                LOG(d.msg);

                message_critical(d.msg);

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
                case FACES:
                        enable_radio_button(ui.radioButton_Faces);
                        break;
                case FACES_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_FacesConvexHull);
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

                disable_object_buttons();

                break;
        }
        case WindowEvent::EventType::BOUND_COCONE_LOADED:
        {
                const WindowEvent::bound_cocone_loaded& d = event.get<WindowEvent::bound_cocone_loaded>();

                m_bound_cocone_rho = d.rho;
                m_bound_cocone_alpha = d.alpha;

                set_bound_cocone_label(m_bound_cocone_rho, m_bound_cocone_alpha);

                disable_bound_cocone_buttons();

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
        QTimer::singleShot(50, this, SLOT(window_shown()));
}

void MainWindow::resize_window()
{
        if (WINDOW_SIZE_GRAPHICS)
        {
                QRect screen_geometry = QDesktopWidget().screenGeometry(this);
                QSize graphics_size = screen_geometry.size() * WINDOW_SIZE_COEF;

                // Из документации на resize: the size excluding any window frame
                this->resize(graphics_size + (this->geometry().size() - m_widget_under_window->size()));
        }
        else
        {
                QRect desktop_geometry = QDesktopWidget().availableGeometry(this);
                QSize window_size = desktop_geometry.size() * WINDOW_SIZE_COEF;

                // Из документации на resize: the size excluding any window frame
                this->resize(window_size - (this->frameGeometry().size() - this->geometry().size()));
        }
}

void MainWindow::move_window_to_desktop_center()
{
        // Из документации на move: the position on the desktop, including frame
        this->move((QDesktopWidget().availableGeometry(this).width() - this->frameGeometry().width()) / 2,
                   (QDesktopWidget().availableGeometry(this).height() - this->frameGeometry().height()) / 2);
}

void MainWindow::window_shown()
{
        resize_window();

        move_window_to_desktop_center();

        try
        {
                m_show = create_show(this, get_widget_window_id(m_widget_under_window), qcolor_to_vec3(m_clear_color),
                                     qcolor_to_vec3(m_default_color), qcolor_to_vec3(m_wireframe_color),
                                     ui.checkBox_Smooth->isChecked(), ui.checkBox_Wireframe->isChecked(),
                                     ui.checkBox_Shadow->isChecked(), ui.checkBox_Materials->isChecked(),
                                     ui.checkBox_ShowEffect->isChecked(), ui.checkBox_show_dft->isChecked(),
                                     ui.checkBox_convex_hull_2d->isChecked(), ui.checkBox_OpticalFlow->isChecked(), get_ambient(),
                                     get_diffuse(), get_specular(), get_dft_brightness(), get_default_ns());
        }
        catch (std::exception& e)
        {
                program_ended(e.what());
                return;
        }
        catch (...)
        {
                program_ended("");
                return;
        }

        if (QCoreApplication::arguments().count() == 2)
        {
                start_thread_open_file(QCoreApplication::arguments().at(1).toStdString());
        }

        m_timer.start(TIMER_PROGRESS_BAR_INTERVAL);
}

void MainWindow::on_actionLoad_triggered()
{
        QString file_name = QFileDialog::getOpenFileName(this, "Open OBJ file", "", "OBJ files (*.obj);; Points files(*.txt)",
                                                         nullptr, QFileDialog::ReadOnly | QFileDialog::DontUseNativeDialog);
        if (file_name.size() > 0)
        {
                start_thread_open_file(file_name.toStdString());
        }
}

void MainWindow::on_actionExport_triggered()
{
        if (m_file_loading)
        {
                message_warning("Loading file");
                return;
        }

        if (!(ui.radioButton_Cocone->isChecked() || ui.radioButton_BoundCocone->isChecked()))
        {
                message_warning("Select COCONE or BOUND COCONE");
                return;
        }

        if (ui.radioButton_Cocone->isChecked() && (!m_surface_cocone || m_surface_cocone->get_faces().size() == 0))
        {
                message_warning("COCONE not created");
                return;
        }

        if (ui.radioButton_BoundCocone->isChecked() &&
            (!m_surface_bound_cocone || m_surface_bound_cocone->get_faces().size() == 0))
        {
                message_warning("BOUND COCONE not created");
                return;
        }

        std::string name;
        const IObj* obj;

        if (ui.radioButton_Cocone->isChecked())
        {
                name = "COCONE";
                obj = m_surface_cocone.get();
        }
        else if (ui.radioButton_BoundCocone->isChecked())
        {
                name = "BOUND COCONE";
                obj = m_surface_bound_cocone.get();
        }
        else
        {
                return;
        }

        QString file_name = QFileDialog::getSaveFileName(this, "Export " + QString(name.c_str()) + " to OBJ file", "",
                                                         "OBJ files (*.obj)", nullptr, QFileDialog::DontUseNativeDialog);
        if (file_name.size() == 0)
        {
                return;
        }

        // Здесь запись в файл делается в потоке интерфейса, поэтому во время записи
        // не может начаться загрузка файла с удалением объекта obj
        try
        {
                save_obj_geometry_to_file(obj, file_name.toStdString(), name);
        }
        catch (std::exception& e)
        {
                error_message("Export " + name + " to file:\n" + e.what());
        }
        catch (...)
        {
                error_message("Unknown error while export " + name + " to file");
        }

        message_information(name + " exported.\n\n" + file_name.toStdString());
}

void MainWindow::resizeEvent(QResizeEvent*)
{
        if (m_show)
        {
                m_show->parent_resized();
        }
}

void MainWindow::error_message(const std::string& msg) noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::error_message>, msg));
}
CATCH_ALL_SEND_EVENT()
void MainWindow::window_ready() noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::window_ready>));
}
CATCH_ALL_SEND_EVENT()
void MainWindow::program_ended(const std::string& msg) noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::program_ended>, msg));
}
CATCH_ALL_SEND_EVENT()
void MainWindow::error_src_message(const std::string& msg, const std::string& src) noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::error_src_message>, msg, src));
}
CATCH_ALL_SEND_EVENT()
void MainWindow::object_loaded(int id) noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::object_loaded>, id));
}
CATCH_ALL_SEND_EVENT()
void MainWindow::file_loaded(const std::string& msg) noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::file_loaded>, msg));
}
CATCH_ALL_SEND_EVENT()
void MainWindow::bound_cocone_loaded(double rho, double alpha) noexcept try
{
        emit SignalWindowEvent(WindowEvent(in_place<WindowEvent::bound_cocone_loaded>, rho, alpha));
}
CATCH_ALL_SEND_EVENT()

void MainWindow::on_actionExit_triggered()
{
        close();
}

void MainWindow::on_Button_ResetView_clicked()
{
        m_show->reset_view();
}

void MainWindow::widget_under_window_mouse_wheel_slot(double delta)
{
        if (m_show)
        {
                m_show->mouse_wheel(delta);
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
        QColorDialog dialog(this);
        dialog.setCurrentColor(m_clear_color);
        dialog.setWindowTitle("Background color");
        dialog.setOptions(QColorDialog::NoButtons | QColorDialog::DontUseNativeDialog);
        connect(&dialog, &QColorDialog::currentColorChanged, [this](const QColor& c) {
                if (c.isValid())
                {
                        m_clear_color = c;
                        m_show->set_clear_color(qcolor_to_vec3(c));

                        QPalette palette;
                        palette.setColor(QPalette::Window, m_clear_color);
                        ui.widget_clear_color->setPalette(palette);
                }
        });

        dialog.exec();
}

void MainWindow::on_ButtonDefaultColor_clicked()
{
        QColor c = QColorDialog::getColor(m_default_color, this, "Default color", QColorDialog::DontUseNativeDialog);
        if (c.isValid())
        {
                m_default_color = c;
                m_show->set_default_color(qcolor_to_vec3(c));

                QPalette palette;
                palette.setColor(QPalette::Window, m_default_color);
                ui.widget_default_color->setPalette(palette);
        }
}

void MainWindow::on_ButtonWireframeColor_clicked()
{
        QColor c = QColorDialog::getColor(m_wireframe_color, this, "Wireframe color", QColorDialog::DontUseNativeDialog);
        if (c.isValid())
        {
                m_wireframe_color = c;
                m_show->set_wireframe_color(qcolor_to_vec3(c));

                QPalette palette;
                palette.setColor(QPalette::Window, m_wireframe_color);
                ui.widget_wireframe_color->setPalette(palette);
        }
}

void MainWindow::on_checkBox_Shadow_clicked()
{
        m_show->show_shadow(ui.checkBox_Shadow->isChecked());
}

void MainWindow::on_checkBox_Wireframe_clicked()
{
        // ui.widget_wireframe_color->setEnabled(ui.checkBox_Wireframe->isChecked());
        // ui.ButtonWireframeColor->setEnabled(ui.checkBox_Wireframe->isChecked());
        // ui.label_wireframe_color->setEnabled(ui.checkBox_Wireframe->isChecked());

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

void MainWindow::on_radioButton_Faces_clicked()
{
        m_show->show_object(FACES);
}

void MainWindow::on_radioButton_FacesConvexHull_clicked()
{
        m_show->show_object(FACES_CONVEX_HULL);
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
