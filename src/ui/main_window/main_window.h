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

#pragma once

#include "event_emitter.h"
#include "objects.h"
#include "threads.h"

#include "progress/progress_list.h"
#include "show/show.h"
#include "tests/self_test.h"

#include "ui_main_window.h"

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

class MainWindow final : public QMainWindow
{
        Q_OBJECT

public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

public slots:

private slots:
        void on_actionLoad_triggered();
        void on_actionExport_triggered();
        void on_actionExit_triggered();
        void on_actionBoundCocone_triggered();
        void on_actionPainter_triggered();
        void on_actionHelp_triggered();
        void on_actionSelfTest_triggered();
        void on_actionAbout_triggered();
        void on_actionFullScreen_triggered();
        void on_slider_Ambient_valueChanged(int value);
        void on_slider_Diffuse_valueChanged(int value);
        void on_slider_Specular_valueChanged(int value);
        void on_slider_DFT_Brightness_valueChanged(int value);
        void on_slider_Default_Ns_valueChanged(int value);
        void on_slider_ShadowQuality_valueChanged(int value);
        void on_toolButton_BackgroundColor_clicked();
        void on_toolButton_DefaultColor_clicked();
        void on_toolButton_WireframeColor_clicked();
        void on_toolButton_dft_background_color_clicked();
        void on_toolButton_dft_color_clicked();
        void on_pushButton_ResetLighting_clicked();
        void on_pushButton_ResetView_clicked();
        void on_checkBox_Wireframe_clicked();
        void on_checkBox_Materials_clicked();
        void on_checkBox_Shadow_clicked();
        void on_checkBox_Fog_clicked();
        void on_checkBox_Smooth_clicked();
        void on_checkBox_ShowEffect_clicked();
        void on_checkBox_show_dft_clicked();
        void on_checkBox_convex_hull_2d_clicked();
        void on_checkBox_OpticalFlow_clicked();
        void on_checkBox_VerticalSync_clicked();
        void on_radioButton_Model_clicked();
        void on_radioButton_ModelConvexHull_clicked();
        void on_radioButton_ModelMST_clicked();
        void on_radioButton_Cocone_clicked();
        void on_radioButton_CoconeConvexHull_clicked();
        void on_radioButton_BoundCocone_clicked();
        void on_radioButton_BoundCoconeConvexHull_clicked();

        void slot_object_repository();
        void slot_window_event(const WindowEvent&);
        void slot_timer_progress_bar();
        void slot_window_first_shown();
        void slot_widget_under_window_mouse_wheel(double delta);
        void slot_widget_under_window_resize();

private:
        void constructor_connect();
        void constructor_interface();
        void constructor_repository();
        void constructor_buttons();
        void constructor_objects();

        void set_window_title_file(const std::string& file_name);

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void close_without_confirmation();

        void stop_all_threads();

        template <typename F>
        void catch_all(const F& function) const noexcept;

        bool find_object(std::string* object_name, ObjectId* object_id);

        void thread_load_from_file(std::string file_name, bool use_object_selection_dialog);
        void thread_load_from_repository(const std::tuple<int, std::string>& object);
        void thread_self_test(SelfTestType test_type, bool with_confirmation);
        void thread_export(const std::string& name, ObjectId id);
        void thread_reload_bound_cocone();

        void set_dependent_interface();

        static void strike_out_radio_button(QRadioButton* button);
        static void enable_radio_button(QRadioButton* button);
        void strike_out_all_objects_buttons();
        void strike_out_bound_cocone_buttons();

        void progress_bars(bool permanent, const ProgressRatioList* progress_list, std::list<QProgressBar>* progress_bars);

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

        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

        void exception_handler(const std::exception_ptr& ptr, const std::string& msg, bool window_exists) const noexcept;

        static bool dialog_object_selection(QWidget* parent, std::unordered_set<ObjectId>* objects_to_load);

        Ui::MainWindow ui;

        const std::thread::id m_window_thread_id;

        WindowEventEmitter m_event_emitter;

        Threads m_threads;

        std::vector<std::tuple<const QRadioButton*, ObjectId>> m_object_buttons;

        std::unordered_map<QObject*, std::tuple<int, std::string>> m_action_to_object_name_map;

        std::unique_ptr<IShow> m_show;

        std::unique_ptr<MainObjects> m_objects;

        QColor m_background_color;
        QColor m_default_color;
        QColor m_wireframe_color;

        QColor m_dft_background_color;
        QColor m_dft_color;

        bool m_first_show = true;

        QTimer m_timer_progress_bar;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;

        unsigned m_dimension = 0;

        bool m_close_without_confirmation = false;

        std::unordered_set<ObjectId> m_objects_to_load;
};
