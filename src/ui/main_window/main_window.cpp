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

#include "catch.h"
#include "identifiers.h"

#include "ui/dialogs/application_about.h"
#include "ui/dialogs/application_help.h"
#include "ui/dialogs/bound_cocone_parameters.h"
#include "ui/dialogs/message_box.h"
#include "ui/dialogs/point_object_parameters.h"
#include "ui/dialogs/source_error.h"
#include "ui/support/support.h"

#include "application/application_name.h"
#include "com/error.h"
#include "com/file/file_sys.h"
#include "com/log.h"
#include "com/names.h"
#include "com/print.h"

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFileDialog>

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

// Задержка в миллисекундах после showEvent для вызова по таймеру
// функции обработки появления окна.
constexpr int WINDOW_SHOW_DELAY_MSEC = 50;

// увеличение текстуры тени по сравнению с размером окна.
constexpr int SHADOW_ZOOM = 2;

// Для трассировки пути. Количество лучей на один пиксель в одном проходе.
constexpr int PATH_TRACING_DEFAULT_SAMPLES_PER_PIXEL = 25;
constexpr int PATH_TRACING_MAX_SAMPLES_PER_PIXEL = 100;

// Для трассировки пути для 3 измерений. Максимальный размер экрана в пикселях.
constexpr int PATH_TRACING_3D_MAX_SCREEN_SIZE = 10000;

// Для трассировки пути для 4 и более измерений. Размеры экрана в пикселях.
constexpr int PATH_TRACING_DEFAULT_SCREEN_SIZE = 500;
constexpr int PATH_TRACING_MINIMUM_SCREEN_SIZE = 50;
constexpr int PATH_TRACING_MAXIMUM_SCREEN_SIZE = 5000;

// Сколько потоков не надо использовать от максимума для создания октадеревьев.
constexpr int MESH_OBJECT_NOT_USED_THREAD_COUNT = 2;

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          m_window_thread_id(std::this_thread::get_id()),
          m_threads(m_event_emitter),
          m_objects(
                  create_main_objects(std::max(1, hardware_concurrency() - MESH_OBJECT_NOT_USED_THREAD_COUNT), m_event_emitter)),
          m_first_show(true),
          m_dimension(0)
{
        static_assert(std::is_same_v<decltype(ui.graphics_widget), GraphicsWidget*>);

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

        ui.graphics_widget->setText("");
        connect(ui.graphics_widget, SIGNAL(wheel(double)), this, SLOT(slot_widget_under_window_mouse_wheel(double)));
        connect(ui.graphics_widget, SIGNAL(resize()), this, SLOT(slot_widget_under_window_resize()));

        connect(&m_timer_progress_bar, SIGNAL(timeout()), this, SLOT(slot_timer_progress_bar()));
}

void MainWindow::constructor_interface()
{
        set_window_title_file("");

        QMainWindow::addAction(ui.actionFullScreen);

        set_widgets_enabled(QMainWindow::layout(), true);
        set_dependent_interface();
        strike_out_all_objects_buttons();

        set_bound_cocone_parameters(BOUND_COCONE_DEFAULT_RHO, BOUND_COCONE_DEFAULT_ALPHA);

        set_background_color(BACKGROUND_COLOR);
        set_default_color(DEFAULT_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);

        ui.mainWidget->layout()->setContentsMargins(3, 3, 3, 3);
        ui.mainWidget->layout()->setSpacing(3);

        ui.radioButton_Model->setChecked(true);

        ui.tabWidget->setCurrentIndex(0);

        ui.actionHelp->setText(QString(APPLICATION_NAME) + " Help");
        ui.actionAbout->setText("About " + QString(APPLICATION_NAME));

        ui.Slider_ShadowQuality->setSliderPosition(SHADOW_ZOOM);

        // Чтобы добавление и удаление QProgressBar не меняло высоту ui.statusBar
        ui.statusBar->setFixedHeight(ui.statusBar->height());
}

void MainWindow::constructor_repository()
{
        // QMenu* menuCreate = new QMenu("Create", this);
        // ui.menuBar->insertMenu(ui.menuHelp->menuAction(), menuCreate);
        for (const auto & [ dimension, object_names ] : m_objects->list_of_repository_point_objects())
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
        m_object_buttons.push_back({ui.radioButton_Model, OBJECT_MODEL});
        m_object_buttons.push_back({ui.radioButton_ModelMST, OBJECT_MODEL_MST});
        m_object_buttons.push_back({ui.radioButton_ModelConvexHull, OBJECT_MODEL_CONVEX_HULL});
        m_object_buttons.push_back({ui.radioButton_Cocone, OBJECT_COCONE});
        m_object_buttons.push_back({ui.radioButton_CoconeConvexHull, OBJECT_COCONE_CONVEX_HULL});
        m_object_buttons.push_back({ui.radioButton_BoundCocone, OBJECT_BOUND_COCONE});
        m_object_buttons.push_back({ui.radioButton_BoundCoconeConvexHull, OBJECT_BOUND_COCONE_CONVEX_HULL});
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

        stop_all_threads();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!message_question_default_no(this, "Do you want to close the main window?"))
        {
                event->ignore();
                return;
        }

        stop_all_threads();

        event->accept();
}

