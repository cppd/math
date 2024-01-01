/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/color/rgb8.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/com/min_max.h>
#include <src/com/print.h>
#include <src/com/spinlock.h>
#include <src/com/type/limit.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/numerical/vector.h>
#include <src/painter/painter.h>
#include <src/painter/scenes/storage.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ns::gui::painter_window
{
class Pixels
{
public:
        struct Image final
        {
                image::ColorFormat color_format;
                std::vector<std::byte> pixels;
        };

        struct Images final
        {
                std::vector<int> size;
                Image rgb;
                Image rgba;
        };

        virtual ~Pixels() = default;

        [[nodiscard]] virtual std::optional<float> pixel_max() const = 0;

        [[nodiscard]] virtual const std::vector<int>& screen_size() const = 0;

        virtual void set_brightness_parameter(float brightness_parameter) = 0;
        [[nodiscard]] virtual std::span<const std::byte> slice_r8g8b8a8(long long slice_number) const = 0;
        [[nodiscard]] virtual const std::vector<long long>& busy_indices_2d() const = 0;
        [[nodiscard]] virtual painter::Statistics statistics() const = 0;

        [[nodiscard]] virtual std::optional<Images> slice(long long slice_number) const = 0;
        [[nodiscard]] virtual std::optional<Images> pixels() const = 0;
};

template <std::size_t N, typename T, typename Color>
class PainterPixels final : public Pixels, public painter::Notifier<N - 1>
{
        static_assert(N >= 3);

        static constexpr std::chrono::seconds NORMALIZE_INTERVAL{10};

        static constexpr std::optional<int> MAX_PASS_COUNT = std::nullopt;
        static constexpr long long NULL_INDEX = -1;

        static constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R8G8B8A8_SRGB;
        static constexpr std::size_t PIXEL_SIZE = image::format_pixel_size_in_bytes(COLOR_FORMAT);
        static constexpr std::uint8_t ALPHA = Limits<std::uint8_t>::max();

        static constexpr float MIN = Limits<float>::lowest();

        const painter::scenes::StorageScene<N, T, Color> scene_;

        const std::thread::id thread_id_ = std::this_thread::get_id();

        const GlobalIndex<N - 1, long long> global_index_;
        const std::vector<int> screen_size_;
        const long long slice_count_ = global_index_.count() / screen_size_[0] / screen_size_[1];
        const std::size_t slice_size_ = PIXEL_SIZE * screen_size_[0] * screen_size_[1];

        std::vector<long long> busy_indices_2d_;

        std::vector<std::byte> pixels_r8g8b8a8_{make_initial_image(screen_size_, COLOR_FORMAT)};
        std::vector<Vector<3, float>> pixels_original_{
                static_cast<std::size_t>(global_index_.count()), Vector<3, float>(MIN)};
        std::vector<Spinlock> pixels_lock_{pixels_original_.size()};
        std::atomic<float> pixels_coef_ = 1;

        static_assert(std::atomic<float>::is_always_lock_free);
        std::atomic<float> pixel_brightness_parameter_ = 0;
        std::atomic<float> pixel_max_ = MIN;

        painter::Images<N - 1> painter_images_;
        std::atomic_bool painter_images_ready_ = false;

        std::unique_ptr<painter::Painter> painter_;

        std::atomic_bool normalize_stop_ = false;
        std::thread normalize_thread_;

        static void write_r8g8b8a8(std::byte* const ptr, const Vector<3, float>& rgb)
        {
                const color::RGB8 rgb8 = color::make_rgb8(rgb);
                const std::array<std::uint8_t, 4> rgba8{rgb8.red(), rgb8.green(), rgb8.blue(), ALPHA};

                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);
                static_assert(std::span(rgba8).size_bytes() == PIXEL_SIZE);
                std::memcpy(ptr, rgba8.data(), PIXEL_SIZE);
        }

        [[nodiscard]] std::array<int, N - 1> flip_vertically(std::array<int, N - 1> pixel) const
        {
                pixel[1] = screen_size_[1] - 1 - pixel[1];
                return pixel;
        }

        void check_slice_number(const long long slice_number) const
        {
                if (slice_number < 0 || slice_number >= slice_count_)
                {
                        error("Error slice number " + to_string(slice_number) + ", slice count "
                              + to_string(slice_count_));
                }
        }

        void normalize_pixels(const float coef)
        {
                ASSERT(pixels_original_.size() * PIXEL_SIZE == pixels_r8g8b8a8_.size());
                std::byte* ptr = pixels_r8g8b8a8_.data();
                for (std::size_t i = 0; i < pixels_original_.size(); ++i, ptr += PIXEL_SIZE)
                {
                        const std::lock_guard lg(pixels_lock_[i]);
                        const Vector<3, float>& pixel = pixels_original_[i];
                        if (pixel[0] != MIN)
                        {
                                write_r8g8b8a8(ptr, pixel * coef);
                        }
                }
                ASSERT(ptr == pixels_r8g8b8a8_.data() + pixels_r8g8b8a8_.size());
        }

        void normalize_pixels()
        {
                ASSERT(std::this_thread::get_id() != thread_id_);

                Clock::time_point last_normalize_time = Clock::now();
                float brightness_parameter = -1;

                while (!normalize_stop_.load(std::memory_order_relaxed))
                {
                        if (Clock::now() - last_normalize_time < NORMALIZE_INTERVAL
                            && brightness_parameter == pixel_brightness_parameter_.load(std::memory_order_relaxed))
                        {
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                continue;
                        }

                        brightness_parameter = pixel_brightness_parameter_.load(std::memory_order_relaxed);
                        last_normalize_time = Clock::now();

                        static_assert(sizeof(pixels_original_[0]) == 3 * sizeof(float));
                        const float max = max_value(
                                std::span<const float>(pixels_original_[0].data(), 3 * pixels_original_.size()));
                        if (max == MIN)
                        {
                                continue;
                        }
                        pixel_max_.store(max, std::memory_order_relaxed);

                        const float coef = std::lerp(1 / max, 1.0f, brightness_parameter);
                        if (coef == pixels_coef_.load(std::memory_order_relaxed))
                        {
                                continue;
                        }
                        pixels_coef_.store(coef, std::memory_order_relaxed);

                        normalize_pixels(coef);
                }
        }

        // PainterNotifier

        void thread_busy(const unsigned thread_number, const std::array<int, N - 1>& pixel) override
        {
                const long long x = pixel[0];
                const long long y = screen_size_[1] - 1 - pixel[1];
                busy_indices_2d_[thread_number] = y * screen_size_[0] + x;
        }

        void thread_free(const unsigned thread_number) override
        {
                busy_indices_2d_[thread_number] = NULL_INDEX;
        }

        void pixel_set(const std::array<int, N - 1>& pixel, const Vector<3, float>& rgb) override
        {
                static_assert(COLOR_FORMAT == image::ColorFormat::R8G8B8A8_SRGB);

                const std::size_t index = global_index_.compute(flip_vertically(pixel));
                std::byte* const ptr = pixels_r8g8b8a8_.data() + PIXEL_SIZE * index;

                const std::lock_guard lg(pixels_lock_[index]);
                pixels_original_[index] = rgb;
                write_r8g8b8a8(ptr, rgb * pixels_coef_.load(std::memory_order_relaxed));
        }

        painter::Images<N - 1>* images(const long long /*pass_number*/) override
        {
                return &painter_images_;
        }

        void pass_done(const long long /*pass_number*/) override
        {
                painter_images_ready_ = true;
        }

        void error_message(const std::string& msg) override
        {
                message_error(msg);
        }

        // Pixels

        [[nodiscard]] std::optional<float> pixel_max() const override
        {
                const float max = pixel_max_.load(std::memory_order_relaxed);
                if (max != MIN)
                {
                        return max;
                }
                return std::nullopt;
        }

        [[nodiscard]] const std::vector<int>& screen_size() const override
        {
                return screen_size_;
        }

        [[nodiscard]] const std::vector<long long>& busy_indices_2d() const override
        {
                return busy_indices_2d_;
        }

        [[nodiscard]] painter::Statistics statistics() const override
        {
                return painter_->statistics();
        }

        void set_brightness_parameter(const float brightness_parameter) override
        {
                if (!(brightness_parameter >= 0 && brightness_parameter <= 1))
                {
                        error("Brightness parameter " + to_string(brightness_parameter)
                              + " must be in the range [0, 1]");
                }

                pixel_brightness_parameter_.store(brightness_parameter, std::memory_order_relaxed);
        }

        [[nodiscard]] std::span<const std::byte> slice_r8g8b8a8(const long long slice_number) const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                check_slice_number(slice_number);

                return {&pixels_r8g8b8a8_[slice_number * slice_size_], slice_size_};
        }

        [[nodiscard]] std::optional<Images> slice(const long long slice_number) const override
        {
                if (!painter_images_ready_)
                {
                        return std::nullopt;
                }

                check_slice_number(slice_number);

                const auto slice_pixels = [&](const image::Image<N - 1>& image)
                {
                        const long long pixel_size = image::format_pixel_size_in_bytes(image.color_format);
                        const long long slice_size = pixel_size * screen_size_[0] * screen_size_[1];
                        const std::byte* const begin = &image.pixels[slice_number * slice_size];
                        const std::byte* const end = begin + slice_size;
                        return std::vector(begin, end);
                };

                const painter::ImagesReading lock(&painter_images_);

                const image::Image<N - 1>& rgb = lock.image_with_background();
                const image::Image<N - 1>& rgba = lock.image_without_background();

                ASSERT(!rgb.pixels.empty() && !rgb.pixels.empty());
                ASSERT(std::equal(rgb.size.cbegin(), rgb.size.cend(), screen_size_.cbegin(), screen_size_.cend()));
                ASSERT(std::equal(rgba.size.cbegin(), rgba.size.cend(), screen_size_.cbegin(), screen_size_.cend()));

                std::optional<Images> images(std::in_place);

                images->size = {screen_size_[0], screen_size_[1]};
                images->rgb.color_format = rgb.color_format;
                images->rgb.pixels = slice_pixels(rgb);
                images->rgba.color_format = rgba.color_format;
                images->rgba.pixels = slice_pixels(rgba);

                return images;
        }

        [[nodiscard]] std::optional<Images> pixels() const override
        {
                if (!painter_images_ready_)
                {
                        return std::nullopt;
                }

                const painter::ImagesReading lock(&painter_images_);

                const image::Image<N - 1>& rgb = lock.image_with_background();
                const image::Image<N - 1>& rgba = lock.image_without_background();

                ASSERT(!rgb.pixels.empty() && !rgb.pixels.empty());
                ASSERT(std::equal(rgb.size.cbegin(), rgb.size.cend(), screen_size_.cbegin(), screen_size_.cend()));
                ASSERT(std::equal(rgba.size.cbegin(), rgba.size.cend(), screen_size_.cbegin(), screen_size_.cend()));

                std::optional<Images> images(std::in_place);

                images->size = screen_size_;
                images->rgb.color_format = rgb.color_format;
                images->rgb.pixels = rgb.pixels;
                images->rgba.color_format = rgba.color_format;
                images->rgba.pixels = rgba.pixels;

                return images;
        }

