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

private:
        void on_about_triggered();
        void on_background_color_clicked();
        void on_bound_cocone_triggered();
        void on_clip_plane_changed(int value);
        void on_clip_plane_clicked();
        void on_clip_plane_color_clicked();
        void on_convex_hull_2d_clicked();
        void on_dft_background_color_clicked();
        void on_dft_brightness_changed(int value);
        void on_dft_clicked();
        void on_dft_color_clicked();
        void on_exit_triggered();
        void on_export_triggered();
        void on_fog_clicked();
        void on_fps_clicked();
        void on_full_screen_triggered();
        void on_help_triggered();
        void on_isosurface_clicked();
        void on_isosurface_transparency_changed(int value);
        void on_isovalue_changed(int value);
        void on_lighting_intensity_changed(int value);
        void on_load_triggered();
        void on_materials_clicked();
        void on_mesh_ambient_changed(int value);
        void on_mesh_color_clicked();
        void on_mesh_diffuse_changed(int value);
        void on_mesh_specular_changed(int value);
        void on_mesh_specular_power_changed(int value);
        void on_mesh_transparency_changed(int value);
        void on_normal_color_negative_clicked();
        void on_normal_color_positive_clicked();
        void on_normals_changed(int value);
        void on_normals_clicked();
        void on_optical_flow_clicked();
        void on_painter_triggered();
        void on_pencil_sketch_clicked();
        void on_reset_lighting_clicked();
        void on_reset_view_clicked();
        void on_self_test_triggered();
        void on_shadow_clicked();
        void on_shadow_quality_changed(int value);
        void on_smooth_clicked();
        void on_vertical_sync_clicked();
        void on_volume_ambient_changed(int value);
        void on_volume_color_clicked();
        void on_volume_diffuse_changed(int value);
        void on_volume_specular_changed(int value);
        void on_volume_specular_power_changed(int value);
        void on_volume_transparency_changed(int value);
        void on_wireframe_clicked();
        void on_wireframe_color_clicked();

        void on_graphics_widget_resize(QResizeEvent*);
        void on_graphics_widget_mouse_wheel(QWheelEvent*);
        void on_graphics_widget_mouse_move(QMouseEvent*);
        void on_graphics_widget_mouse_press(QMouseEvent*);
        void on_graphics_widget_mouse_release(QMouseEvent*);

        void on_repository_mesh(int dimension, const std::string& object_name);
        void on_repository_volume(int dimension, const std::string& object_name);

        void on_model_tree_item_changed();
        void on_slider_volume_levels_changed(double, double);
        void on_timer_progress_bar();
        void on_first_shown();

        //

        void constructor_connect();
        void constructor_graphics_widget();
        void constructor_interface();
        void constructor_objects();

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void terminate_all_threads();

        //

        void load_from_file(const std::string& file_name, bool use_object_selection_dialog);
        void self_test(test::SelfTestType test_type, bool with_confirmation);

        //

        void set_dependent_interface();

        void progress_bars(
                WorkerThreads::Action action,
                bool permanent,
                const ProgressRatioList* progress_list,
                std::list<QProgressBar>* progress_bars);

        double lighting_intensity() const;
        double dft_brightness() const;
        double shadow_zoom() const;
        double normal_length() const;

        void set_background_color(const QColor& c);
        void set_specular_color(const QColor& c);
        void set_wireframe_color(const QColor& c);
        void set_clip_plane_color(const QColor& c);
        void set_normal_color_positive(const QColor& c);
        void set_normal_color_negative(const QColor& c);

        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

        bool stop_action(WorkerThreads::Action action);

        void disable_mesh_parameters();
        void update_mesh_ui(ObjectId id);

        void disable_volume_parameters();
        void update_volume_ui(ObjectId id);

        const std::thread::id m_thread_id = std::this_thread::get_id();
        bool m_first_show = true;

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
        QColor m_specular_color;
        QColor m_wireframe_color;
        QColor m_clip_plane_color;
        QColor m_normal_color_positive;
        QColor m_normal_color_negative;
        QColor m_dft_background_color;
        QColor m_dft_color;

        QTimer m_timer_progress_bar;
};
}
