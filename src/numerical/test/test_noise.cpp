/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../noise.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/image/file_save.h>
#include <src/test/test.h>

#include <filesystem>
#include <vector>

namespace ns::numerical
{
namespace
{
constexpr int IMAGE_SIZE = 500;
template <typename T>
constexpr T NOISE_SIZE = 10;
constexpr std::string_view FILE_NAME = "noise";

template <typename T>
T noise_coordinate(const int x)
{
        static constexpr int CENTER = IMAGE_SIZE / 2;
        return (x - CENTER) * (NOISE_SIZE<T> / IMAGE_SIZE);
}

template <std::size_t N, typename T>
        requires (N == 2)
T compute_noise(const int i, const int j)
{
        Vector<2, T> p;
        p[0] = noise_coordinate<T>(i);
        p[1] = noise_coordinate<T>(j);
        return noise(p);
}

template <std::size_t N, typename T>
        requires (N == 3)
T compute_noise(const int i, const int j)
{
        Vector<3, T> p;
        p[0] = noise_coordinate<T>(i);
        p[1] = 3;
        p[2] = noise_coordinate<T>(j);
        return noise(p);
}

template <std::size_t N, typename T>
        requires (N == 4)
T compute_noise(const int i, const int j)
{
        Vector<4, T> p;
        p[0] = 2;
        p[1] = noise_coordinate<T>(i);
        p[2] = -5;
        p[3] = noise_coordinate<T>(j);
        return noise(p);
}

template <std::size_t N, typename T>
void test()
{
        std::vector<float> pixels(static_cast<unsigned long long>(IMAGE_SIZE) * IMAGE_SIZE);

        unsigned long long index = -1;
        for (int i = 0; i < IMAGE_SIZE; ++i)
        {
                for (int j = 0; j < IMAGE_SIZE; ++j)
                {
                        const T n = compute_noise<N, T>(i, j);
                        if (!(n > T{-1.01} && n < T{1.01}))
                        {
                                error("Noise value " + to_string(n) + " is not in the range [-1, 1]");
                        }
                        pixels[++index] = (1 + n) / 2;
                }
        }

        image::save(
                std::filesystem::temp_directory_path()
                        / path_from_utf8(std::string(FILE_NAME) + '_' + std::to_string(N)),
                image::ImageView<2>(
                        {IMAGE_SIZE, IMAGE_SIZE}, image::ColorFormat::R32,
                        std::as_bytes(std::span(pixels.cbegin(), pixels.cend()))));
}

template <typename T>
void test()
{
        test<2, T>();
        test<3, T>();
        test<4, T>();
}

void test_noise()
{
        LOG("Test noise");
        test<float>();
        test<double>();
        LOG("Test noise passed");
}

TEST_SMALL("Noise", test_noise)
}
}
