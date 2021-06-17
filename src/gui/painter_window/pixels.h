/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "initial_image.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/com/min_max.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/image/format.h>
#include <src/painter/painter.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <vector>

namespace ns::gui::painter_window
{
struct Pixels
{
        struct Image
        {
                image::ColorFormat color_format;
                std::vector<std::byte> pixels;
        };
        struct Images
        {
                std::vector<int> size;
                Image rgb;
                Image rgba;
        };

        virtual ~Pixels() = default;

        virtual const char* floating_point_name() const = 0;
        virtual const char* color_name() const = 0;
        virtual std::optional<float> pixel_max() const = 0;

        virtual const std::vector<int>& screen_size() const = 0;

        virtual std::span<const std::byte> slice_r8g8b8a8(long long slice_number, float brightness_parameter) = 0;
        virtual const std::vector<long long>& busy_indices_2d() const = 0;
        virtual painter::Statistics statistics() const = 0;

        virtual std::optional<Images> slice(long long slice_number) const = 0;
        virtual std::optional<Images> pixels() const = 0;
};

template <std::size_t N>
class PainterPixels final : public Pixels, public painter::Notifier<N - 1>
{
        static_assert(N >= 3);

        static constexpr std::chrono::seconds NORMALIZE_INTERVAL{10};

        static constexpr std::optional<int> MAX_PASS_COUNT = std::nullopt;
        static constexpr long long NULL_INDEX = -1;

        static constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R8G8B8A8_SRGB;
        static constexpr std::size_t PIXEL_SIZE = image::format_pixel_size_in_bytes(COLOR_FORMAT);
        static constexpr uint8_t ALPHA = limits<uint8_t>::max();

        const std::thread::id m_thread_id = std::this_thread::get_id();

        const char* const m_floating_point_name;
        const char* const m_color_name;

        const GlobalIndex<N - 1, long long> m_global_index;
        const std::vector<int> m_screen_size;
        const long long m_slice_count = m_global_index.count() / m_screen_size[0] / m_screen_size[1];
        const std::size_t m_slice_size = PIXEL_SIZE * m_screen_size[0] * m_screen_size[1];

        std::vector<long long> m_busy_indices_2d;

        std::vector<std::byte> m_pixels_r8g8b8a8 = make_initial_image(m_screen_size, COLOR_FORMAT);

        static constexpr float MIN = limits<float>::lowest();
        std::vector<Vector<3, float>> m_pixels_original{
                static_cast<std::size_t>(m_global_index.count()), Vector<3, float>(MIN, MIN, MIN)};
        static_assert(sizeof(m_pixels_original[0]) == 3 * sizeof(float));
        std::span<const float> m_pixels_original_span{m_pixels_original[0].data(), 3 * m_pixels_original.size()};

        float m_pixel_brightness_parameter = 0;
        std::optional<float> m_pixel_max;
        float m_pixel_coef = 1;

        TimePoint m_last_normalize_time = time();

        painter::Images<N - 1> m_painter_images;
        std::atomic_bool m_painter_images_ready = false;

        std::unique_ptr<painter::Painter> m_painter;

        static void write_r8g8b8a8(std::byte* ptr, const Vector<3, float>& rgb)
        {
                RGB8 rgb8 = make_rgb8(rgb);
                std::array<uint8_t, 4> rgba8{rgb8.red, rgb8.green, rgb8.blue, ALPHA};

                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);
                static_assert(std::span(rgba8).size_bytes() == PIXEL_SIZE);
                std::memcpy(ptr, rgba8.data(), PIXEL_SIZE);
        }

        std::array<int, N - 1> flip_vertically(std::array<int, N - 1> pixel) const
        {
                pixel[1] = m_screen_size[1] - 1 - pixel[1];
                return pixel;
        }

        void check_slice_number(long long slice_number) const
        {
                if (slice_number < 0 || slice_number >= m_slice_count)
                {
                        error("Error slice number " + to_string(slice_number) + ", slice count "
                              + to_string(m_slice_count));
                }
        }

        void normalize_pixels()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                const float max = max_value(m_pixels_original_span);
                if (max == MIN)
                {
                        m_pixel_max = std::nullopt;
                        m_pixel_coef = 1;
                        return;
                }

                m_pixel_max = max;
                const float coef = std::lerp(1 / max, 1.0f, m_pixel_brightness_parameter);
                if (m_pixel_coef == coef)
                {
                        return;
                }
                m_pixel_coef = coef;

                ASSERT(m_pixels_original.size() * PIXEL_SIZE == m_pixels_r8g8b8a8.size());