void MainWindow::stop_all_threads()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        m_threads.stop_all_threads();

        m_show.reset();

        set_log_callback(nullptr);
}

template <typename F>
void MainWindow::catch_all(const F& function) const noexcept
{
        static_assert(noexcept(catch_all_exceptions(m_event_emitter, function)));

        catch_all_exceptions(m_event_emitter, function);
}

void MainWindow::thread_load_from_file(std::string file_name)
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
                        QString caption = "Open";
                        QString filter =
                                file_filter("OBJ and Point files", m_objects->obj_extensions(), m_objects->txt_extensions());
                        QFileDialog::Options options = QFileDialog::ReadOnly | QFileDialog::DontUseNativeDialog;

                        QString q_file_name = QFileDialog::getOpenFileName(this, caption, "", filter, nullptr, options);
                        if (q_file_name.size() == 0)
                        {
                                return;
                        }

                        file_name = q_file_name.toStdString();
                }

                m_threads.start_thread(ThreadAction::LoadObject, [=](ProgressRatioList* progress_list, std::string* message) {
                        *message = "Load " + file_name;

                        m_objects->load_from_file(progress_list, file_name, m_bound_cocone_rho, m_bound_cocone_alpha);
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

                if (!PointObjectParameters(this).show(std::get<0>(object), std::get<1>(object), POINT_COUNT_DEFAULT,
                                                      POINT_COUNT_MINIMUM, POINT_COUNT_MAXIMUM, &point_count))
                {
                        return;
                }

                m_threads.start_thread(ThreadAction::LoadObject, [=](ProgressRatioList* progress_list, std::string* message) {
                        *message = "Load " + space_name(std::get<0>(object)) + " " + std::get<1>(object);

                        m_objects->load_from_repository(progress_list, object, m_bound_cocone_rho, m_bound_cocone_alpha,
                                                        point_count);
                });
        });
}

void MainWindow::thread_self_test(SelfTestType test_type)
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        if (!m_threads.action_allowed(ThreadAction::SelfTest))
        {
                m_event_emitter.message_warning("Self-Test is not available at this time (thread working)");
                return;
        }

        m_threads.start_thread(ThreadAction::SelfTest, [=](ProgressRatioList* progress_list, std::string* message) {
                *message = "Self-Test";

                self_test(test_type, progress_list, [&](const char* test_name, const auto& test_function) noexcept {
                        catch_all([&](std::string* m) {
                                *m = test_name;
                                test_function();
                        });
                });
        });
}

