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

#include <src/color/color.h>
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
        std::vector<Vector<3, float>> m_rgb_data;
        std::array<int, N> m_size;
        std::array<int, N> m_max;
        GlobalIndex<N, long long> m_global_index;

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

                m_size = size;

                for (unsigned i = 0; i < N; ++i)
                {
                        m_max[i] = m_size[i] - 1;
                }

                m_global_index = decltype(m_global_index)(m_size);

                m_rgb_data.clear();
                m_rgb_data.shrink_to_fit();
                m_rgb_data.resize(m_global_index.count());
        }

public:
        explicit MeshTexture(const image::Image<N>& image)
        {
                resize(image.size);

                image::format_conversion(
                        image.color_format, image.pixels, image::ColorFormat::R32G32B32,
                        std::as_writable_bytes(std::span(m_rgb_data.data(), m_rgb_data.size())));

                for (Vector<3, float>& c : m_rgb_data)
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
        Color color(const Vector<N, T>& p) const
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

                std::array<Vector<3, float>, (1 << N)> pixels;

                for (unsigned i = 0; i < pixels.size(); ++i)
                {
                        long long index = 0;
                        for (unsigned n = 0; n < N; ++n)
                        {
                                int coordinate = ((1 << n) & i) ? x1[n] : x0[n];
                                index += m_global_index.stride(n) * coordinate;
                        }
                        pixels[i] = m_rgb_data[index];
                }

                Vector<3, float> rgb = interpolation(pixels, x);

                return Color(rgb[0], rgb[1], rgb[2]);
        }
};
}