                std::byte* ptr = m_pixels_r8g8b8a8.data();
                for (const Vector<3, float>& pixel : m_pixels_original)
                {
                        if (pixel[0] != MIN)
                        {
                                write_r8g8b8a8(ptr, pixel * coef);
                        }
                        ptr += PIXEL_SIZE;
                }
                ASSERT(ptr == m_pixels_r8g8b8a8.data() + m_pixels_r8g8b8a8.size());
        }

        // PainterNotifier

        void thread_busy(unsigned thread_number, const std::array<int, N - 1>& pixel) override
        {
                long long x = pixel[0];
                long long y = m_screen_size[1] - 1 - pixel[1];
                m_busy_indices_2d[thread_number] = y * m_screen_size[0] + x;
        }

        void thread_free(unsigned thread_number) override
        {
                m_busy_indices_2d[thread_number] = NULL_INDEX;
        }

        void pixel_set(const std::array<int, N - 1>& pixel, const Vector<3, float>& rgb) override
        {
                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);

                const std::size_t index = m_global_index.compute(flip_vertically(pixel));
                m_pixels_original[index] = rgb;
                write_r8g8b8a8(m_pixels_r8g8b8a8.data() + PIXEL_SIZE * index, rgb * m_pixel_coef);
        }

        painter::Images<N - 1>* images(long long /*pass_number*/) override
        {
                return &m_painter_images;
        }

        void pass_done(long long /*pass_number*/) override
        {
                m_painter_images_ready = true;
        }

        void error_message(const std::string& msg) override
        {
                MESSAGE_ERROR(msg);
        }

        // Pixels

        const char* floating_point_name() const override
        {
                return m_floating_point_name;
        }

        const char* color_name() const override
        {
                return m_color_name;
        }

        std::optional<float> pixel_max() const override
        {
                return m_pixel_max;
        }

        const std::vector<int>& screen_size() const override
        {
                return m_screen_size;
        }

        const std::vector<long long>& busy_indices_2d() const override
        {
                return m_busy_indices_2d;
        }

        painter::Statistics statistics() const override
        {
                return m_painter->statistics();
        }

        std::span<const std::byte> slice_r8g8b8a8(long long slice_number, float brightness_parameter) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!(brightness_parameter >= 0 && brightness_parameter <= 1))
                {
                        error("Brightness parameter " + to_string(brightness_parameter)
                              + " must be in the range [0, 1]");
                }

                check_slice_number(slice_number);

                if (time() - m_last_normalize_time >= NORMALIZE_INTERVAL
                    || m_pixel_brightness_parameter != brightness_parameter)
                {
                        m_pixel_brightness_parameter = brightness_parameter;
                        m_last_normalize_time = time();
                        normalize_pixels();
                }

                return std::span(&m_pixels_r8g8b8a8[slice_number * m_slice_size], m_slice_size);
        }

        std::optional<Images> slice(long long slice_number) const override
        {
                if (!m_painter_images_ready)
                {
                        return std::nullopt;
                }

                check_slice_number(slice_number);

                const auto slice_pixels = [&](const image::Image<N - 1>& image)
                {
                        const long long pixel_size = image::format_pixel_size_in_bytes(image.color_format);
                        const long long slice_size = pixel_size * m_screen_size[0] * m_screen_size[1];
                        const std::byte* begin = &image.pixels[slice_number * slice_size];
                        const std::byte* end = begin + slice_size;
                        return std::vector(begin, end);
                };

                painter::ImagesReading lock(&m_painter_images);

                const image::Image<N - 1>& rgb = lock.image_with_background();
                const image::Image<N - 1>& rgba = lock.image_without_background();

                ASSERT(!rgb.pixels.empty() && !rgb.pixels.empty());
                ASSERT(std::equal(rgb.size.cbegin(), rgb.size.cend(), m_screen_size.cbegin(), m_screen_size.cend()));
                ASSERT(std::equal(rgba.size.cbegin(), rgba.size.cend(), m_screen_size.cbegin(), m_screen_size.cend()));

                std::optional<Images> images(std::in_place);

                images->size = {m_screen_size[0], m_screen_size[1]};
                images->rgb.color_format = rgb.color_format;
                images->rgb.pixels = slice_pixels(rgb);
                images->rgba.color_format = rgba.color_format;
                images->rgba.pixels = slice_pixels(rgba);

                return images;
        }

        std::optional<Images> pixels() const override
        {
                if (!m_painter_images_ready)
                {
                        return std::nullopt;
                }

                painter::ImagesReading lock(&m_painter_images);

                const image::Image<N - 1>& rgb = lock.image_with_background();
                const image::Image<N - 1>& rgba = lock.image_without_background();

                ASSERT(!rgb.pixels.empty() && !rgb.pixels.empty());
                ASSERT(std::equal(rgb.size.cbegin(), rgb.size.cend(), m_screen_size.cbegin(), m_screen_size.cend()));
                ASSERT(std::equal(rgba.size.cbegin(), rgba.size.cend(), m_screen_size.cbegin(), m_screen_size.cend()));

                std::optional<Images> images(std::in_place);

                images->size = m_screen_size;
                images->rgb.color_format = rgb.color_format;
                images->rgb.pixels = rgb.pixels;
                images->rgba.color_format = rgba.color_format;
                images->rgba.pixels = rgba.pixels;

                return images;
        }

public:
        template <typename T, typename Color>
        PainterPixels(
                std::shared_ptr<const painter::Scene<N, T, Color>> scene,
                unsigned thread_count,
                int samples_per_pixel,
                bool smooth_normal)
                : m_floating_point_name(type_bit_name<T>()),
                  m_color_name(Color::name()),
                  m_global_index(scene->projector().screen_size()),
                  m_screen_size(
                          [](const auto& array)
                          {
                                  return std::vector(array.cbegin(), array.cend());
                          }(scene->projector().screen_size())),
                  m_busy_indices_2d(thread_count, NULL_INDEX),
                  m_painter(painter::create_painter(
                          this,
                          samples_per_pixel,
                          MAX_PASS_COUNT,
                          std::move(scene),
                          thread_count,
                          smooth_normal))
        {
        }

        PainterPixels(const PainterPixels&) = delete;
        PainterPixels& operator=(const PainterPixels&) = delete;
        PainterPixels(PainterPixels&&) = delete;
        PainterPixels& operator=(PainterPixels&&) = delete;

        ~PainterPixels() override
        {
                m_painter.reset();
        }
};
}
