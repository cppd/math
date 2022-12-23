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
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/image/file_save.h>
#include <src/numerical/vector.h>
#include <src/settings/dimensions.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <numeric>
#include <random>
#include <vector>

namespace ns::noise
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
class Noise final
{
        Vector<N, T> vector_;
        std::array<int, 2> indices_;

public:
        Noise()
        {
                PCG pcg(N * 12345);

                static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
                // using double to make identical numbers for different types
                std::uniform_real_distribution<double> urd(-10, 10);
                for (std::size_t i = 0; i < N; ++i)
                {
                        vector_[i] = static_cast<T>(urd(pcg));
                }

                std::array<int, N> indices;
                std::iota(indices.begin(), indices.end(), 0);
                std::sample(indices.cbegin(), indices.cend(), indices_.begin(), indices_.size(), pcg);
        }

        [[nodiscard]] T compute(const int i, const int j)
        {
                Vector<N, T> p = vector_;
                p[indices_[0]] = noise_coordinate<T>(i);
                p[indices_[1]] = noise_coordinate<T>(j);
                return noise(p);
        }
};

template <std::size_t N, typename T>
void test()
{
        Noise<N, T> noise;

        std::vector<float> pixels(static_cast<unsigned long long>(IMAGE_SIZE) * IMAGE_SIZE);

        unsigned long long index = -1;
        for (int i = 0; i < IMAGE_SIZE; ++i)
        {
                for (int j = 0; j < IMAGE_SIZE; ++j)
                {
                        const T n = noise.compute(i, j);
                        if (!(n > T{-1.001} && n < T{1.001}))
                        {
                                error("Noise value " + to_string(n) + " is not in the range [-1, 1]");
                        }
                        pixels[++index] = (1 + n) / 2;
                }
        }

        const std::string file_name = std::string(FILE_NAME) + '_' + std::to_string(N) + "d_" + type_name<T>();

        image::save(
                std::filesystem::temp_directory_path() / path_from_utf8(file_name),
                image::ImageView<2>(
                        {IMAGE_SIZE, IMAGE_SIZE}, image::ColorFormat::R32,
                        std::as_bytes(std::span(pixels.cbegin(), pixels.cend()))));
}

template <std::size_t... I>
void test(std::index_sequence<I...>&&)
{
        (test<I, float>(), ...);
        (test<I, double>(), ...);
}

void test_noise()
{
        LOG("Test noise");
        test(settings::Dimensions2());
        LOG("Test noise passed");
}

TEST_SMALL("Noise", test_noise)
}
}