void MainWindow::thread_export(const std::string& name, int id)
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

        if (id == OBJECT_MODEL &&
            !message_question_default_no(this, "Only export of geometry is supported.\nDo you want to continue?"))
        {
                return;
        }

        if (m_dimension < 3)
        {
                m_event_emitter.message_error("No dimension information");
                return;
        }

        catch_all([&](std::string* msg) {
                *msg = "Export to file";

                QString caption = "Export " + QString(name.c_str()) + " to OBJ";
                QString filter = file_filter("OBJ files", m_objects->obj_extension(m_dimension));
                QFileDialog::Options options = QFileDialog::DontUseNativeDialog;

                QString qt_file_name = QFileDialog::getSaveFileName(this, caption, "", filter, nullptr, options);
                if (qt_file_name.size() == 0)
                {
                        return;
                }

                std::string file_name = qt_file_name.toStdString();

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

        if (!m_threads.action_allowed(ThreadAction::ReloadBoundCocone))
        {
                m_event_emitter.message_warning("BOUND COCONE is not available at this time (thread working)");
                return;
        }

        if (!m_objects->manifold_constructor_exists())
        {
                m_event_emitter.message_warning("No manifold constructor");
                return;
        }

        double rho = m_bound_cocone_rho;
        double alpha = m_bound_cocone_alpha;

        catch_all([&](std::string* msg) {
                *msg = "Reload BoundCocone";

                if (!BoundCoconeParameters(this).show(BOUND_COCONE_MINIMUM_RHO_EXPONENT, BOUND_COCONE_MINIMUM_ALPHA_EXPONENT,
                                                      &rho, &alpha))
                {
                        return;
                }

                m_threads.start_thread(ThreadAction::ReloadBoundCocone,
                                       [=](ProgressRatioList* progress_list, std::string* message) {
                                               *message = "BOUND COCONE reconstruction";

                                               m_objects->compute_bound_cocone(progress_list, rho, alpha);
                                       });
        });
}

void MainWindow::progress_bars(bool permanent, const ProgressRatioList* progress_list, std::list<QProgressBar>* progress_bars)
{
        static_assert(std::numeric_limits<unsigned>::max() >= std::numeric_limits<int>::max());

        constexpr unsigned MAX_INT = std::numeric_limits<int>::max();

        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios = progress_list->ratios();

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
                progress_bars(t.permanent, t.progress_list, t.progress_bars);
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
        strike_out_radio_button(ui.radioButton_ModelMST);
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
        switch (event.type())
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

                switch (static_cast<ObjectIdentifier>(d.id))
                {
                case OBJECT_MODEL:
                        enable_radio_button(ui.radioButton_Model);
                        break;
                case OBJECT_MODEL_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_ModelConvexHull);
                        break;
                case OBJECT_MODEL_MST:
                        enable_radio_button(ui.radioButton_ModelMST);
                        break;
                case OBJECT_COCONE:
                        enable_radio_button(ui.radioButton_Cocone);
                        break;
                case OBJECT_COCONE_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_CoconeConvexHull);
                        break;
                case OBJECT_BOUND_COCONE:
                        enable_radio_button(ui.radioButton_BoundCocone);
                        break;
                case OBJECT_BOUND_COCONE_CONVEX_HULL:
                        enable_radio_button(ui.radioButton_BoundCoconeConvexHull);
                        break;
                }

                break;
        }
        case WindowEvent::EventType::FILE_LOADED:
        {
                const WindowEvent::file_loaded& d = event.get<WindowEvent::file_loaded>();

                std::string file_name = file_base_name(d.file_name);
                set_window_title_file(file_name + " [" + space_name(d.dimension) + "]");
                strike_out_all_objects_buttons();
                ui.radioButton_Model->setChecked(true);
                m_dimension = d.dimension;

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

        thread_self_test(SelfTestType::Essential);

        try
        {
                m_show = create_show(&m_event_emitter, widget_window_id(ui.graphics_widget), qcolor_to_rgb(m_background_color),
                                     qcolor_to_rgb(m_default_color), qcolor_to_rgb(m_wireframe_color),
                                     ui.checkBox_Smooth->isChecked(), ui.checkBox_Wireframe->isChecked(),
                                     ui.checkBox_Shadow->isChecked(), ui.checkBox_Materials->isChecked(),
                                     ui.checkBox_ShowEffect->isChecked(), ui.checkBox_show_dft->isChecked(),
                                     ui.checkBox_convex_hull_2d->isChecked(), ui.checkBox_OpticalFlow->isChecked(),
                                     ambient_light(), diffuse_light(), specular_light(), dft_brightness(), default_ns(),
                                     ui.checkBox_VerticalSync->isChecked(), shadow_zoom());

                m_objects->set_show(m_show.get());
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
                thread_load_from_file(QCoreApplication::arguments().at(1).toStdString());
        }
}

