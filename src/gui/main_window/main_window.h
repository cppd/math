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

#include "event.h"
#include "threads.h"

#include "ui_main_window.h"

#include <src/com/sequence.h>
#include <src/painter/shapes/mesh.h>
#include <src/progress/progress_list.h>
#include <src/settings/dimensions.h>
#include <src/storage/multi_repository.h>
#include <src/storage/multi_storage.h>
#include <src/test/self_test.h>
#include <src/view/interface.h>

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

signals:
        void window_signal(const std::function<void()>&) const;
private slots:
        void window_slot(const std::function<void()>&) const;

private:
        void run_in_window_thread(const std::function<void()>&) const;

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
        void on_slider_normals_valueChanged(int value);
        void on_slider_shadow_quality_valueChanged(int value);
        void on_slider_specular_valueChanged(int value);
        void on_slider_volume_levels_range_changed(double, double);
        void on_toolButton_background_color_clicked();
        void on_toolButton_clip_plane_color_clicked();
        void on_toolButton_default_color_clicked();
        void on_toolButton_dft_background_color_clicked();
        void on_toolButton_dft_color_clicked();
        void on_toolButton_normal_color_negative_clicked();
        void on_toolButton_normal_color_positive_clicked();
        void on_toolButton_wireframe_color_clicked();

        void graphics_widget_resize(QResizeEvent*);
        void graphics_widget_mouse_wheel(QWheelEvent*);
        void graphics_widget_mouse_move(QMouseEvent*);
        void graphics_widget_mouse_press(QMouseEvent*);
        void graphics_widget_mouse_release(QMouseEvent*);

        void model_tree_item_changed();

        void slot_mesh_object_repository();
        void slot_volume_object_repository();
        void slot_timer_progress_bar();
        void slot_window_first_shown();

private:
        void constructor_threads();
        void constructor_connect();
        void constructor_interface();
        void constructor_objects_and_repository();
        void constructor_model_events();
        void delete_model_events();

        enum class ComputationType
        {
                Mst,
                ConvexHull,
                Cocone,
                BoundCocone
        };

        void set_window_title_file(const std::string& file_name);

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void close_without_confirmation();

        void terminate_all_threads();

        template <typename F>
        void catch_all(const F& function) const noexcept;

        //

        void thread_load_from_file(std::string file_name, bool use_object_selection_dialog);
        void thread_load_from_mesh_repository();
        void thread_load_from_volume_repository();

        template <size_t N>
        std::optional<WorkerThreads::Function> export_function(
                const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object);
        void thread_export();

        void thread_bound_cocone();
        void thread_self_test(SelfTestType test_type, bool with_confirmation);

        template <size_t N>
        std::optional<WorkerThreads::Function> painter_function(
                const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object);
        void thread_painter();

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
        void set_wireframe_color(const QColor& c);
        void set_clip_plane_color(const QColor& c);
        void set_normal_color_positive(const QColor& c);
        void set_normal_color_negative(const QColor& c);

        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

        void exception_handler(const std::exception_ptr& ptr, const std::string& msg, bool window_exists)
                const noexcept;

        bool dialog_object_selection(std::unordered_set<ComputationType>* objects_to_load);

        bool stop_action(WorkerThreads::Action action);

        void event_from_window(const WindowEvent& event);
        void event_from_log(const LogEvent& event);
        void event_from_view(const view::Event& event);
        template <size_t N>
        void event_from_mesh([[maybe_unused]] const mesh::MeshEvent<N>& event);
        template <size_t N>
        void event_from_mesh_window_thread(const mesh::MeshEvent<N>& event);
        template <size_t N>
        void event_from_volume([[maybe_unused]] const volume::VolumeEvent<N>& event);
        template <size_t N>
        void event_from_volume_window_thread(const volume::VolumeEvent<N>& event);

        void update_volume_ui(ObjectId id);

        Ui::MainWindow ui;

        const std::thread::id m_window_thread_id;

        std::function<void(WindowEvent&&)> m_window_events;

        std::unique_ptr<WorkerThreads> m_worker_threads;

        struct RepositoryActionDescription final
        {
                int dimension;
                std::string object_name;
        };
        std::unordered_map<QObject*, RepositoryActionDescription> m_repository_actions;

        std::unique_ptr<view::View> m_view;

        std::unique_ptr<storage::MultiRepository> m_repository;
        std::unique_ptr<storage::MultiStorage> m_storage;

        template <size_t N>
        struct ModelEvents
        {
                std::function<void(mesh::MeshEvent<N>&&)> mesh_events;
                std::function<void(volume::VolumeEvent<N>&&)> volume_events;
        };
        Sequence<settings::Dimensions, std::tuple, ModelEvents> m_model_events;

        QColor m_background_color;
        QColor m_default_color;
        QColor m_wireframe_color;
        QColor m_clip_plane_color;
        QColor m_normal_color_positive;
        QColor m_normal_color_negative;

        QColor m_dft_background_color;
        QColor m_dft_color;

        bool m_first_show;

        QTimer m_timer_progress_bar;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;

        bool m_close_without_confirmation;

        std::unordered_set<ComputationType> m_objects_to_load;

        const int m_mesh_threads;
};
