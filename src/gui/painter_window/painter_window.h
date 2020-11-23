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

#include "conversion.h"
#include "image.h"
#include "painter_window_2d.h"

#include "../com/main_thread.h"
#include "../com/support.h"
#include "../com/threads.h"
#include "../dialogs/file_dialog.h"
#include "../dialogs/message.h"

#include <src/color/conversion.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/image/file.h>
#include <src/image/flip.h>
#include <src/painter/paintbrushes/bar_paintbrush.h>
#include <src/painter/painter.h>
#include <src/process/load.h>
#include <src/utility/file/path.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gui::painter_window
{
template <size_t N, typename T>
class PainterWindow final : public painter_window_implementation::PainterWindow2d,
                            public painter::PainterNotifier<N - 1>
{
        static_assert(N >= 3);

        static constexpr const char* SAVE_IMAGE_FILE_FORMAT = "png";
        static constexpr unsigned char ALPHA_FOR_FILES = 255;
        static constexpr unsigned char ALPHA_FOR_VOLUME = 1;
        static constexpr unsigned char ALPHA_FOR_FULL_COVERAGE = 1;
        static constexpr int PANTBRUSH_WIDTH = 20;
        static constexpr size_t N_IMAGE = N - 1;

        const std::thread::id m_thread_id;

        const std::shared_ptr<const painter::Scene<N, T>> m_scene;
        const GlobalIndex<N_IMAGE, long long> m_global_index;
        const std::array<int, N - 1> m_screen_size;
        const long long m_pixel_count;

        const size_t m_slice_size;
        long long m_slice_offset;

        std::vector<std::byte> m_pixels_bgra;
        painter::BarPaintbrush<N_IMAGE> m_paintbrush;
        std::vector<long long> m_busy_indices_2d;

        std::atomic_bool m_stop;
        std::thread m_thread;

        std::unique_ptr<WorkerThreads> m_worker_threads;

        //

        template <typename Type, size_t Size>
        static std::vector<Type> array_to_vector(const std::array<Type, Size>& array)
        {
                std::vector<Type> vector(Size);
                std::copy(array.cbegin(), array.cend(), vector.begin());
                return vector;
        }

        static std::vector<int> initial_slider_positions()
        {
                return std::vector<int>(N_IMAGE - 2, 0);
        }

        static void set_alpha_brga(std::vector<std::byte>* pixels, unsigned char alpha)
        {
                ASSERT(pixels->size() % 4 == 0);
                ASSERT(alpha > 0);

                for (auto iter = pixels->begin(); iter != pixels->end(); std::advance(iter, 4))
                {
                        std::memcpy(&(*(iter + 3)), &alpha, 1);
                }
        }

        static void correct_alpha_brga(std::vector<std::byte>* pixels, unsigned char alpha)
        {
                ASSERT(pixels->size() % 4 == 0);
                ASSERT(alpha > 0);

                for (auto iter = pixels->begin(); iter != pixels->end(); std::advance(iter, 4))
                {
                        unsigned char a;
                        std::memcpy(&a, &(*(iter + 3)), 1);
                        if (a != 0)
                        {
                                std::memcpy(&(*(iter + 3)), &alpha, 1);
                        }
                        else
                        {
                                std::array<unsigned char, 4> c{0, 0, 0, 0};
                                std::memcpy(&(*iter), c.data(), 4);
                        }
                }
        }

        //

        long long pixel_index(const std::array<int_least16_t, N_IMAGE>& pixel) const
        {
                return m_global_index.compute(pixel);
        }

        long long pixel_index_for_slider_positions(const std::vector<int>& slider_positions) const
        {
                ASSERT(slider_positions.size() + 2 == N_IMAGE);

                std::array<int_least16_t, N_IMAGE> pixel;

                pixel[0] = 0;
                pixel[1] = 0;

                for (unsigned i = 0; i < slider_positions.size(); ++i)
                {
                        int dimension = i + 2;
                        pixel[dimension] = slider_positions[i];

                        ASSERT(pixel[dimension] >= 0
                               && pixel[dimension] < m_scene->projector().screen_size()[dimension]);
                }

                return pixel_index(pixel);
        }

        // PainterWindow2d

        Statistics statistics() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                const painter::Statistics sp = m_paintbrush.statistics();
                Statistics s;
                s.pass_number = sp.pass_number;
                s.pass_progress = static_cast<double>(sp.pass_pixel_count) / m_pixel_count;
                s.pixel_count = sp.pixel_count;
                s.ray_count = sp.ray_count;
                s.sample_count = sp.sample_count;
                s.previous_pass_duration = sp.previous_pass_duration;
                return s;
        }

        void slider_positions_change_event(const std::vector<int>& slider_positions) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_slice_offset = 4 * pixel_index_for_slider_positions(slider_positions);
        }

        std::span<const std::byte> pixels_bgra_2d() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return std::span(&m_pixels_bgra[m_slice_offset], m_slice_size);
        }

        const std::vector<long long>& busy_indices_2d() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_busy_indices_2d;
        }

        void save_to_file(bool without_background) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                std::shared_ptr<std::vector<std::byte>> pixels_bgra =
                        std::make_shared<std::vector<std::byte>>(m_slice_size);

                std::memcpy(pixels_bgra->data(), &m_pixels_bgra[m_slice_offset], data_size(*pixels_bgra));

                m_worker_threads->terminate_and_start(
                        0, "Saving to file",
                        [&]() -> std::function<void(ProgressRatioList*)>
                        {
                                const std::string caption = "Save";
                                dialog::FileFilter filter;
                                filter.name = "Images";
                                filter.file_extensions = {SAVE_IMAGE_FILE_FORMAT};
                                const bool read_only = true;
                                std::optional<std::string> file_name_string =
                                        dialog::save_file(caption, {filter}, read_only);
                                if (!file_name_string)
                                {
                                        return nullptr;
                                }

                                std::function<void(ProgressRatioList*)> f =
                                        [pixels_bgra = std::move(pixels_bgra),
                                         file_name_string = std::move(file_name_string), screen_size = m_screen_size,
                                         without_background](ProgressRatioList* progress_list)
                                {
                                        ProgressRatio progress(progress_list, "Saving");
                                        progress.set(0);

                                        image::ColorFormat format;
                                        if (without_background)
                                        {
                                                correct_alpha_brga(pixels_bgra.get(), ALPHA_FOR_FILES);
                                                format = image::ColorFormat::R8G8B8A8_SRGB;
                                        }
                                        else
                                        {
                                                format = image::ColorFormat::R8G8B8_SRGB;
                                        }

                                        image::save_image_to_file(
                                                path_from_utf8(*file_name_string),
                                                image::ImageView<2>(
                                                        {screen_size[0], screen_size[1]}, format,
                                                        format_conversion_from_bgra(*pixels_bgra, format)));
                                };
                                return f;
                        });
        }

        void save_all_to_files(bool without_background) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                namespace fs = std::filesystem;

                if constexpr (N_IMAGE >= 3)
                {
                        std::shared_ptr<std::vector<std::byte>> pixels_bgra =
                                std::make_shared<std::vector<std::byte>>(m_pixels_bgra);

                        m_worker_threads->terminate_and_start(
                                0, "Saving all to files",
                                [&]() -> std::function<void(ProgressRatioList*)>
                                {
                                        const std::string caption = "Save All";
                                        const bool read_only = false;
                                        std::optional<std::string> directory_string =
                                                dialog::select_directory(caption, read_only);
                                        if (!directory_string)
                                        {
                                                return nullptr;
                                        }

                                        std::function<void(ProgressRatioList*)> f =
                                                [pixels_bgra = std::move(pixels_bgra),
                                                 directory_string = std::move(directory_string),
                                                 screen_size = m_screen_size,
                                                 without_background](ProgressRatioList* progress_list)
                                        {
                                                ProgressRatio progress(progress_list, "Saving");
                                                progress.set(0);

                                                image::ColorFormat format;
                                                if (without_background)
                                                {
                                                        correct_alpha_brga(pixels_bgra.get(), ALPHA_FOR_FILES);
                                                        format = image::ColorFormat::R8G8B8A8_SRGB;
                                                }
                                                else
                                                {
                                                        format = image::ColorFormat::R8G8B8_SRGB;
                                                }

                                                image::save_image_to_files(
                                                        path_from_utf8(*directory_string), SAVE_IMAGE_FILE_FORMAT,
                                                        image::ImageView<N_IMAGE>(
                                                                screen_size, format,
                                                                format_conversion_from_bgra(*pixels_bgra, format)));
                                        };
                                        return f;
                                });
                }
        }

        void add_volume(bool without_background) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                std::shared_ptr<std::vector<std::byte>> pixels_bgra =
                        std::make_shared<std::vector<std::byte>>(m_pixels_bgra);

                m_worker_threads->terminate_and_start(
                        0, "Adding volume",
                        [&]() -> std::function<void(ProgressRatioList*)>
                        {
                                std::function<void(ProgressRatioList*)> f =
                                        [pixels_bgra = std::move(pixels_bgra), screen_size = m_screen_size,
                                         without_background](ProgressRatioList* progress_list)
                                {
                                        ProgressRatio progress(progress_list, "Adding volume");
                                        progress.set(0);

                                        if (without_background)
                                        {
                                                correct_alpha_brga(pixels_bgra.get(), ALPHA_FOR_VOLUME);
                                        }
                                        else
                                        {
                                                set_alpha_brga(pixels_bgra.get(), ALPHA_FOR_VOLUME);
                                        }

                                        image::Image<N_IMAGE> image;
                                        image.size = screen_size;
                                        image.color_format = image::ColorFormat::R8G8B8A8_SRGB;
                                        image.pixels = format_conversion_from_bgra(*pixels_bgra, image.color_format);

                                        image::flip_image_vertically(&image);

                                        process::load_from_volume_image<N_IMAGE>("Painter Volume", std::move(image));
                                };
                                return f;
                        });
        }

        void timer_event() override
        {
                m_worker_threads->set_progresses();
        }

        // IPainterNotifier

        void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N_IMAGE>& pixel) override
        {
                long long x = pixel[0];
                long long y = m_screen_size[1] - 1 - pixel[1];
                m_busy_indices_2d[thread_number] = y * m_screen_size[0] + x;
        }

        void set_pixel(long long pixel_index, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        {
                const std::array<unsigned char, 4> c{b, g, r, a};
                std::memcpy(&m_pixels_bgra[4 * pixel_index], c.data(), 4);
        }

        void painter_pixel_after(
                unsigned /*thread_number*/,
                const std::array<int_least16_t, N_IMAGE>& pixel,
                const Color& color,
                float coverage) override
        {
                std::array<int_least16_t, N_IMAGE> p = pixel;
                p[1] = m_screen_size[1] - 1 - pixel[1];

                if (coverage >= 1)
                {
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(color.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(color.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(color.blue());
                        set_pixel(pixel_index(p), r, g, b, ALPHA_FOR_FULL_COVERAGE);
                }
                else if (coverage <= 0)
                {
                        Color c = m_scene->background_color();
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        set_pixel(pixel_index(p), r, g, b, 0);
                }
                else
                {
                        Color c = interpolation(m_scene->background_color(), color, coverage);
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        set_pixel(pixel_index(p), r, g, b, 0);
                }
        }

        void painter_error_message(const std::string& msg) override
        {
                MESSAGE_ERROR(msg);
        }

public:
        PainterWindow(
                const std::string& name,
                unsigned thread_count,
                int samples_per_pixel,
                bool smooth_normal,
                const std::shared_ptr<const painter::Scene<N, T>>& scene)
                : PainterWindow2d(name, array_to_vector(scene->projector().screen_size()), initial_slider_positions()),
                  m_thread_id(std::this_thread::get_id()),
                  m_scene(scene),
                  m_global_index(m_scene->projector().screen_size()),
                  m_screen_size(m_scene->projector().screen_size()),
                  m_pixel_count(multiply_all<long long>(m_screen_size)),
                  m_slice_size(4ull * m_screen_size[0] * m_screen_size[1]),
                  m_slice_offset(4ull * pixel_index_for_slider_positions(initial_slider_positions())),
                  m_pixels_bgra(make_bgra_image(m_scene->projector().screen_size())),
                  m_paintbrush(m_scene->projector().screen_size(), PANTBRUSH_WIDTH, -1),
                  m_busy_indices_2d(thread_count, -1),
                  m_worker_threads(create_worker_threads(1, 0, status_bar()))
        {
                m_stop = false;
                m_thread = std::thread(
                        [=, this]()
                        {
                                paint(this, samples_per_pixel, *m_scene, &m_paintbrush, thread_count, &m_stop,
                                      smooth_normal);
                        });
        }

        ~PainterWindow() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_worker_threads->terminate_all();

                m_stop = true;
                if (m_thread.joinable())
                {
                        m_thread.join();
                }
        }

        PainterWindow(const PainterWindow&) = delete;
        PainterWindow(PainterWindow&&) = delete;
        PainterWindow& operator=(const PainterWindow&) = delete;
        PainterWindow& operator=(PainterWindow&&) = delete;
};

template <size_t N, typename T>
void create_painter_window(
        const std::string& name,
        unsigned thread_count,
        int samples_per_pixel,
        bool smooth_normal,
        std::unique_ptr<const painter::Scene<N, T>>&& scene)
{
        MainThread::run(
                [=, scene = std::shared_ptr<const painter::Scene<N, T>>(std::move(scene))]()
                {
                        create_and_show_delete_on_close_window<PainterWindow<N, T>>(
                                name, thread_count, samples_per_pixel, smooth_normal, scene);
                });
}
}
