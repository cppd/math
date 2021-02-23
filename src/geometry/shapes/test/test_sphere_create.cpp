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

#include "../sphere_create.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/numerical/orthogonal.h>
#include <src/test/test.h>

#include <sstream>
#include <unordered_map>

namespace ns::geometry
{
namespace
{
template <std::size_t N, typename T>
struct Hash
{
        std::size_t operator()(const std::array<T, N>& v) const
        {
                return array_hash(v);
        }
};

template <typename T>
void test_creating_sphere()
{
        constexpr unsigned FACE_COUNT = 1000;

        std::vector<Vector<3, T>> vertices;
        std::vector<std::array<int, 3>> faces;

        create_sphere(FACE_COUNT, &vertices, &faces);

        if (faces.size() < FACE_COUNT)
        {
                error("Face count " + to_string(faces.size()) + " is less than required minimum "
                      + to_string(FACE_COUNT));
        }

        std::unordered_map<std::array<int, 2>, int, Hash<2, int>> edges;

        for (const std::array<int, 3>& face : faces)
        {
                Vector<3, T> n = numerical::ortho_nn(vertices, face).normalized();
                if (!is_finite(n))
                {
                        error("Face normal " + to_string(n) + " is not finite");
                }
                for (unsigned r = 0; r < 3; ++r)
                {
                        std::array<int, 2> edge = sort(del_elem(face, r));
                        ++edges[edge];
                }
        }

        for (const auto& [edge, count] : edges)
        {
                if (count != 2)
                {
                        error("Edge face count " + to_string(count) + " is not equal to 2");
                }
        }

        int euler_characteristic = std::ssize(vertices) - std::ssize(edges) + std::ssize(faces);
        if (euler_characteristic != 2)
        {
                std::ostringstream oss;
                oss << "Euler characteristic " << euler_characteristic << " is not equal to 2 for 2-sphere" << '\n';
                oss << "Vertex count = " << vertices.size();
                oss << "Edge count = " << edges.size();
                oss << "Face count = " << faces.size();
                error(oss.str());
        }
}

void test()
{
        LOG("Test Sphere Creation");

        test_creating_sphere<float>();
        test_creating_sphere<double>();
}

TEST_SMALL("Sphere Creation", test)
}
}
