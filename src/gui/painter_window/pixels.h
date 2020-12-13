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

#include <src/color/color.h>
#include <src/color/conversion.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/image/conversion.h>
#include <src/image/format.h>
#include <src/painter/paintbrushes/bar_paintbrush.h>
#include <src/painter/painter.h>

#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace gui::painter_window
{
struct Pixels
{
        virtual ~Pixels() = default;

        virtual void set_slice_offset(const std::vector<int>& slider_positions) = 0;
        virtual const std::vector<long long>& busy_indices_2d() const = 0;
        virtual painter::Statistics statistics() const = 0;
        virtual const std::vector<int>& screen_size() const = 0;
        virtual image::ColorFormat color_format() const = 0;

        virtual std::span<const std::byte> slice_r8g8b8a8_with_background() const = 0;

        virtual std::vector<std::byte> slice(bool without_background) const = 0;
        virtual std::vector<std::byte> pixels(bool without_background) const = 0;
};

template <size_t N, typename T>
class PainterPixels final : public Pixels, public painter::PainterNotifier<N - 1>
{
        static constexpr int PANTBRUSH_WIDTH = 20;

        static constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R8G8B8A8_SRGB;
        static constexpr size_t PIXEL_SIZE = image::format_pixel_size_in_bytes(COLOR_FORMAT);

        const std::shared_ptr<const painter::Scene<N, T>> m_scene;
        painter::BarPaintbrush<N - 1> m_paintbrush;

        const std::vector<int> m_screen_size;
        const Color m_background_color;

        std::vector<std::byte> m_pixels_with_background;
        std::vector<std::byte> m_pixels_without_background;
        const size_t m_slice_size;
        long long m_slice_offset;
        std::vector<long long> m_busy_indices_2d;

        const GlobalIndex<N - 1, long long> m_global_index;

        std::atomic_bool m_painting_stop;
        std::thread m_painting_thread;

        template <typename Type, size_t Size>
        static std::vector<Type> array_to_vector(const std::array<Type, Size>& array)
        {
                std::vector<Type> vector(Size);
                std::copy(array.cbegin(), array.cend(), vector.begin());
                return vector;
        }

        static std::vector<std::byte> make_initial_image(
                const std::vector<int>& screen_size,
                image::ColorFormat color_format)
        {
                constexpr std::array<uint8_t, 4> LIGHT = {100, 150, 200, 255};
                constexpr std::array<uint8_t, 4> DARK = {0, 0, 0, 255};

                const size_t pixel_size = image::format_pixel_size_in_bytes(color_format);

                std::vector<std::byte> light;
                std::vector<std::byte> dark;
                image::format_conversion(
                        image::ColorFormat::R8G8B8A8_SRGB, std::as_bytes(std::span(LIGHT)), color_format, &light);
                image::format_conversion(
                        image::ColorFormat::R8G8B8A8_SRGB, std::as_bytes(std::span(DARK)), color_format, &dark);
                ASSERT(pixel_size == light.size());
                ASSERT(pixel_size == dark.size());

                const int slice_count = multiply_all<long long>(screen_size) / screen_size[0] / screen_size[1];

                std::vector<std::byte> image(pixel_size * multiply_all<long long>(screen_size));

                size_t index = 0;
                for (int i = 0; i < slice_count; ++i)
                {
                        for (int y = 0; y < screen_size[1]; ++y)
                        {
                                for (int x = 0; x < screen_size[0]; ++x)
                                {
                                        const std::byte* pixel = ((x + y) & 1) ? light.data() : dark.data();
                                        std::memcpy(&image[index], pixel, pixel_size);
                                        index += pixel_size;
                                }
                        }
                }

                ASSERT(index == image.size());

                return image;
        }

        void set_with_background(long long pixel_index, unsigned char r, unsigned char g, unsigned char b)
        {
                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);

                const std::array<unsigned char, 4> c{r, g, b, 255};
                static_assert(std::span(c).size_bytes() == PIXEL_SIZE);
                std::memcpy(&m_pixels_with_background[PIXEL_SIZE * pixel_index], c.data(), PIXEL_SIZE);
        }

        void set_without_background(
                long long pixel_index,
                unsigned char r,
                unsigned char g,
                unsigned char b,
                unsigned char a)
        {
                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);

                const std::array<unsigned char, 4> c{r, g, b, a};
                static_assert(std::span(c).size_bytes() == PIXEL_SIZE);
                std::memcpy(&m_pixels_without_background[PIXEL_SIZE * pixel_index], c.data(), PIXEL_SIZE);
        }

        // PainterNotifier

        void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N - 1>& pixel) override
        {
                long long x = pixel[0];
                long long y = m_screen_size[1] - 1 - pixel[1];
                m_busy_indices_2d[thread_number] = y * m_screen_size[0] + x;
        }

        void painter_pixel_after(
                unsigned /*thread_number*/,
                const std::array<int_least16_t, N - 1>& pixel,
                const Color& color,
                float coverage) override
        {
                std::array<int_least16_t, N - 1> p = pixel;
                p[1] = m_screen_size[1] - 1 - pixel[1];
                long long index = m_global_index.compute(p);
                if (coverage >= 1)
                {
                        unsigned char r = color::linear_float_to_srgb_uint8(color.red());
                        unsigned char g = color::linear_float_to_srgb_uint8(color.green());
                        unsigned char b = color::linear_float_to_srgb_uint8(color.blue());
                        set_with_background(index, r, g, b);
                        set_without_background(index, r, g, b, 255);
                }
                else if (coverage <= 0)
                {
                        const Color& c = m_background_color;
                        unsigned char r = color::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color::linear_float_to_srgb_uint8(c.blue());
                        set_with_background(index, r, g, b);
                        set_without_background(index, 0, 0, 0, 0);
                }
                else
                {
                        {
                                const Color& c = interpolation(m_background_color, color, coverage);
                                unsigned char r = color::linear_float_to_srgb_uint8(c.red());
                                unsigned char g = color::linear_float_to_srgb_uint8(c.green());
                                unsigned char b = color::linear_float_to_srgb_uint8(c.blue());
                                set_with_background(index, r, g, b);
                        }
                        {
                                unsigned char r = color::linear_float_to_srgb_uint8(color.red());
                                unsigned char g = color::linear_float_to_srgb_uint8(color.green());
                                unsigned char b = color::linear_float_to_srgb_uint8(color.blue());
                                unsigned char a = color::linear_float_to_linear_uint8(coverage);
                                set_without_background(index, r, g, b, a);
                        }
                }
        }

        void painter_error_message(const std::string& msg) override
        {
                MESSAGE_ERROR(msg);
        }

        // Pixels

        void set_slice_offset(const std::vector<int>& slider_positions) override
        {
                ASSERT(slider_positions.size() + 2 == N - 1);
                std::array<int_least16_t, N - 1> p;
                p[0] = 0;
                p[1] = 0;
                for (unsigned dimension = 2; dimension < N - 1; ++dimension)
                {
                        p[dimension] = slider_positions[dimension - 2];
                        ASSERT(p[dimension] >= 0 && p[dimension] < m_screen_size[dimension]);
                }
                m_slice_offset = PIXEL_SIZE * m_global_index.compute(p);
        }

        std::span<const std::byte> slice_r8g8b8a8_with_background() const override
        {
                return std::span(&m_pixels_with_background[m_slice_offset], m_slice_size);
        }

        std::vector<std::byte> slice(bool without_background) const override
        {
                if (without_background)
                {
                        return std::vector<std::byte>(
                                m_pixels_without_background.data() + m_slice_offset,
                                m_pixels_without_background.data() + m_slice_offset + m_slice_size);
                }
                return std::vector<std::byte>(
                        m_pixels_with_background.data() + m_slice_offset,
                        m_pixels_with_background.data() + m_slice_offset + m_slice_size);
        }

        std::vector<std::byte> pixels(bool without_background) const override
        {
                if (without_background)
                {
                        return m_pixels_without_background;
                }
                return m_pixels_with_background;
        }

        const std::vector<long long>& busy_indices_2d() const override
        {
                return m_busy_indices_2d;
        }

        painter::Statistics statistics() const override
        {
                return m_paintbrush.statistics();
        }

        const std::vector<int>& screen_size() const override
        {
                return m_screen_size;
        }

        image::ColorFormat color_format() const override
        {
                return COLOR_FORMAT;
        }

