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

#include "../application/main_thread.h"
#include "../com/support.h"
#include "../dialogs/file_dialog.h"

#include <src/color/conversion.h>
#include <src/com/alg.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/image/file.h>
#include <src/painter/painter.h>
#include <src/painter/visible_paintbrush.h>
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

        static constexpr int PANTBRUSH_WIDTH = 20;
        static constexpr std::uint_least32_t COLOR_LIGHT = Srgb8(100, 150, 200).to_uint_bgr();
        static constexpr std::uint_least32_t COLOR_DARK = Srgb8(0, 0, 0).to_uint_bgr();

        static constexpr size_t N_IMAGE = N - 1;

        const std::thread::id m_thread_id;
        const std::shared_ptr<const painter::PaintObjects<N, T>> m_paint_objects;
        const GlobalIndex<N_IMAGE, long long> m_global_index;
        const std::array<int, N - 1> m_screen_size;
        const int m_height;
        long long m_slice_offset;
        std::vector<std::uint_least32_t> m_pixels_bgr;
        std::vector<std::byte> m_pixels_bytes_rgba;
        painter::VisibleBarPaintbrush<N_IMAGE> m_paintbrush;
        std::vector<long long> m_busy_pixels;
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
                               && pixel[dimension] < m_paint_objects->projector().screen_size()[dimension]);
                }

                return pixel_index(pixel);
        }

        // PainterWindow2d
        void painter_statistics(
                long long* pass_count,
                long long* pixel_count,
                long long* ray_count,
                long long* sample_count,
                double* previous_pass_duration) const override
        {
                m_paintbrush.statistics(pass_count, pixel_count, ray_count, sample_count, previous_pass_duration);
        }

        void slider_positions_change_event(const std::vector<int>& slider_positions) override
        {
                m_slice_offset = slice_offset_for_slider_positions(slider_positions);
        }

        const std::vector<std::uint_least32_t>& pixels_bgr() const override
        {
                return m_pixels_bgr;
        }

        long long pixels_offset() const override
        {
                return m_slice_offset;
        }

        const std::vector<long long>& pixels_busy() const override
        {
                return m_busy_pixels;
        }

        void save_to_file() const override
        {
                int width = m_screen_size[0];
                int height = m_screen_size[1];

                std::vector<std::uint_least32_t> pixels(1ull * width * height);
                std::memcpy(pixels.data(), &m_pixels_bgr[m_slice_offset], data_size(pixels));

                std::vector<std::byte> bytes(3ull * width * height);
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
                        image::ImageView<2>({width, height}, image::ColorFormat::R8G8B8_SRGB, bytes));
        }

        void add_volume() const override
        {
                if (N_IMAGE != 3)
                {
                        return;
                }

                ASSERT(4 * multiply_all<long long>(m_screen_size)
                       == static_cast<long long>(m_pixels_bytes_rgba.size()));

                image::Image<N_IMAGE> image;

                image.size = m_screen_size;
                image.color_format = image::ColorFormat::R8G8B8A8_SRGB;

                image.pixels.resize(m_pixels_bytes_rgba.size());
                std::memcpy(image.pixels.data(), m_pixels_bytes_rgba.data(), image.pixels.size());

                process::load_from_volume_image<N_IMAGE>("Painter Volume", std::move(image));
        }

        // IPainterNotifier
        void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N_IMAGE>& pixel) override
        {
                std::array<int_least16_t, N_IMAGE> p = pixel;
                p[1] = m_height - 1 - pixel[1];

                m_busy_pixels[thread_number] = pixel_index(p);
        }

        void set_pixels_rgba(long long pixel_index, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        {
                std::byte* ptr = &m_pixels_bytes_rgba[4 * pixel_index];
                std::memcpy(ptr++, &r, 1);
                std::memcpy(ptr++, &g, 1);
                std::memcpy(ptr++, &b, 1);
                std::memcpy(ptr++, &a, 1);
        }

        void painter_pixel_after(
                unsigned /*thread_number*/,
                const std::array<int_least16_t, N_IMAGE>& pixel,
                const Color& color,
                float coverage) override
        {
                std::array<int_least16_t, N_IMAGE> p = pixel;
                p[1] = m_height - 1 - pixel[1];

                if (coverage >= 1)
                {
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(color.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(color.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(color.blue());
                        m_pixels_bgr[pixel_index(p)] = Srgb8(r, g, b).to_uint_bgr();
                        if (N_IMAGE == 3)
                        {
                                set_pixels_rgba(pixel_index(pixel), r, g, b, 1);
                        }
                }
                else if (coverage <= 0)
                {
                        Color c = m_paint_objects->background_color();
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        m_pixels_bgr[pixel_index(p)] = Srgb8(r, g, b).to_uint_bgr();
                }
                else
                {
                        Color c = interpolation(m_paint_objects->background_color(), color, coverage);
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        m_pixels_bgr[pixel_index(p)] = Srgb8(r, g, b).to_uint_bgr();
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
                const std::shared_ptr<const painter::PaintObjects<N, T>>& paint_objects)
                : PainterWindow2d(
                        name,
                        array_to_vector(paint_objects->projector().screen_size()),
                        initial_slider_positions()),
                  m_thread_id(std::this_thread::get_id()),
                  m_paint_objects(paint_objects),
                  m_global_index(m_paint_objects->projector().screen_size()),
                  m_screen_size(m_paint_objects->projector().screen_size()),
                  m_height(m_paint_objects->projector().screen_size()[1]),
                  m_slice_offset(slice_offset_for_slider_positions(initial_slider_positions())),
                  m_pixels_bgr(make_bgr_images(m_paint_objects->projector().screen_size())),
                  m_paintbrush(m_paint_objects->projector().screen_size(), PANTBRUSH_WIDTH, -1),
                  m_busy_pixels(thread_count, -1)
        {
                if (N_IMAGE == 3)
                {
                        m_pixels_bytes_rgba.resize(4 * m_pixels_bgr.size());
                        std::memset(m_pixels_bytes_rgba.data(), 0, m_pixels_bytes_rgba.size());
                }

                m_stop = false;
                m_thread = std::thread([=, this]() {
                        paint(this, samples_per_pixel, *m_paint_objects, &m_paintbrush, thread_count, &m_stop,
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
}

template <size_t N, typename T>
void create_painter_window(
        const std::string& name,
        unsigned thread_count,
        int samples_per_pixel,
        bool smooth_normal,
        std::unique_ptr<const painter::PaintObjects<N, T>>&& paint_objects)
{
        namespace impl = painter_window_implementation;
        std::shared_ptr<const painter::PaintObjects<N, T>> objects = std::move(paint_objects);
        application::MainThreadQueue::push([=]() {
                create_and_show_delete_on_close_window<impl::PainterWindow<N, T>>(
                        name, thread_count, samples_per_pixel, smooth_normal, objects);
        });
}
}
