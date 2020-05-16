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

#include "image.h"

#include "file.h"

#include <src/color/format.h>
#include <src/com/alg.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/interpolation.h>
#include <src/utility/string/str.h>

#include <algorithm>

namespace
{
#if 0
// В зависимости от типа данных и настроек компилятора, работает как в разы
// быстрее варианта value - std::floor(value), так и в разы медленнее.
template <typename T>
T wrap_coordinate_repeate(T value)
{
        T r = value - static_cast<int>(value);
        return (r >= 0) ? r : 1 + r;
}
#else
template <typename T>
T wrap_coordinate_repeate(T value)
{
        return value - std::floor(value);
}
#endif

#if 0
template <typename T>
T wrap_coordinate_clamp_to_edge(T value)
{
        return std::clamp(value, static_cast<T>(0), static_cast<T>(1));
}
#endif
}

template <size_t N>
std::array<int, N> Image<N>::initial_value()
{
        std::array<int, N> v;
        v.fill(limits<int>::lowest());
        return v;
}

template <size_t N>
Image<N>::Image(const std::array<int, N>& size)
{
        resize(size);
}

template <size_t N>
Image<N>::Image(const std::array<int, N>& size, ColorFormat color_format, const std::vector<std::byte>& pixels)
{
        load_from_pixels(size, color_format, pixels);
}

template <size_t N>
template <size_t X>
Image<N>::Image(std::enable_if_t<X == 2, const std::string&> file_name)
{
        static_assert(N == X);

        read_from_file(file_name);
}

template <size_t N>
void Image<N>::resize(const std::array<int, N>& size)
{
        ASSERT(std::all_of(size.cbegin(), size.cend(), [](int v) { return v > 0; }));

        if (m_size == size)
        {
                return;
        }

        for (int v : size)
        {
                if (v < 2)
                {
                        error("Image size is less than 2");
                }
        }

        m_data.clear();
        m_data.shrink_to_fit();

        m_size = size;

        for (unsigned i = 0; i < N; ++i)
        {
                m_max[i] = m_size[i] - 1;
                m_max_0[i] = m_size[i] - 2;
        }

        m_global_index = decltype(m_global_index)(m_size);

        // Смещения для следующих элементов от заданного с сортировкой по возрастанию
        // от измерения с максимальным номером к измерению с минимальным номером.
        // Пример для двух измерений:
        // (x    , y    ) = 0
        // (x + 1, y    ) = 1
        // (x    , y + 1) = width
        // (x + 1, y + 1) = width + 1
        for (unsigned i = 0; i < (1 << N); ++i)
        {
                long long offset_index = 0;
                for (unsigned n = 0; n < N; ++n)
                {
                        if ((1 << n) & i)
                        {
                                offset_index += m_global_index.stride(n);
                        }
                }
                m_pixel_offsets[i] = offset_index;
        }

        m_data.resize(m_global_index.count());
}

template <size_t N>
bool Image<N>::empty() const
{
        return m_data.empty();
}

template <size_t N>
long long Image<N>::pixel_index(const std::array<int, N>& p) const
{
        return m_global_index.compute(p);
}

template <size_t N>
template <typename T>
Color Image<N>::texture(const Vector<N, T>& p) const
{
        std::array<int, N> x0;
        std::array<T, N> local_x;

        for (unsigned i = 0; i < N; ++i)
        {
                T x = wrap_coordinate_repeate(p[i]) * m_max[i];

                x0[i] = static_cast<int>(x);

                // Если значение x[i] равно максимуму (это целое число), то x0[i] получится
                // неправильным, поэтому требуется корректировка для этого случая
                x0[i] = std::min(x0[i], m_max_0[i]);

                local_x[i] = x - x0[i];
        }

        long long index = pixel_index(x0);

        std::array<Color, (1 << N)> pixels;
        pixels[0] = m_data[index]; // в соответствии с равенством index + m_pixel_offsets[0] == index
        for (unsigned i = 1; i < pixels.size(); ++i)
        {
                pixels[i] = m_data[index + m_pixel_offsets[i]];
        }

        return interpolation(pixels, local_x);
}

template <size_t N>
void Image<N>::load_from_pixels(
        const std::array<int, N>& size,
        ColorFormat color_format,
        const std::vector<std::byte>& pixels)
{
        if (color_format != ColorFormat::R8G8B8A8_SRGB)
        {
                error("Image format " + color::format_to_string(color_format)
                      + " is not supported for loading image from pixels");
        }

        if (4ull * multiply_all<long long>(size) != pixels.size())
        {
                error("Image size error for RGBA pixels");
        }

        resize(size);

        for (size_t i = 0, p = 0; i < m_data.size(); p += 4, ++i)
        {
                std::array<uint8_t, 3> rgb;
                std::memcpy(rgb.data(), &pixels[p], rgb.size());
                m_data[i] = Srgb8(rgb[0], rgb[1], rgb[2]);
        }
}

template <size_t N>
void Image<N>::set_pixel(const std::array<int, N>& p, const Color& color)
{
        m_data[pixel_index(p)] = color;
}

template <size_t N>
template <size_t X>
std::enable_if_t<X == 2> Image<N>::read_from_file(const std::string& file_name)
{
        static_assert(N == X);

        std::array<int, N> size;

        ColorFormat color_format;
        std::vector<std::byte> pixels;

        load_image_from_file_rgb(file_name, &size[0], &size[1], &color_format, &pixels);

        resize(size);

        color::format_conversion(
                color_format, pixels, ColorFormat::R32G32B32,
                Span<std::byte>(reinterpret_cast<std::byte*>(data_pointer(m_data)), data_size(m_data)));
}

template <size_t N>
template <size_t X>
std::enable_if_t<X == 2> Image<N>::write_to_file(const std::string& file_name) const
{
        static_assert(N == X);

        if (empty())
        {
                error("No data to write the image to the file " + file_name);
        }

        save_image_to_file(
                file_name, m_size[0], m_size[1], ColorFormat::R32G32B32,
                Span<const std::byte>(reinterpret_cast<const std::byte*>(data_pointer(m_data)), data_size(m_data)));
}

// Текстурные координаты могут отсчитываться снизу, поэтому нужна эта функция
template <size_t N>
template <size_t X>
std::enable_if_t<X == 2> Image<N>::flip_vertically()
{
        static_assert(N == X);

        int width = m_size[0];
        int height = m_size[1];

        for (int y1 = 0, y2 = height - 1; y1 < height / 2; ++y1, --y2)
        {
                for (int x = 0; x < width; ++x)
                {
                        std::array<int, 2> p1{x, y1};
                        std::array<int, 2> p2{x, y2};
                        std::swap(m_data[pixel_index(p1)], m_data[pixel_index(p2)]);
                }
        }
}

template class Image<2>;
template class Image<3>;
template class Image<4>;
template class Image<5>;

template Color Image<2>::texture(const Vector<2, float>& p) const;
template Color Image<3>::texture(const Vector<3, float>& p) const;
template Color Image<4>::texture(const Vector<4, float>& p) const;
template Color Image<5>::texture(const Vector<5, float>& p) const;

template Color Image<2>::texture(const Vector<2, double>& p) const;
template Color Image<3>::texture(const Vector<3, double>& p) const;
template Color Image<4>::texture(const Vector<4, double>& p) const;
template Color Image<5>::texture(const Vector<5, double>& p) const;

template Image<2>::Image(const std::string& file_name);
template void Image<2>::read_from_file(const std::string& file_name);
template void Image<2>::write_to_file(const std::string& file_name) const;
template void Image<2>::flip_vertically();
