/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "actions.h"
#include "image_widget.h"
#include "pixels.h"
#include "sliders_widget.h"
#include "statistics_widget.h"

#include "../com/application.h"
#include "../com/support.h"

#include "ui_painter_window.h"

#include <src/com/type/name.h>
#include <src/painter/painter.h>
#include <src/painter/scenes/storage.h>

#include <QTimer>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace ns::gui::painter_window
{
class PainterWindow final : public QMainWindow
{
        Q_OBJECT

private:
        const std::thread::id thread_id_ = std::this_thread::get_id();
        bool first_show_ = true;

        Ui::PainterWindow ui_;

        std::unique_ptr<Pixels> pixels_;
        long long slice_ = 0;

        std::unique_ptr<QSlider> brightness_parameter_slider_;
        std::unique_ptr<ImageWidget> image_widget_;
        std::unique_ptr<StatisticsWidget> statistics_widget_;
        std::unique_ptr<SlidersWidget> sliders_widget_;

        std::unique_ptr<Actions> actions_;

        QTimer timer_;

        void on_first_shown();

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void create_interface(const char* integrator_name, const char* floating_point_name, const char* color_name);
        void create_sliders();
        void create_actions();

        void adjust_window_size();

        void update_image();

public:
        PainterWindow(
                const std::string& name,
                painter::Integrator integrator,
                const char* floating_point_name,
                const char* color_name,
                std::unique_ptr<Pixels>&& pixels);

        ~PainterWindow() override;
};

template <std::size_t N, typename T, typename Color>
void create_painter_window(
        std::string name,
        const painter::Integrator integrator,
        const unsigned thread_count,
        const int samples_per_pixel,
        const bool flat_shading,
        painter::scenes::StorageScene<N, T, Color>&& scene)
{
        Application::run(
                [=, name = std::move(name),
                 scene = std::make_shared<painter::scenes::StorageScene<N, T, Color>>(std::move(scene))]()
                {
                        auto pixels = std::make_unique<PainterPixels<N, T, Color>>(
                                std::move(*scene), integrator, thread_count, samples_per_pixel, flat_shading);
                        create_and_show_delete_on_close_window<PainterWindow>(
                                name, integrator, type_bit_name<T>(), Color::name(), std::move(pixels));
                });
}
}
