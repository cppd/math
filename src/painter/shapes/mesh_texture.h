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

#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/interpolation.h>
#include <src/com/print.h>
#include <src/image/conversion.h>
#include <src/image/image.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <vector>

namespace ns::painter
{
template <std::size_t N>
class MeshTexture
{
        std::vector<Vector<3, float>> rgb_data_;
        std::array<int, N> size_;
        std::array<int, N> max_;
        GlobalIndex<N, long long> global_index_;

        void resize(const std::array<int, N>& size)
        {
                if (!std::all_of(
                            size.cbegin(), size.cend(),
                            [](int v)
                            {
                                    return v > 0;
                            }))
                {
                        error("Error image size " + to_string(size));
                }

                size_ = size;

                for (unsigned i = 0; i < N; ++i)
                {
                        max_[i] = size_[i] - 1;
                }

                global_index_ = decltype(global_index_)(size_);

                rgb_data_.clear();
                rgb_data_.shrink_to_fit();
                rgb_data_.resize(global_index_.count());
        }

public:
        explicit MeshTexture(const image::Image<N>& image)
        {
                resize(image.size);

                image::format_conversion(
                        image.color_format, image.pixels, image::ColorFormat::R32G32B32,
                        std::as_writable_bytes(std::span(rgb_data_.data(), rgb_data_.size())));

                for (Vector<3, float>& c : rgb_data_)
                {
                        if (!is_finite(c))
                        {
                                error("Not finite color " + to_string(c) + " in texture");
                        }
                        c[0] = std::clamp<float>(c[0], 0, 1);
                        c[1] = std::clamp<float>(c[1], 0, 1);
                        c[2] = std::clamp<float>(c[2], 0, 1);
                }
        }

        template <typename T>
        Vector<3, float> color(const Vector<N, T>& p) const
        {
                // Vulkan: Texel Coordinate Systems, Wrapping Operation.

                std::array<int, N> x0;
                std::array<int, N> x1;
                std::array<T, N> x;

                for (unsigned i = 0; i < N; ++i)
                {
                        T v = p[i] * size_[i] - T(0.5);
                        T floor = std::floor(v);

                        x[i] = v - floor;
                        x0[i] = floor;
                        x1[i] = x0[i] + 1;

                        if ((true))
                        {
                                // wrap: clamp to edge
                                x0[i] = std::clamp(x0[i], 0, max_[i]);
                                x1[i] = std::clamp(x1[i], 0, max_[i]);
                        }
                        else
                        {
                                // wrap: repeate
                                x0[i] = x0[i] % size_[i];
                                x1[i] = x1[i] % size_[i];
                        }
                }

                std::array<Vector<3, float>, (1 << N)> pixels;

                for (unsigned i = 0; i < pixels.size(); ++i)
                {
                        long long index = 0;
                        for (unsigned n = 0; n < N; ++n)
                        {
                                int coordinate = ((1 << n) & i) ? x1[n] : x0[n];
                                index += global_index_.stride(n) * coordinate;
                        }
                        pixels[i] = rgb_data_[index];
                }

                Vector<3, float> rgb = interpolation(pixels, x);

                return rgb;
        }
};
}
