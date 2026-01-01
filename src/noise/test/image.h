/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/image/file_save.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/numerical/vector.h>
#include <src/settings/directory.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ns::noise::test
{
namespace image_implementation
{
template <std::size_t N, typename T>
class NoiseImage final
{
        int center_;
        T ratio_;

        numerical::Vector<N, T> vector_;
        std::array<int, 2> indices_;

        [[nodiscard]] T noise_coordinate(const int x) const
        {
                return (x - center_) * ratio_;
        }

public:
        NoiseImage(const int image_size, const T noise_size)
                : center_(image_size / 2),
                  ratio_(noise_size / image_size)
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

        template <typename F>
        [[nodiscard]] T compute(const int i, const int j, const F& noise) const
        {
                numerical::Vector<N, T> p = vector_;
                p[indices_[0]] = noise_coordinate(i);
                p[indices_[1]] = noise_coordinate(j);
                return noise(p);
        }
};
}

template <std::size_t N, typename T>
void make_noise_image(
        const std::string_view file_name,
        const int image_size,
        const std::type_identity_t<T> noise_size,
        T (*noise)(const numerical::Vector<N, T>&))
{
        const image_implementation::NoiseImage<N, T> noise_image(image_size, noise_size);

        std::vector<float> pixels(static_cast<unsigned long long>(image_size) * image_size);

        unsigned long long index = -1;
        float max = -Limits<float>::infinity();
        for (int i = 0; i < image_size; ++i)
        {
                for (int j = 0; j < image_size; ++j)
                {
                        const float n = noise_image.compute(i, j, noise);
                        if (!(n > -1.001f && n < 1.001f))
                        {
                                error("Noise value " + to_string(n) + " is not in the range [-1, 1]");
                        }
                        max = std::max(max, std::abs(n));
                        pixels[++index] = n;
                }
        }

        ASSERT(max >= 0);
        for (float& pixel : pixels)
        {
                pixel = (1 + pixel / max) / 2;
        }

        const std::string name = std::string(file_name) + '_' + std::to_string(N) + "d_" + type_name<T>();
        const std::span bytes = std::as_bytes(std::span(pixels.cbegin(), pixels.cend()));

        image::save(
                settings::test_path(name),
                image::ImageView<2>({image_size, image_size}, image::ColorFormat::R32, bytes));
}
}
