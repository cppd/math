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

#pragma once
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ui_main_window.h"

#include "main_window_event.h"

#include "com/thread.h"
#include "geom/convex_hull_types.h"
#include "progress/progress_list.h"
#include "show/show.h"

#include <QColor>
#include <QProgressBar>
#include <QTimer>
#include <array>
#include <memory>
#include <mutex>
#include <string>

template <size_t N>
struct ISurfaceReconstructor;

class MainWindow final : public QMainWindow, public ICallBack
{
        Q_OBJECT

public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow();

protected:
        void showEvent(QShowEvent* e) override;
        void resizeEvent(QResizeEvent*) override;

signals:
        void SignalWindowEvent(const WindowEvent&) const;

public slots:

private slots:
        void on_actionLoad_triggered();
        void on_actionExit_triggered();
        void on_actionAbout_triggered();
        void on_MainWindow_SignalWindowEvent(const WindowEvent&);
        void on_Button_ResetView_clicked();
        void on_Slider_Ambient_valueChanged(int value);
        void on_Slider_Diffuse_valueChanged(int value);
        void on_Slider_Specular_valueChanged(int value);
        void on_Slider_DFT_Brightness_valueChanged(int value);
        void on_ButtonBackgroundColor_clicked();
        void on_ButtonDefaultColor_clicked();
        void on_checkBox_Wireframe_clicked();
        void on_checkBox_Materials_clicked();
        void on_checkBox_Shadow_clicked();
        void on_checkBox_Smooth_clicked();
        void on_ButtonWireframeColor_clicked();
        void on_checkBox_ShowEffect_clicked();
        void on_checkBox_show_dft_clicked();
        void on_checkBox_convex_hull_2d_clicked();
        void on_actionFullScreen_triggered();
        void on_radioButton_Faces_clicked();
        void on_radioButton_FacesConvexHull_clicked();
        void on_radioButton_Cocone_clicked();
        void on_radioButton_CoconeConvexHull_clicked();
        void on_radioButton_BoundCocone_clicked();
        void on_radioButton_BoundCoconeConvexHull_clicked();
        void on_Button_LoadBoundCocone_clicked();
        void on_actionKeyboard_and_Mouse_triggered();
        void on_Slider_Default_Ns_valueChanged(int value);

        void timer_slot();
        void window_shown();

private:
        void set_dependent_interface_enabled();

        static void disable_radio_button(QRadioButton* button);
        static void enable_radio_button(QRadioButton* button);
        void disable_object_buttons();
        void disable_bound_cocone_buttons();

        void thread_open_file(const std::string& file_name, ConvexHullComputationType convex_hull_type) noexcept;
        void thread_faces(std::shared_ptr<IObj> obj, ConvexHullComputationType convex_hull_type) noexcept;
        void thread_surface_reconstructor(ConvexHullComputationType convex_hull_type) noexcept;
        void thread_cocone(ConvexHullComputationType convex_hull_type) noexcept;
        void thread_bound_cocone(ConvexHullComputationType convex_hull_type, double rho, double alpha) noexcept;

        void start_thread_open_file(const std::string& file_name);
        void stop_all_threads();

        void error_message(const std::string&) noexcept override;
        void window_ready() noexcept override;
        void program_ended(const std::string&) noexcept override;
        void error_src_message(const std::string&, const std::string&) noexcept override;
        void object_loaded(int) noexcept override;
        void file_loaded(const std::string& msg /*, ConvexHullComputationType convex_hull_type*/) noexcept;
        void bound_cocone_loaded(double rho, double alpha) noexcept;

        double get_ambient() const;
        double get_diffuse() const;
        double get_specular() const;
        double get_dft_brightness() const;
        double get_default_ns() const;

        ConvexHullComputationType get_ch_type() const;
        void init_convex_hull_type(ConvexHullComputationType);

        void set_bound_cocone_label(double rho, double alpha);

        Ui::MainWindow ui;

        std::unique_ptr<IShow> m_show;

        QColor m_clear_color;
        QColor m_default_color;
        QColor m_wireframe_color;

        QTimer m_timer;

        ProgressRatioList m_progress_ratio_list;
        std::list<QProgressBar> m_progress_bars;

        QWidget* m_widget_under_window;

        bool m_first_show;

        std::thread m_open_file_thread;
        std::thread m_bound_cocone_thread;

        QLabel m_convex_hull_type_label;

        std::vector<glm::vec3> m_points;
        std::unique_ptr<ISurfaceReconstructor<3>> m_surface_reconstructor;

        std::atomic_bool m_file_loading;
        std::atomic_bool m_bound_cocone_loading;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;
};

#endif
