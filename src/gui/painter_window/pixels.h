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

        virtual const std::vector<int>& screen_size() const = 0;

        virtual std::span<const std::byte> slice_r8g8b8a8(long long slice_number) = 0;
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

        const std::thread::id m_thread_id = std::this_thread::get_id();

        const char* const m_floating_point_name;
        const char* const m_color_name;

        const GlobalIndex<N - 1, long long> m_global_index;
        const std::vector<int> m_screen_size;
        const long long m_slice_count;
        const std::size_t m_slice_size = PIXEL_SIZE * m_screen_size[0] * m_screen_size[1];
        std::vector<long long> m_busy_indices_2d;

        std::vector<std::byte> m_pixels_r8g8b8a8 = make_initial_image(m_screen_size, COLOR_FORMAT);

        std::vector<std::optional<Vector<3, float>>> m_pixels_original{
                static_cast<std::size_t>(m_global_index.count())};
        float m_pixel_max_r = 1;

        TimePoint m_last_normalize_time = time();

        painter::Images<N - 1> m_painter_images;
        std::atomic_bool m_painter_images_ready = false;

        std::unique_ptr<painter::Painter> m_painter;

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
                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);

                const float max = [this]
                {
                        Vector<3, float> m;
                        m[0] = limits<float>::lowest();
                        m[1] = limits<float>::lowest();
                        m[2] = limits<float>::lowest();
                        for (const std::optional<Vector<3, float>>& pixel : m_pixels_original)
                        {
                                if (pixel)
                                {
                                        for (std::size_t i = 0; i < 3; ++i)
                                        {
                                                if (is_finite((*pixel)[i]))
                                                {
                                                        m[i] = std::max(m[i], (*pixel)[i]);
                                                }
                                        }
                                }
                        }
                        return std::max(std::max(m[0], m[1]), m[2]);
                }();

                if (!(max > 0 && max != 1))
                {
                        return;
                }

                const float max_r = 1 / max;
                m_pixel_max_r = max_r;

                std::byte* ptr = m_pixels_r8g8b8a8.data();
                ASSERT(m_pixels_r8g8b8a8.size() % PIXEL_SIZE == 0);
                for (std::size_t i = 0; i < m_pixels_original.size(); ++i)
                {
                        if (m_pixels_original[i])
                        {
                                std::array<uint8_t, 4> rgba8;
                                static_assert(std::span(rgba8).size_bytes() == PIXEL_SIZE);
                                RGB8 rgb8 = make_rgb8(*m_pixels_original[i] * max_r);
                                rgba8[0] = rgb8.red;
                                rgba8[1] = rgb8.green;
                                rgba8[2] = rgb8.blue;
                                std::memcpy(ptr, rgba8.data(), PIXEL_SIZE);
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

                static constexpr uint8_t ALPHA = limits<uint8_t>::max();

                const long long index = m_global_index.compute(flip_vertically(pixel));

                m_pixels_original[index] = rgb;
                const RGB8 rgb8 = make_rgb8(rgb * m_pixel_max_r);
                const std::array<uint8_t, 4> rgba8{rgb8.red, rgb8.green, rgb8.blue, ALPHA};
                static_assert(std::span(rgba8).size_bytes() == PIXEL_SIZE);
                std::memcpy(&m_pixels_r8g8b8a8[PIXEL_SIZE * index], rgba8.data(), PIXEL_SIZE);
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

        std::span<const std::byte> slice_r8g8b8a8(long long slice_number) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                check_slice_number(slice_number);

                if (time() - m_last_normalize_time >= NORMALIZE_INTERVAL)
                {
                        normalize_pixels();
                        m_last_normalize_time = time();
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
                  m_slice_count(m_global_index.count() / m_screen_size[0] / m_screen_size[1]),
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
