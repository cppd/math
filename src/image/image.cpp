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
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/interpolation.h>
#include <src/com/type/limit.h>

#include <algorithm>

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
        resize(size);

        color::format_conversion(
                color_format, pixels, ColorFormat::R32G32B32,
                Span<std::byte>(reinterpret_cast<std::byte*>(data_pointer(m_data)), data_size(m_data)));
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
        if (!std::all_of(size.cbegin(), size.cend(), [](int v) { return v > 0; }))
        {
                error("Error image size " + to_string(size));
        }

        if (m_size == size)
        {
                return;
        }

        m_size = size;

        for (unsigned i = 0; i < N; ++i)
        {
                m_max[i] = m_size[i] - 1;
        }

        m_global_index = decltype(m_global_index)(m_size);

        m_data.clear();
        m_data.shrink_to_fit();
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
        // Vulkan: Texel Coordinate Systems, Wrapping Operation.

        std::array<int, N> x0;
        std::array<int, N> x1;
        std::array<T, N> x;

        for (unsigned i = 0; i < N; ++i)
        {
                T v = p[i] * m_size[i] - T(0.5);
                T floor = std::floor(v);

                x[i] = v - floor;
                x0[i] = floor;
                x1[i] = x0[i] + 1;

                if ((true))
                {
                        // wrap: clamp to edge
                        x0[i] = std::clamp(x0[i], 0, m_max[i]);
                        x1[i] = std::clamp(x1[i], 0, m_max[i]);
                }
                else
                {
                        // wrap: repeate
                        x0[i] = x0[i] % m_size[i];
                        x1[i] = x1[i] % m_size[i];
                }
        }

        std::array<Color, (1 << N)> pixels;

        for (unsigned i = 0; i < pixels.size(); ++i)
        {
                long long index = 0;
                for (unsigned n = 0; n < N; ++n)
                {
                        int coordinate = ((1 << n) & i) ? x1[n] : x0[n];
                        index += m_global_index.stride(n) * coordinate;
                }
                pixels[i] = m_data[index];
        }

        return interpolation(pixels, x);
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
