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

#include "painter_window_2d.h"

#include "../com/main_thread.h"
#include "../com/support.h"
#include "../dialogs/file_dialog.h"

#include <src/color/conversion.h>
#include <src/com/alg.h>
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
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gui
{
namespace painter_window_implementation
{
template <size_t N, typename T>
class PainterWindow final : public PainterWindow2d, public painter::PainterNotifier<N - 1>
{
        static_assert(N >= 3);

        static constexpr const char* SAVE_IMAGE_FILE_FORMAT = "png";

        static constexpr unsigned char ALPHA_FOR_3D_IMAGE = 1;

        static constexpr int PANTBRUSH_WIDTH = 20;
        static constexpr std::uint_least32_t COLOR_LIGHT = Srgb8(100, 150, 200).to_uint_bgr();
        static constexpr std::uint_least32_t COLOR_DARK = Srgb8(0, 0, 0).to_uint_bgr();

        static constexpr size_t N_IMAGE = N - 1;

        const std::thread::id m_thread_id;
        const std::shared_ptr<const painter::Scene<N, T>> m_scene;
        const GlobalIndex<N_IMAGE, long long> m_global_index;
        const std::array<int, N - 1> m_screen_size;
        const long long m_pixel_count;
        long long m_slice_offset;
        std::vector<std::uint_least32_t> m_pixels_bgr32;
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

        static std::vector<std::uint_least32_t> make_bgr_images(const std::array<int, N - 1>& screen_size)
        {
                int width = screen_size[0];
                int height = screen_size[1];
                int count = multiply_all<long long>(screen_size) / (width * height);

                std::vector<std::uint_least32_t> images(width * height * count);

                long long index = 0;
                for (int i = 0; i < count; ++i)
                {
                        for (int y = 0; y < height; ++y)
                        {
                                for (int x = 0; x < width; ++x)
                                {
                                        images[index] = ((x + y) & 1) ? COLOR_LIGHT : COLOR_DARK;
                                        ++index;
                                }
                        }
                }

                ASSERT(index == static_cast<long long>(images.size()));

                return images;
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

        static std::vector<std::byte> bgra32_to_r8g8b8(const std::vector<std::uint_least32_t>& pixels)
        {
                std::vector<std::byte> bytes(3 * pixels.size());
                std::byte* ptr = bytes.data();
                for (std::uint_least32_t c : pixels)
                {
                        unsigned char b = c & 0xff;
                        unsigned char g = (c >> 8) & 0xff;
                        unsigned char r = (c >> 16) & 0xff;
                        std::memcpy(ptr++, &r, 1);
                        std::memcpy(ptr++, &g, 1);
                        std::memcpy(ptr++, &b, 1);
                }
                return bytes;
        }

        static void bgra32_to_r8g8b8a8(std::vector<std::byte>* pixels)
        {
                static_assert(4 * sizeof(std::byte) == sizeof(std::uint_least32_t));
                ASSERT(pixels->size() % 4 == 0);
                for (size_t i = 0; i < pixels->size();)
                {
                        std::uint_least32_t c;
                        std::memcpy(&c, &(*pixels)[i], 4);
                        unsigned char b = c & 0xff;
                        unsigned char g = (c >> 8) & 0xff;
                        unsigned char r = (c >> 16) & 0xff;
                        unsigned char a = (c >> 24) & 0xff;
                        std::memcpy(&(*pixels)[i++], &r, 1);
                        std::memcpy(&(*pixels)[i++], &g, 1);
                        std::memcpy(&(*pixels)[i++], &b, 1);
                        std::memcpy(&(*pixels)[i++], &a, 1);
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
                m_slice_offset = slice_offset_for_slider_positions(slider_positions);
        }

        const std::vector<std::uint_least32_t>& pixels_bgr() const override
        {
                return m_pixels_bgr32;
        }

        long long pixels_offset() const override
        {
                return m_slice_offset;
        }

        const std::vector<long long>& busy_indices_2d() const override
        {
                return m_busy_indices_2d;
        }

        void save_to_file() const override
        {
                int width = m_screen_size[0];
                int height = m_screen_size[1];

                std::vector<std::uint_least32_t> pixels(1ull * width * height);
                std::memcpy(pixels.data(), &m_pixels_bgr32[m_slice_offset], data_size(pixels));

                const std::string caption = "Save";
                dialog::FileFilter filter;
                filter.name = "Images";
                filter.file_extensions = {SAVE_IMAGE_FILE_FORMAT};
                const bool read_only = true;
                std::string file_name_string;
                if (!dialog::save_file(caption, {filter}, read_only, &file_name_string))
                {
                        return;
                }

                image::save_image_to_file(
                        path_from_utf8(file_name_string),
                        image::ImageView<2>(
                                {width, height}, image::ColorFormat::R8G8B8_SRGB, bgra32_to_r8g8b8(pixels)));
        }

        void add_volume() const override
        {
                if (N_IMAGE != 3)
                {
                        return;
                }

                image::Image<N_IMAGE> image;

                image.size = m_screen_size;
                image.color_format = image::ColorFormat::R8G8B8A8_SRGB;

                static_assert(4 * sizeof(std::byte) == sizeof(std::uint_least32_t));
                image.pixels.resize(4 * m_pixels_bgr32.size());

                ASSERT(data_size(image.pixels) == data_size(m_pixels_bgr32));
                std::memcpy(image.pixels.data(), m_pixels_bgr32.data(), image.pixels.size());

                flip_vertically_2d_images(&image.pixels, 4ull * m_screen_size[0], m_screen_size[1]);

                bgra32_to_r8g8b8a8(&image.pixels);

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
                m_pixels_bgr32[pixel_index] = Srgb8(r, g, b).to_uint_bgr();
        }

        void set_pixels_rgba(long long pixel_index, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        {
                m_pixels_bgr32[pixel_index] = Srgb8(r, g, b).to_uint_bgr() | (a << 24u);
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
                  m_pixel_count(multiply_all<long long>(m_screen_size)),
                  m_slice_offset(slice_offset_for_slider_positions(initial_slider_positions())),
                  m_pixels_bgr32(make_bgr_images(m_scene->projector().screen_size())),
                  m_paintbrush(m_scene->projector().screen_size(), PANTBRUSH_WIDTH, -1),
                  m_busy_indices_2d(thread_count, -1)
        {
                m_stop = false;
                m_thread = std::thread([=, this]() {
                        paint(this, samples_per_pixel, *m_scene, &m_paintbrush, thread_count, &m_stop, smooth_normal);
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
}

template <size_t N, typename T>
void create_painter_window(
        const std::string& name,
        unsigned thread_count,
        int samples_per_pixel,
        bool smooth_normal,
        std::unique_ptr<const painter::Scene<N, T>>&& scene)
{
        namespace impl = painter_window_implementation;
        MainThread::run([=, scene = std::shared_ptr<const painter::Scene<N, T>>(std::move(scene))]() {
                create_and_show_delete_on_close_window<impl::PainterWindow<N, T>>(
                        name, thread_count, samples_per_pixel, smooth_normal, scene);
        });
}
}