public:
        PainterPixels(
                const std::shared_ptr<const painter::Scene<N, T>>& scene,
                unsigned thread_count,
                int samples_per_pixel,
                bool smooth_normal)
                : m_scene(scene),
                  m_paintbrush(m_scene->projector().screen_size(), PANTBRUSH_WIDTH, -1),
                  m_screen_size(array_to_vector(m_scene->projector().screen_size())),
                  m_background_color(m_scene->background_color()),
                  m_pixels_with_background(make_initial_image(m_screen_size, COLOR_FORMAT)),
                  m_pixels_without_background(make_initial_image(m_screen_size, COLOR_FORMAT)),
                  m_slice_size(image::format_pixel_size_in_bytes(COLOR_FORMAT) * m_screen_size[0] * m_screen_size[1]),
                  m_slice_offset(0),
                  m_busy_indices_2d(thread_count, -1),
                  m_global_index(m_scene->projector().screen_size())
        {
                m_painting_stop = false;
                m_painting_thread = std::thread(
                        [=, this]()
                        {
                                paint(this, samples_per_pixel, *m_scene, &m_paintbrush, thread_count, &m_painting_stop,
                                      smooth_normal);
                        });
        }

        ~PainterPixels() override
        {
                m_painting_stop = true;
                if (m_painting_thread.joinable())
                {
                        m_painting_thread.join();
                }
        }
};
}