public:
        PainterPixels(
                painter::scenes::StorageScene<N, T, Color>&& scene,
                const painter::Integrator integrator,
                const unsigned thread_count,
                const int samples_per_pixel,
                const bool flat_shading)
                : scene_(std::move(scene)),
                  global_index_(scene_.scene->projector().screen_size()),
                  screen_size_(
                          [](const auto& array)
                          {
                                  return std::vector(array.cbegin(), array.cend());
                          }(scene_.scene->projector().screen_size())),
                  busy_indices_2d_(thread_count, NULL_INDEX),
                  painter_(painter::create_painter(
                          integrator,
                          this,
                          samples_per_pixel,
                          MAX_PASS_COUNT,
                          scene_.scene.get(),
                          thread_count,
                          flat_shading))
        {
                normalize_thread_ = std::thread(
                        [this]
                        {
                                normalize_pixels();
                        });
        }

        PainterPixels(const PainterPixels&) = delete;
        PainterPixels& operator=(const PainterPixels&) = delete;
        PainterPixels(PainterPixels&&) = delete;
        PainterPixels& operator=(PainterPixels&&) = delete;

        ~PainterPixels() override
        {
                normalize_stop_.store(true, std::memory_order_relaxed);
                normalize_thread_.join();
                painter_.reset();
        }
};
}
