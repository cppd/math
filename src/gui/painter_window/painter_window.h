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
#include "../dialogs/file_dialog.h"
#include "../dialogs/message.h"

#include <src/color/conversion.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/image/file.h>
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

        static constexpr unsigned char ALPHA_FOR_3D_IMAGE = 1;

        static constexpr int PANTBRUSH_WIDTH = 20;

        static constexpr size_t N_IMAGE = N - 1;

        const std::thread::id m_thread_id;
        const std::shared_ptr<const painter::Scene<N, T>> m_scene;
        const GlobalIndex<N_IMAGE, long long> m_global_index;
        const std::array<int, N - 1> m_screen_size;
        long long m_slice_offset;
        std::vector<std::uint_least32_t> m_pixels_bgra32;
        painter::BarPaintbrush<N_IMAGE> m_paintbrush;
        std::vector<long long> m_busy_indices_2d;
        std::atomic_bool m_stop;
        std::thread m_thread;

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

        static void flip_vertically_2d_images(std::vector<std::byte>* pixels, size_t width, size_t height)
        {
                size_t size_2d = width * height;
                ASSERT(pixels->size() % size_2d == 0);
                std::vector<std::byte> row(width);
                for (size_t offset_2d = 0; offset_2d < pixels->size(); offset_2d += size_2d)
                {
                        size_t row_1 = offset_2d;
                        size_t row_2 = offset_2d + width * (height - 1);
                        size_t row_1_max = offset_2d + width * (height / 2);
                        for (; row_1 < row_1_max; row_1 += width, row_2 -= width)
                        {
                                std::memcpy(row.data(), &(*pixels)[row_1], width);
                                std::memcpy(&(*pixels)[row_1], &(*pixels)[row_2], width);
                                std::memcpy(&(*pixels)[row_2], row.data(), width);
                        }
                }
        }

        static void set_alpha_brga32(std::vector<std::byte>* pixels, bool without_background, unsigned char alpha)
        {
                constexpr size_t PIXEL_SIZE = sizeof(std::uint_least32_t);
                ASSERT(pixels->size() % PIXEL_SIZE == 0);
                for (size_t i = 0; i < pixels->size(); i += PIXEL_SIZE)
                {
                        std::uint_least32_t c;
                        std::memcpy(&c, pixels->data() + i, PIXEL_SIZE);
                        if (!without_background)
                        {
                                c &= 0xff'ff'ff;
                                c |= (alpha << 24u);
                        }
                        else
                        {
                                if (((c >> 24u) & 0xff) != 0)
                                {
                                        c &= 0xff'ff'ff;
                                        c |= (alpha << 24u);
                                }
                                else
                                {
                                        c = 0;
                                }
                        }
                        std::memcpy(pixels->data() + i, &c, PIXEL_SIZE);
                }
        }

        //

        long long pixel_index(const std::array<int_least16_t, N_IMAGE>& pixel) const
        {
                return m_global_index.compute(pixel);
        }

        long long slice_offset_for_slider_positions(const std::vector<int>& slider_positions) const
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
                s.pass_progress = static_cast<double>(sp.pass_pixel_count) / m_pixels_bgra32.size();
                s.pixel_count = sp.pixel_count;
                s.ray_count = sp.ray_count;
                s.sample_count = sp.sample_count;
                s.previous_pass_duration = sp.previous_pass_duration;
                return s;
        }

        void slider_positions_change_event(const std::vector<int>& slider_positions) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_slice_offset = slice_offset_for_slider_positions(slider_positions);
        }

        const std::vector<std::uint_least32_t>& pixels_bgra32() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_pixels_bgra32;
        }

        long long pixels_offset() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_slice_offset;
        }

        const std::vector<long long>& busy_indices_2d() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_busy_indices_2d;
        }

        void save_to_file() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                int width = m_screen_size[0];
                int height = m_screen_size[1];

                std::vector<std::byte> pixels_bgra32(sizeof(std::uint_least32_t) * width * height);
                std::memcpy(pixels_bgra32.data(), &m_pixels_bgra32[m_slice_offset], data_size(pixels_bgra32));

                const std::string caption = "Save";
                dialog::FileFilter filter;
                filter.name = "Images";
                filter.file_extensions = {SAVE_IMAGE_FILE_FORMAT};
                const bool read_only = true;
                const std::optional<std::string> file_name_string = dialog::save_file(caption, {filter}, read_only);
                if (!file_name_string)
                {
                        return;
                }

                constexpr image::ColorFormat FORMAT = image::ColorFormat::R8G8B8_SRGB;

                image::save_image_to_file(
                        path_from_utf8(*file_name_string),
                        image::ImageView<2>(
                                {width, height}, FORMAT, format_conversion_from_bgra32(pixels_bgra32, FORMAT)));
        }

        void save_all_to_files(bool without_background) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                namespace fs = std::filesystem;

                if (N_IMAGE != 3)
                {
                        return;
                }

                std::vector<std::byte> pixels_bgra32(sizeof(std::uint_least32_t) * m_pixels_bgra32.size());
                ASSERT(data_size(pixels_bgra32) == data_size(m_pixels_bgra32));
                std::memcpy(pixels_bgra32.data(), m_pixels_bgra32.data(), data_size(pixels_bgra32));

                const std::string caption = "Save All";
                const bool read_only = false;
                const std::optional<std::string> directory_string = dialog::select_directory(caption, read_only);
                if (!directory_string)
                {
                        return;
                }

                if (!without_background)
                {
                        constexpr image::ColorFormat FORMAT = image::ColorFormat::R8G8B8_SRGB;

                        image::save_image_to_files(
                                path_from_utf8(*directory_string), SAVE_IMAGE_FILE_FORMAT,
                                image::ImageView<3>(
                                        {m_screen_size[0], m_screen_size[1], m_screen_size[2]}, FORMAT,
                                        format_conversion_from_bgra32(pixels_bgra32, FORMAT)));
                }
                else
                {
                        set_alpha_brga32(&pixels_bgra32, true /*without_background*/, 255);

                        constexpr image::ColorFormat FORMAT = image::ColorFormat::R8G8B8A8_SRGB;

                        image::save_image_to_files(
                                path_from_utf8(*directory_string), SAVE_IMAGE_FILE_FORMAT,
                                image::ImageView<3>(
                                        {m_screen_size[0], m_screen_size[1], m_screen_size[2]}, FORMAT,
                                        format_conversion_from_bgra32(pixels_bgra32, FORMAT)));
                }
        }

        void add_volume(bool without_background) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (N_IMAGE != 3)
                {
                        return;
                }

                std::vector<std::byte> pixels_bgra32(sizeof(std::uint_least32_t) * m_pixels_bgra32.size());
                ASSERT(data_size(pixels_bgra32) == data_size(m_pixels_bgra32));
                std::memcpy(pixels_bgra32.data(), m_pixels_bgra32.data(), data_size(pixels_bgra32));

                flip_vertically_2d_images(
                        &pixels_bgra32, sizeof(std::uint_least32_t) * m_screen_size[0], m_screen_size[1]);

                set_alpha_brga32(&pixels_bgra32, without_background, ALPHA_FOR_3D_IMAGE);

                image::Image<N_IMAGE> image;
                image.size = m_screen_size;
                image.color_format = image::ColorFormat::R8G8B8A8_SRGB;
                image.pixels = format_conversion_from_bgra32(pixels_bgra32, image.color_format);

                process::load_from_volume_image<N_IMAGE>("Painter Volume", std::move(image));
        }

        // IPainterNotifier
        void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N_IMAGE>& pixel) override
        {
                long long x = pixel[0];
                long long y = m_screen_size[1] - 1 - pixel[1];
                m_busy_indices_2d[thread_number] = y * m_screen_size[0] + x;
        }

        void set_pixels_rgb(long long pixel_index, unsigned char r, unsigned char g, unsigned char b)
        {
                m_pixels_bgra32[pixel_index] = Srgb8(r, g, b).to_uint_bgr();
        }

        void set_pixels_rgba(long long pixel_index, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        {
                m_pixels_bgra32[pixel_index] = Srgb8(r, g, b).to_uint_bgr() | (a << 24u);
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
                        if (N_IMAGE != 3)
                        {
                                set_pixels_rgb(pixel_index(p), r, g, b);
                        }
                        else
                        {
                                set_pixels_rgba(pixel_index(p), r, g, b, ALPHA_FOR_3D_IMAGE);
                        }
                }
                else if (coverage <= 0)
                {
                        Color c = m_scene->background_color();
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        set_pixels_rgb(pixel_index(p), r, g, b);
                }
                else
                {
                        Color c = interpolation(m_scene->background_color(), color, coverage);
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        set_pixels_rgb(pixel_index(p), r, g, b);
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
                  m_slice_offset(slice_offset_for_slider_positions(initial_slider_positions())),
                  m_pixels_bgra32(make_bgra32_images(m_scene->projector().screen_size())),
                  m_paintbrush(m_scene->projector().screen_size(), PANTBRUSH_WIDTH, -1),
                  m_busy_indices_2d(thread_count, -1)
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