void MainWindow::on_actionLoad_triggered()
{
        thread_load_from_file();
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
        int object_id = 0;

        bool found = false;
        for (const auto & [ button, id ] : m_object_buttons)
        {
                if (button->isChecked())
                {
                        object_name = button->text().toStdString();
                        object_id = id;
                        found = true;
                        break;
                }
        }

        if (!found)
        {
                m_event_emitter.message_warning("Select object button to export");
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
        application_help(this);
}

void MainWindow::on_actionSelfTest_triggered()
{
        thread_self_test(SelfTestType::Extended);
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

double MainWindow::ambient_light() const
{
        double value = ui.Slider_Ambient->value() - ui.Slider_Ambient->minimum();
        double delta = ui.Slider_Ambient->maximum() - ui.Slider_Ambient->minimum();
        return 2 * value / delta;
}
double MainWindow::diffuse_light() const
{
        double value = ui.Slider_Diffuse->value() - ui.Slider_Diffuse->minimum();
        double delta = ui.Slider_Diffuse->maximum() - ui.Slider_Diffuse->minimum();
        return 2 * value / delta;
}
double MainWindow::specular_light() const
{
        double value = ui.Slider_Specular->value() - ui.Slider_Specular->minimum();
        double delta = ui.Slider_Specular->maximum() - ui.Slider_Specular->minimum();
        return 2 * value / delta;
}
double MainWindow::dft_brightness() const
{
        double value = ui.Slider_DFT_Brightness->value() - ui.Slider_DFT_Brightness->minimum();
        double delta = ui.Slider_DFT_Brightness->maximum() - ui.Slider_DFT_Brightness->minimum();
        double value_gamma = std::pow(value / delta, DFT_GAMMA);
        return std::pow(DFT_MAX_BRIGHTNESS, value_gamma);
}
double MainWindow::default_ns() const
{
        return ui.Slider_Default_Ns->value();
}
double MainWindow::shadow_zoom() const
{
        return ui.Slider_ShadowQuality->value();
}

void MainWindow::on_Slider_Ambient_valueChanged(int)
{
        m_show->set_ambient(ambient_light());
}

void MainWindow::on_Slider_Diffuse_valueChanged(int)
{
        m_show->set_diffuse(diffuse_light());
}

void MainWindow::on_Slider_Specular_valueChanged(int)
{
        m_show->set_specular(specular_light());
}

void MainWindow::on_Slider_DFT_Brightness_valueChanged(int)
{
        m_show->set_dft_brightness(dft_brightness());
}

void MainWindow::on_Slider_Default_Ns_valueChanged(int)
{
        m_show->set_default_ns(default_ns());
}

void MainWindow::on_Slider_ShadowQuality_valueChanged(int)
{
        if (m_show)
        {
                m_show->set_shadow_zoom(shadow_zoom());
        }
}

void MainWindow::on_ButtonBackgroundColor_clicked()
{
        color_dialog(this, "Background color", m_background_color, [this](const QColor& c) { set_background_color(c); });
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
        m_show->show_object(OBJECT_MODEL);
}

void MainWindow::on_radioButton_ModelConvexHull_clicked()
{
        m_show->show_object(OBJECT_MODEL_CONVEX_HULL);
}

void MainWindow::on_radioButton_ModelMST_clicked()
{
        m_show->show_object(OBJECT_MODEL_MST);
}

void MainWindow::on_radioButton_Cocone_clicked()
{
        m_show->show_object(OBJECT_COCONE);
}

void MainWindow::on_radioButton_CoconeConvexHull_clicked()
{
        m_show->show_object(OBJECT_COCONE_CONVEX_HULL);
}

void MainWindow::on_radioButton_BoundCocone_clicked()
{
        m_show->show_object(OBJECT_BOUND_COCONE);
}

void MainWindow::on_radioButton_BoundCoconeConvexHull_clicked()
{
        m_show->show_object(OBJECT_BOUND_COCONE_CONVEX_HULL);
}

void MainWindow::on_pushButton_Painter_clicked()
{
        std::string model_name;
        int id = -1;

        bool found = false;
        for (const auto & [ button, button_model_id ] : m_object_buttons)
        {
                if (button->isChecked())
                {
                        model_name = button->text().toStdString();
                        id = button_model_id;
                        found = true;
                        break;
                }
        }
        if (!found)
        {
                m_event_emitter.message_warning("No object button is checked");
                return;
        }

        if (!m_objects->mesh_exists(id))
        {
                m_event_emitter.message_warning("No object to paint");
                return;
        }

        catch_all([&](std::string* message) {
                *message = "Painter";

                PaintingInformation3d info_3d;
                m_show->camera_information(&info_3d.camera_up, &info_3d.camera_direction, &info_3d.view_center,
                                           &info_3d.view_width);
                info_3d.object_position = m_show->object_position();
                info_3d.light_direction = m_show->light_direction();
                info_3d.object_size = m_show->object_size();
                info_3d.max_screen_size = PATH_TRACING_3D_MAX_SCREEN_SIZE;
                m_show->paint_width_height(&info_3d.paint_width, &info_3d.paint_height);

                PaintingInformationNd info_nd;
                info_nd.default_screen_size = PATH_TRACING_DEFAULT_SCREEN_SIZE;
                info_nd.minimum_screen_size = PATH_TRACING_MINIMUM_SCREEN_SIZE;
                info_nd.maximum_screen_size = PATH_TRACING_MAXIMUM_SCREEN_SIZE;

                PaintingInformationAll info_all;
                info_all.parent_window = this;
                info_all.window_title = QMainWindow::windowTitle().toStdString();
                info_all.model_name = model_name;
                info_all.default_samples_per_pixel = PATH_TRACING_DEFAULT_SAMPLES_PER_PIXEL;
                info_all.max_samples_per_pixel = PATH_TRACING_MAX_SAMPLES_PER_PIXEL;
                info_all.background_color = qcolor_to_rgb(m_background_color);
                info_all.default_color = qcolor_to_rgb(m_default_color);
                info_all.diffuse = diffuse_light();

                m_objects->paint(id, info_3d, info_nd, info_all);

        });
}
