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

#include "../../mesh_utility.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/test/test.h>

#include <filesystem>
#include <random>

namespace ns::mesh
{
namespace
{
template <std::size_t N>
void test_obj_file(
        const mesh::Mesh<N>& mesh,
        const std::string& name,
        const std::filesystem::path& directory,
        const std::string& comment,
        ProgressRatio* progress)
{
        LOG("Saving to OBJ...");

        std::filesystem::path file_name = directory / path_from_utf8(name);
        file_name.replace_extension(path_from_utf8(mesh::obj_file_extension(N)));

        std::filesystem::path saved_file = mesh::save_to_obj(mesh, file_name, comment);

        LOG("Loading from OBJ...");

        std::unique_ptr<const mesh::Mesh<N>> file_mesh = mesh::load<N>(saved_file, progress);

        LOG("Comparing meshes...");

        if (mesh.vertices.size() != file_mesh->vertices.size())
        {
                error("Error writing and reading OBJ files (vertices)");
        }
        if (mesh.normals.size() != file_mesh->normals.size())
        {
                error("Error writing and reading OBJ files (normals)");
        }
        if (mesh.texcoords.size() != file_mesh->texcoords.size())
        {
                error("Error writing and reading OBJ files (texture)");
        }
        if (mesh.facets.size() != file_mesh->facets.size())
        {
                error("Error writing and reading OBJ files (facets)");
        }
        if (mesh.points.size() != file_mesh->points.size())
        {
                error("Error writing and reading OBJ files (points)");
        }
        if (mesh.lines.size() != file_mesh->lines.size())
        {
                error("Error writing and reading OBJ files (lines)");
        }
        if (mesh.materials.size() != file_mesh->materials.size())
        {
                error("Error writing and reading OBJ files (materials)");
        }
        if (mesh.images.size() != file_mesh->images.size())
        {
                error("Error writing and reading OBJ files (images)");
        }
}

template <std::size_t N>
void test_stl_file(
        const mesh::Mesh<N>& mesh,
        const std::string& name,
        const std::filesystem::path& directory,
        const std::string& comment,
        ProgressRatio* progress,
        bool ascii_format)
{
        const std::string type_name = ascii_format ? "ASCII" : "binary";
        LOG("Saving to " + type_name + " STL...");

        std::filesystem::path file_name = directory / path_from_utf8(name + "_" + type_name);
        file_name.replace_extension(path_from_utf8(mesh::stl_file_extension(N)));

        std::filesystem::path saved_file = mesh::save_to_stl(mesh, file_name, comment, ascii_format);

        LOG("Loading from " + type_name + " STL...");

        std::unique_ptr<const mesh::Mesh<N>> file_mesh = mesh::load<N>(saved_file, progress);

        LOG("Comparing meshes...");

        if (mesh.vertices.size() != file_mesh->vertices.size())
        {
                error("Error writing and reading STL files (vertices)");
        }
        if (mesh.facets.size() != file_mesh->facets.size())
        {
                error("Error writing and reading STL files (facets)");
        }
}

template <std::size_t N>
void test_geometry_files(
        const std::string& name,
        const std::vector<Vector<N, float>>& vertices,
        const std::vector<std::array<int, N>>& facets,
        ProgressRatio* progress)
{
        static_assert(N >= 3);

        LOG("Creating mesh for facets...");

        const std::unique_ptr<const mesh::Mesh<N>> mesh = mesh::create_mesh_for_facets(vertices, facets);

        const std::string comment = [&]()
        {
                std::string c;
                c += name + "\n";
                c += "vertices = " + to_string(mesh->vertices.size()) + "\n";
                c += "normals = " + to_string(mesh->normals.size()) + "\n";
                c += "facets = " + to_string(mesh->facets.size());
                return c;
        }();

        const std::filesystem::path directory = std::filesystem::temp_directory_path();

        test_obj_file(*mesh, name, directory, comment, progress);

        test_stl_file(*mesh, name, directory, comment, progress, true /*ascii_format*/);
        test_stl_file(*mesh, name, directory, comment, progress, false /*ascii_format*/);
}

template <std::size_t N>
void test_geometry_files()
{
        LOG("Test geometry files, " + space_name(N));

        const unsigned FACET_COUNT = []()
        {
                std::mt19937 engine = create_engine<std::mt19937>();
                return std::uniform_int_distribution<unsigned>(100, 1000)(engine);
        }();

        ProgressRatio progress(nullptr);

        std::vector<Vector<N, float>> vertices;
        std::vector<std::array<int, N>> facets;
        geometry::create_sphere(FACET_COUNT, &vertices, &facets);

        test_geometry_files(to_string(N - 1) + "-sphere", vertices, facets, &progress);

        LOG("Test passed");
}

void test_3()
{
        test_geometry_files<3>();
}

void test_4()
{
        test_geometry_files<4>();
}

void test_5()
{
        test_geometry_files<5>();
}

TEST_SMALL("Mesh Files 3-Space", test_3)
TEST_SMALL("Mesh Files 4-Space", test_4)
TEST_SMALL("Mesh Files 5-Space", test_5)
}
}
