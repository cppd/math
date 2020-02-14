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

#pragma once

#include "event_emitter.h"
#include "objects.h"
#include "threads.h"

#include "ui_main_window.h"

#include <src/progress/progress_list.h>
#include <src/show/interface.h>
#include <src/test/self_test.h>

#include <QColor>
#include <QTimer>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class MainWindow final : public QMainWindow, public DirectEvents
{
        Q_OBJECT

public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

public slots:

private slots:
        void on_actionAbout_triggered();
        void on_actionBoundCocone_triggered();
        void on_actionExit_triggered();
        void on_actionExport_triggered();
        void on_actionFullScreen_triggered();
        void on_actionHelp_triggered();
        void on_actionLoad_triggered();
        void on_actionPainter_triggered();
        void on_actionSelfTest_triggered();
        void on_checkBox_convex_hull_2d_clicked();
        void on_checkBox_dft_clicked();
        void on_checkBox_fog_clicked();
        void on_checkBox_fps_clicked();
        void on_checkBox_materials_clicked();
        void on_checkBox_optical_flow_clicked();
        void on_checkBox_pencil_sketch_clicked();
        void on_checkBox_shadow_clicked();
        void on_checkBox_smooth_clicked();
        void on_checkBox_vertical_sync_clicked();
        void on_checkBox_wireframe_clicked();
        void on_checkBox_clip_plane_clicked();
        void on_pushButton_reset_lighting_clicked();
        void on_pushButton_reset_view_clicked();
        void on_radioButton_bound_cocone_clicked();
        void on_radioButton_bound_cocone_convex_hull_clicked();
        void on_radioButton_cocone_clicked();
        void on_radioButton_cocone_convex_hull_clicked();
        void on_radioButton_model_clicked();
        void on_radioButton_model_convex_hull_clicked();
        void on_radioButton_model_mst_clicked();
        void on_slider_ambient_valueChanged(int value);
        void on_slider_default_ns_valueChanged(int value);
        void on_slider_dft_brightness_valueChanged(int value);
        void on_slider_diffuse_valueChanged(int value);
        void on_slider_shadow_quality_valueChanged(int value);
        void on_slider_specular_valueChanged(int value);
        void on_slider_clip_plane_valueChanged(int value);
        void on_toolButton_background_color_clicked();
        void on_toolButton_default_color_clicked();
        void on_toolButton_dft_background_color_clicked();
        void on_toolButton_dft_color_clicked();
        void on_toolButton_wireframe_color_clicked();
        void on_toolButton_clip_plane_color_clicked();

        void graphics_widget_resize(QResizeEvent*);
        void graphics_widget_mouse_wheel(QWheelEvent*);
        void graphics_widget_mouse_move(QMouseEvent*);
        void graphics_widget_mouse_press(QMouseEvent*);
        void graphics_widget_mouse_release(QMouseEvent*);

        void slot_object_repository();
        void slot_timer_progress_bar();
        void slot_window_first_shown();

private:
        void constructor_threads();
        void constructor_connect();
        void constructor_interface();
        void constructor_buttons();
        void constructor_objects_and_repository();

        static std::unordered_set<ObjectId> default_objects_to_load();

        void set_window_title_file(const std::string& file_name);

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void close_without_confirmation();

        void terminate_all_threads();

        template <typename F>
        void catch_all(const F& function) const noexcept;

        bool find_object(std::string* object_name, ObjectId* object_id);
        QRadioButton* object_id_to_button(ObjectId id);

        void thread_load_from_file(std::string file_name, bool use_object_selection_dialog);
        void thread_load_from_repository(int dimension, const std::string& object_name);
        void thread_self_test(SelfTestType test_type, bool with_confirmation);
        void thread_export(const std::string& name, ObjectId id);
        void thread_reload_bound_cocone();

        void set_dependent_interface();

        static void reset_object_button(QRadioButton* button, bool object_to_load);
        static void show_object_button(QRadioButton* button);
        void reset_all_object_buttons(const std::unordered_set<ObjectId>& objects_to_load);
        void reset_bound_cocone_buttons(const std::unordered_set<ObjectId>& objects_to_load);

        void progress_bars(
                MainThreads::Action thread_action,
                bool permanent,
                const ProgressRatioList* progress_list,
                std::list<QProgressBar>* progress_bars);

        double ambient_light() const;
        double diffuse_light() const;
        double specular_light() const;
        double default_ns() const;
        double dft_brightness() const;
        double shadow_zoom() const;

        static double lighting_slider_value(const QSlider* slider);

        void set_bound_cocone_parameters(double rho, double alpha);

        void set_background_color(const QColor& c);
        void set_default_color(const QColor& c);
        void set_wireframe_color(const QColor& c);
        void set_clip_plane_color(const QColor& c);

        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

        void exception_handler(const std::exception_ptr& ptr, const std::string& msg, bool window_exists) const
                noexcept;

        static bool dialog_object_selection(QWidget* parent, std::unordered_set<ObjectId>* objects_to_load);

        void direct_message_error(const std::string& msg) override;
        void direct_message_error_fatal(const std::string& msg) override;
        void direct_message_error_source(const std::string& msg, const std::string& src) override;
        void direct_message_information(const std::string& msg) override;
        void direct_message_warning(const std::string& msg) override;
        void direct_object_loaded(int id) override;
        void direct_mesh_loaded(ObjectId id) override;
        void direct_file_loaded(
                const std::string& file_name,
                unsigned dimension,
                const std::unordered_set<ObjectId>& objects) override;
        void direct_bound_cocone_loaded(double rho, double alpha) override;
        void direct_log(const std::string& msg) override;

        Ui::MainWindow ui;

        const std::thread::id m_window_thread_id;

        WindowEventEmitter m_event_emitter;

        std::unique_ptr<MainThreads> m_threads;

        std::unordered_map<ObjectId, QRadioButton*> m_object_id_to_button;

        std::unordered_map<QObject*, std::tuple<int, std::string>> m_action_to_dimension_and_object_name;

        std::unique_ptr<ShowObject> m_show_object;
        Show* m_show = nullptr;

        std::unique_ptr<MainObjects> m_objects;

        QColor m_background_color;
        QColor m_default_color;
        QColor m_wireframe_color;
        QColor m_clip_plane_color;

        QColor m_dft_background_color;
        QColor m_dft_color;

        bool m_first_show;

        QTimer m_timer_progress_bar;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;

        unsigned m_dimension;

        bool m_close_without_confirmation;

        std::unordered_set<ObjectId> m_objects_to_load;
};
