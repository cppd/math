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
#include "threads.h"

#include "ui_main_window.h"

#include <src/progress/progress_list.h>
#include <src/show/interface.h>
#include <src/storage/manage.h>
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

class MainWindow final : public QMainWindow, public AllEvents
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
        void on_checkBox_normals_clicked();
        void on_pushButton_reset_lighting_clicked();
        void on_pushButton_reset_view_clicked();
        void on_slider_ambient_valueChanged(int value);
        void on_slider_default_ns_valueChanged(int value);
        void on_slider_dft_brightness_valueChanged(int value);
        void on_slider_diffuse_valueChanged(int value);
        void on_slider_shadow_quality_valueChanged(int value);
        void on_slider_specular_valueChanged(int value);
        void on_slider_clip_plane_valueChanged(int value);
        void on_slider_normals_valueChanged(int value);
        void on_toolButton_background_color_clicked();
        void on_toolButton_default_color_clicked();
        void on_toolButton_dft_background_color_clicked();
        void on_toolButton_dft_color_clicked();
        void on_toolButton_wireframe_color_clicked();
        void on_toolButton_clip_plane_color_clicked();
        void on_toolButton_normal_color_positive_clicked();
        void on_toolButton_normal_color_negative_clicked();

        void graphics_widget_resize(QResizeEvent*);
        void graphics_widget_mouse_wheel(QWheelEvent*);
        void graphics_widget_mouse_move(QMouseEvent*);
        void graphics_widget_mouse_press(QMouseEvent*);
        void graphics_widget_mouse_release(QMouseEvent*);

        void model_tree_item_changed();

        void slot_object_repository();
        void slot_timer_progress_bar();
        void slot_window_first_shown();

private:
        void constructor_threads();
        void constructor_connect();
        void constructor_interface();
        void constructor_objects_and_repository();

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

        void thread_load_from_file(std::string file_name, bool use_object_selection_dialog);
        void thread_load_from_repository(int dimension, const std::string& object_name);
        void thread_export(ObjectId id);
        void thread_bound_cocone(ObjectId id);
        void thread_self_test(SelfTestType test_type, bool with_confirmation);

        template <template <size_t, typename> typename SpatialMeshModel, size_t N, typename T>
        void paint(const std::shared_ptr<const SpatialMeshModel<N, T>>& mesh, const std::string& object_name);

        template <size_t N>
        void loaded_object(const std::shared_ptr<const MeshObject<N>>& object, int dimension);

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

        void exception_handler(const std::exception_ptr& ptr, const std::string& msg, bool window_exists) const
                noexcept;

        bool dialog_object_selection(std::unordered_set<ComputationType>* objects_to_load);

        bool stop_action(WorkerThreads::Action action);

        void message_error(const std::string& msg) override;
        void message_error_fatal(const std::string& msg) override;
        void message_error_source(const std::string& msg, const std::string& src) override;
        void message_information(const std::string& msg) override;
        void message_warning(const std::string& msg) override;
        void show_object_loaded(ObjectId id) override;
        void loaded_object(ObjectId id, size_t dimension) override;
        void loaded_mesh(ObjectId id, size_t dimension) override;
        void deleted_object(ObjectId id, size_t dimension) override;
        void deleted_all(size_t dimension) override;
        void file_loaded(const std::string& file_name, size_t dimension) override;
        void log(const std::string& msg) override;

        Ui::MainWindow ui;

        const std::thread::id m_window_thread_id;

        WindowEventEmitter m_event_emitter;

        std::unique_ptr<WorkerThreads> m_worker_threads;

        std::unordered_map<QObject*, std::tuple<int, std::string>> m_action_to_dimension_and_object_name;

        std::unique_ptr<ShowObject> m_show_object;
        Show* m_show = nullptr;

        std::unique_ptr<StorageManage> m_objects;

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

        unsigned m_dimension;

        bool m_close_without_confirmation;

        std::unordered_set<ComputationType> m_objects_to_load;

        const int m_mesh_threads;
};
