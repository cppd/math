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

#include "graphics_widget.h"
#include "model_events.h"
#include "model_tree.h"
#include "range_slider.h"
#include "repository_actions.h"
#include "threads.h"

#include "ui_main_window.h"

#include <src/progress/progress_list.h>
#include <src/storage/repository.h>
#include <src/test/self_test.h>

#include <QColor>
#include <QTimer>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gui
{
class MainWindow final : public QMainWindow
{
        Q_OBJECT

public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        void append_to_log(const std::string& text, const Srgb8& color);

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
        void on_checkBox_clip_plane_clicked();
        void on_checkBox_convex_hull_2d_clicked();
        void on_checkBox_dft_clicked();
        void on_checkBox_fog_clicked();
        void on_checkBox_fps_clicked();
        void on_checkBox_isosurface_clicked();
        void on_checkBox_materials_clicked();
        void on_checkBox_normals_clicked();
        void on_checkBox_optical_flow_clicked();
        void on_checkBox_pencil_sketch_clicked();
        void on_checkBox_shadow_clicked();
        void on_checkBox_smooth_clicked();
        void on_checkBox_vertical_sync_clicked();
        void on_checkBox_wireframe_clicked();
        void on_pushButton_reset_lighting_clicked();
        void on_pushButton_reset_view_clicked();
        void on_slider_ambient_valueChanged(int value);
        void on_slider_clip_plane_valueChanged(int value);
        void on_slider_default_ns_valueChanged(int value);
        void on_slider_dft_brightness_valueChanged(int value);
        void on_slider_diffuse_valueChanged(int value);
        void on_slider_isovalue_valueChanged(int value);
        void on_slider_normals_valueChanged(int value);
        void on_slider_shadow_quality_valueChanged(int value);
        void on_slider_specular_valueChanged(int value);
        void on_slider_volume_transparency_valueChanged(int value);
        void on_toolButton_background_color_clicked();
        void on_toolButton_clip_plane_color_clicked();
        void on_toolButton_default_color_clicked();
        void on_toolButton_dft_background_color_clicked();
        void on_toolButton_dft_color_clicked();
        void on_toolButton_normal_color_negative_clicked();
        void on_toolButton_normal_color_positive_clicked();
        void on_toolButton_wireframe_color_clicked();

private:
        void graphics_widget_resize(QResizeEvent*);
        void graphics_widget_mouse_wheel(QWheelEvent*);
        void graphics_widget_mouse_move(QMouseEvent*);
        void graphics_widget_mouse_press(QMouseEvent*);
        void graphics_widget_mouse_release(QMouseEvent*);

        void action_mesh_repository(int dimension, std::string object_name);
        void action_volume_repository(int dimension, std::string object_name);

        void model_tree_item_changed();
        void slider_volume_levels_range_changed(double, double);
        void timer_progress_bar();
        void first_shown();

        //

        void constructor_graphics_widget();
        void constructor_interface();
        void constructor_objects();

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void terminate_all_threads();

        //

        void load_from_file(std::string file_name, bool use_object_selection_dialog);
        void self_test(test::SelfTestType test_type, bool with_confirmation);

        //

        void set_dependent_interface();

        void progress_bars(
                WorkerThreads::Action action,
                bool permanent,
                const ProgressRatioList* progress_list,
                std::list<QProgressBar>* progress_bars);

        double ambient_light() const;
        double diffuse_light() const;
        double specular_light() const;
        double default_ns() const;
        double dft_brightness() const;
        double shadow_zoom() const;
        double normal_length() const;

        static double lighting_slider_value(const QSlider* slider);

        void set_background_color(const QColor& c);
        void set_default_color(const QColor& c);
        void set_default_specular_color(const QColor& c);
        void set_wireframe_color(const QColor& c);
        void set_clip_plane_color(const QColor& c);
        void set_normal_color_positive(const QColor& c);
        void set_normal_color_negative(const QColor& c);

        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

        bool stop_action(WorkerThreads::Action action);

        void update_mesh_ui(ObjectId id);

        void disable_volume_parameters();
        void update_volume_ui(ObjectId id);

        const std::thread::id m_thread_id;

        Ui::MainWindow ui;

        std::unique_ptr<WorkerThreads> m_worker_threads;

        std::unique_ptr<ModelEvents> m_mesh_and_volume_events;

        GraphicsWidget* m_graphics_widget;
        std::unique_ptr<view::View> m_view;

        std::unique_ptr<storage::Repository> m_repository;
        std::unique_ptr<RepositoryActions> m_repository_actions;

        std::unique_ptr<ModelTree> m_model_tree;
        std::unique_ptr<RangeSlider> m_slider_volume_levels;

        QColor m_background_color;
        QColor m_default_color;
        QColor m_default_specular_color;
        QColor m_wireframe_color;
        QColor m_clip_plane_color;
        QColor m_normal_color_positive;
        QColor m_normal_color_negative;
        QColor m_dft_background_color;
        QColor m_dft_color;

        bool m_first_show;

        QTimer m_timer_progress_bar;
};
}
