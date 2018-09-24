/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/color/conversion.h"
#include "com/error.h"
#include "com/file/file.h"
#include "com/file/file_sys.h"
#include "com/interpolation.h"
#include "com/string/str.h"

#include <SFML/Graphics/Image.hpp>
#include <algorithm>

namespace
{
std::string file_name_with_extension(const std::string& file_name, const char* extension)
{
        std::string ext = file_extension(file_name);

        if (ext.size() > 0)
        {
                if (ext != extension)
                {
                        error("Unsupported image file format " + ext);
                }
                return file_name;
        }

        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + "." + extension;
}

template <size_t N>
long long mul(const std::array<int, N>& size)
{
        static_assert(N >= 1);
        long long res = size[0];
        for (size_t i = 1; i < N; ++i)
        {
                res *= size[i];
        }
        return res;
}

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
Image<N>::Image(const std::array<int, N>& size)
{
        resize(size);
}

template <size_t N>
Image<N>::Image(const std::array<int, N>& size, const std::vector<unsigned char>& srgba_pixels)
{
        if (4ull * mul(size) != srgba_pixels.size())
        {
                error("Image size error for sRGBA pixels");
        }

        read_from_srgba_pixels(size, srgba_pixels.data());
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
        return m_data.size() == 0;
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
void Image<N>::read_from_srgba_pixels(const std::array<int, N>& size, const unsigned char* srgba_pixels)
{
        static_assert(std::numeric_limits<unsigned char>::digits == 8);

        resize(size);

        for (size_t i = 0, p = 0; i < m_data.size(); p += 4, ++i)
        {
                m_data[i] = SrgbInteger(srgba_pixels[p], srgba_pixels[p + 1], srgba_pixels[p + 2]);
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

        sf::Image image;

        if (!image.loadFromFile(file_name))
        {
                error("Error read image from file " + file_name);
        }

        std::array<int, N> size;

        size[0] = image.getSize().x;
        size[1] = image.getSize().y;

        read_from_srgba_pixels(size, image.getPixelsPtr());
}

// Запись в формат PPM с цветом sRGB
template <size_t N>
template <size_t X>
std::enable_if_t<X == 2> Image<N>::write_to_file(const std::string& file_name) const
{
        static_assert(N == X);

        if (empty())
        {
                error("No data to write the image to the file " + file_name);
        }

        CFile fp(file_name_with_extension(file_name, "ppm"), "wb");

        long long width = m_size[0];
        long long height = m_size[1];
        if (fprintf(fp, "P6\n%lld %lld\n255\n", width, height) <= 0)
        {
                error("Error writing image header");
        }

        std::vector<unsigned char> buffer(m_data.size() * 3);

        for (size_t i = 0, buf = 0; i < m_data.size(); ++i)
        {
                buffer[buf++] = color_conversion::rgb_float_to_srgb_integer(m_data[i].red());
                buffer[buf++] = color_conversion::rgb_float_to_srgb_integer(m_data[i].green());
                buffer[buf++] = color_conversion::rgb_float_to_srgb_integer(m_data[i].blue());
        }

        if (fwrite(buffer.data(), sizeof(buffer[0]), buffer.size(), fp) != buffer.size())
        {
                error("Error writing image data");
        }
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
