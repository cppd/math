/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "painter_window.h"

#include "com/alg.h"
#include "com/color/conversion.h"
#include "com/error.h"

#include <algorithm>

constexpr int PANTBRUSH_WIDTH = 20;

constexpr QRgb DEFAULT_COLOR_LIGHT = qRgb(100, 150, 200);
constexpr QRgb DEFAULT_COLOR_DARK = qRgb(0, 0, 0);

namespace
{
void initial_picture(int width, int height, std::vector<quint32>* data)
{
        static_assert(sizeof(QRgb) == sizeof(quint32));

        unsigned slice_size = width * height;

        ASSERT(data->size() >= slice_size);
        ASSERT(data->size() % slice_size == 0);

        long long slice_count = data->size() / slice_size;
        long long index = 0;
        for (long long slice = 0; slice < slice_count; ++slice)
        {
                for (int y = 0; y < height; ++y)
                {
                        for (int x = 0; x < width; ++x)
                        {
                                quint32 v = ((x + y) & 1) ? DEFAULT_COLOR_LIGHT : DEFAULT_COLOR_DARK;
                                (*data)[index] = v;
                                ++index;
                        }
                }
        }

        ASSERT(index == static_cast<long long>(data->size()));
}

template <size_t N, typename T>
std::vector<T> array_to_vector(const std::array<T, N>& array)
{
        std::vector<T> vector(N);
        std::copy(array.cbegin(), array.cend(), vector.begin());
        return vector;
}
}

template <size_t N, typename T>
long long PainterWindow<N, T>::pixel_index(const std::array<int_least16_t, N_IMAGE>& pixel) const
{
        return m_global_index.compute(pixel);
}

template <size_t N, typename T>
long long PainterWindow<N, T>::offset_for_slider_positions(const std::vector<int>& slider_positions) const
{
        ASSERT(slider_positions.size() + 2 == N_IMAGE);

        std::array<int_least16_t, N_IMAGE> pixel;

        pixel[0] = 0;
        pixel[1] = 0;

        for (unsigned i = 0; i < slider_positions.size(); ++i)
        {
                int dimension = i + 2;
                pixel[dimension] = slider_positions[i];

                ASSERT(pixel[dimension] >= 0 && pixel[dimension] < m_paint_objects->projector().screen_size()[dimension]);
        }

        return pixel_index(pixel);
}

template <size_t N, typename T>
void PainterWindow<N, T>::painter_statistics(long long* pass_count, long long* pixel_count, long long* ray_count,
                                             long long* sample_count, double* previous_pass_duration) const
{
        m_paintbrush.statistics(pass_count, pixel_count, ray_count, sample_count, previous_pass_duration);
}

template <size_t N, typename T>
void PainterWindow<N, T>::slider_positions_change_event(const std::vector<int>& slider_positions)
{
        m_slice_offset = offset_for_slider_positions(slider_positions);
}

template <size_t N, typename T>
const quint32* PainterWindow<N, T>::pixel_pointer(bool show_threads) const
{
        return (show_threads ? m_data.data() : m_data_clean.data()) + m_slice_offset;
}

template <size_t N, typename T>
void PainterWindow<N, T>::painter_pixel_before(const std::array<int_least16_t, N_IMAGE>& pixel)
{
        std::array<int_least16_t, N_IMAGE> p = pixel;
        p[1] = m_height - 1 - pixel[1];

        mark_pixel_busy(pixel_index(p));
}

template <size_t N, typename T>
void PainterWindow<N, T>::painter_pixel_after(const std::array<int_least16_t, N_IMAGE>& pixel, const Color& color)
{
        std::array<int_least16_t, N_IMAGE> p = pixel;
        p[1] = m_height - 1 - pixel[1];

        set_pixel(pixel_index(p), color);
}

template <size_t N, typename T>
void PainterWindow<N, T>::painter_error_message(const std::string& msg)
{
        PainterWindow2d::error_message(msg);
}

template <size_t N, typename T>
void PainterWindow<N, T>::mark_pixel_busy(long long index)
{
        m_data[index] ^= 0x00ff'ffffu;
}

template <size_t N, typename T>
void PainterWindow<N, T>::set_pixel(long long index, const Color& color)
{
        unsigned char r = color_conversion::rgb_float_to_srgb_uint8(color.red());
        unsigned char g = color_conversion::rgb_float_to_srgb_uint8(color.green());
        unsigned char b = color_conversion::rgb_float_to_srgb_uint8(color.blue());

        quint32 c = (r << 16u) | (g << 8u) | b;

        m_data[index] = c;
        m_data_clean[index] = c;
}

template <size_t N, typename T>
std::vector<int> PainterWindow<N, T>::initial_slider_positions()
{
        return std::vector<int>(N_IMAGE - 2, 0);
}

template <size_t N, typename T>
PainterWindow<N, T>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel, bool smooth_normal,
                                   std::unique_ptr<const PaintObjects<N, T>>&& paint_objects)
        : PainterWindow2d(title, array_to_vector(paint_objects->projector().screen_size()), initial_slider_positions()),
          m_paint_objects(std::move(paint_objects)),
          m_global_index(m_paint_objects->projector().screen_size()),
          m_height(m_paint_objects->projector().screen_size()[1]),
          m_window_thread_id(std::this_thread::get_id()),
          m_paintbrush(m_paint_objects->projector().screen_size(), PANTBRUSH_WIDTH, -1),
          m_stop(false),
          m_thread_working(false)
{
        m_slice_offset = offset_for_slider_positions(initial_slider_positions());

        m_data.resize(multiply_all<long long>(m_paint_objects->projector().screen_size()));
        initial_picture(m_paint_objects->projector().screen_size()[0], m_paint_objects->projector().screen_size()[1], &m_data);
        m_data_clean = m_data;

        m_stop = false;
        m_thread_working = true;
        m_thread = std::thread([=, this]() {
                paint(this, samples_per_pixel, *m_paint_objects, &m_paintbrush, thread_count, &m_stop, smooth_normal);
                m_thread_working = false;
        });
}

template <size_t N, typename T>
PainterWindow<N, T>::~PainterWindow()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        m_stop = true;

        if (m_thread.joinable())
        {
                m_thread.join();
        }
}

template PainterWindow<3, float>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                bool smooth_normal,
                                                std::unique_ptr<const PaintObjects<3, float>>&& paint_objects);
template PainterWindow<4, float>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                bool smooth_normal,
                                                std::unique_ptr<const PaintObjects<4, float>>&& paint_objects);
template PainterWindow<5, float>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                bool smooth_normal,
                                                std::unique_ptr<const PaintObjects<5, float>>&& paint_objects);
template PainterWindow<6, float>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                bool smooth_normal,
                                                std::unique_ptr<const PaintObjects<6, float>>&& paint_objects);

template PainterWindow<3, double>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                 bool smooth_normal,
                                                 std::unique_ptr<const PaintObjects<3, double>>&& paint_objects);
template PainterWindow<4, double>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                 bool smooth_normal,
                                                 std::unique_ptr<const PaintObjects<4, double>>&& paint_objects);
template PainterWindow<5, double>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                 bool smooth_normal,
                                                 std::unique_ptr<const PaintObjects<5, double>>&& paint_objects);
template PainterWindow<6, double>::PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                                                 bool smooth_normal,
                                                 std::unique_ptr<const PaintObjects<6, double>>&& paint_objects);
