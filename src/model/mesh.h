/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/image/image.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ns::model::mesh
{
template <std::size_t N>
struct Mesh final
{
        struct Facet final
        {
                std::array<int, N> vertices;
                std::array<int, N> normals; // index or -1
                std::array<int, N> texcoords; // index or -1
                int material; // index or -1
                bool has_texcoord;
                bool has_normal;
        };

        struct Point final
        {
                int vertex;
        };

        struct Line final
        {
                std::array<int, 2> vertices;
        };

        struct Material final
        {
                std::string name;
                color::Color color{0};
                int image{-1}; // index or -1
        };

        std::vector<numerical::Vector<N, float>> vertices;
        std::vector<numerical::Vector<N, float>> normals;
        std::vector<numerical::Vector<N - 1, float>> texcoords;
        std::vector<Facet> facets;
        std::vector<Point> points;
        std::vector<Line> lines;
        std::vector<Material> materials;
        std::vector<image::Image<N - 1>> images;
        numerical::Vector<N, float> center{0};
        float length{0};

        Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;

        ~Mesh() = default;
};
}
