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

#include <src/com/error.h>
#include <src/image/conversion.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/numerical/interpolation.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace ns::painter::shapes::mesh
{
template <std::size_t N>
class Texture final
{
        [[nodiscard]] static std::vector<Vector<3, float>> to_rgb32(const image::Image<N>& image)
        {
                const std::size_t pixel_count = image.pixels.size() / format_pixel_size_in_bytes(image.color_format);

                std::vector<Vector<3, float>> pixels(pixel_count);

                image::format_conversion(
                        image.color_format, image.pixels, image::ColorFormat::R32G32B32,
                        std::as_writable_bytes(std::span(pixels.data(), pixels.size())));

                for (Vector<3, float>& c : pixels)
                {
                        if (!is_finite(c))
                        {
                                error("Not finite color " + to_string(c) + " in texture");
                        }

                        c[0] = std::clamp<float>(c[0], 0, 1);
                        c[1] = std::clamp<float>(c[1], 0, 1);
                        c[2] = std::clamp<float>(c[2], 0, 1);
                }

                return pixels;
        }

        std::vector<Vector<3, float>> pixels_;
        numerical::Interpolation<N, Vector<3, float>, float> interpolation_;

public:
        explicit Texture(const image::Image<N>& image)
                : pixels_(to_rgb32(image)),
                  interpolation_(image.size, pixels_)
        {
        }

        template <typename T>
        [[nodiscard]] Vector<3, float> color(const Vector<N, T>& p) const
        {
                return interpolation_.compute(p);
        }
};
}
