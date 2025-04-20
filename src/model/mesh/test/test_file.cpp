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

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/model/mesh.h>
#include <src/model/mesh/file/load_stl.h>
#include <src/model/mesh/file/save_stl.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/dimensions.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace ns::model::mesh
{
namespace
{
template <std::size_t N>
void compare_obj(const mesh::Mesh<N>& mesh, const std::unique_ptr<const mesh::Mesh<N>>& file_mesh)
{
        if (!file_mesh)
        {
                error("Error writing and reading OBJ files (no mesh)");
        }

        if (!(mesh.vertices.size() == file_mesh->vertices.size()))
        {
                error("Error writing and reading OBJ files (vertices)");
        }

        if (!(mesh.normals.size() == file_mesh->normals.size()))
        {
                error("Error writing and reading OBJ files (normals)");
        }

        if (!(mesh.texcoords.size() == file_mesh->texcoords.size()))
        {
                error("Error writing and reading OBJ files (texture)");
        }

        if (!(mesh.facets.size() == file_mesh->facets.size()))
        {
                error("Error writing and reading OBJ files (facets)");
        }

        if (!(mesh.points.size() == file_mesh->points.size()))
        {
                error("Error writing and reading OBJ files (points)");
        }

        if (!(mesh.lines.size() == file_mesh->lines.size()))
        {
                error("Error writing and reading OBJ files (lines)");
        }

        if (!(mesh.materials.size() == file_mesh->materials.size()))
        {
                error("Error writing and reading OBJ files (materials)");
        }

        if (!(mesh.images.size() == file_mesh->images.size()))
        {
                error("Error writing and reading OBJ files (images)");
        }
}

template <std::size_t N>
void compare_stl(const mesh::Mesh<N>& mesh, const std::unique_ptr<const mesh::Mesh<N>>& file_mesh)
{
        if (!file_mesh)
        {
                error("Error writing and reading STL files (no mesh)");
        }

        if (!(mesh.vertices.size() == file_mesh->vertices.size()))
        {
                error("Error writing and reading STL files (vertices)");
        }

        if (!(mesh.facets.size() == file_mesh->facets.size()))
        {
                error("Error writing and reading STL files (facets)");
        }
}

template <std::size_t N>
void test_obj_file(
        const mesh::Mesh<N>& mesh,
        const std::string& name,
        const std::string& comment,
        progress::Ratio* const progress)
{
        const std::filesystem::path file_name = [&]
        {
                std::filesystem::path path = settings::test_path(name);
                path.replace_extension(path_from_utf8(mesh::obj_file_extension(N)));
                return path;
        }();

        LOG("Saving to OBJ...");
        const std::filesystem::path saved_file = mesh::save_to_obj(mesh, file_name, comment);

        LOG("Loading from OBJ...");
        const std::unique_ptr<const mesh::Mesh<N>> file_mesh = mesh::load<N>(saved_file, progress);

        LOG("Comparing meshes...");
        compare_obj(mesh, file_mesh);
}

template <std::size_t N>
void test_stl_file(
        const mesh::Mesh<N>& mesh,
        const std::string& name,
        const std::string& comment,
        progress::Ratio* const progress,
        const bool ascii_format)
{
        const std::string type_name = ascii_format ? "ASCII" : "binary";

        const std::filesystem::path file_name = [&]
        {
                std::filesystem::path path = settings::test_path(name + "_" + type_name);
                path.replace_extension(path_from_utf8(mesh::stl_file_extension(N)));
                return path;
        }();

        {
                LOG("Saving to " + type_name + " STL...");
                const std::filesystem::path saved_file = mesh::save_to_stl(mesh, file_name, comment, ascii_format);

                LOG("Loading from " + type_name + " STL...");
                const std::unique_ptr<const mesh::Mesh<N>> file_mesh = mesh::load<N>(saved_file, progress);

                LOG("Comparing meshes...");
                compare_stl(mesh, file_mesh);
        }

        if (!ascii_format)
        {
                const auto test = [&](const bool byte_swap)
                {
                        LOG("Saving to " + type_name + " STL (" + (byte_swap ? "" : "no ") + "byte swap)...");
                        const std::filesystem::path saved_file =
                                file::save_to_stl_file(mesh, file_name, comment, ascii_format, byte_swap);

                        LOG("Loading from " + type_name + " STL (" + (byte_swap ? "" : "no ") + "byte swap)...");
                        const std::unique_ptr<const mesh::Mesh<N>> file_mesh =
                                file::load_from_stl_file<N>(saved_file, progress, byte_swap);

                        LOG("Comparing meshes...");
                        compare_stl(mesh, file_mesh);

                        try
                        {
                                file::load_from_stl_file<N>(saved_file, progress, !byte_swap);
                        }
                        catch (...)
                        {
                                return;
                        }
                        error("Error writing and reading STL files (byte swap error)");
                };

                test(false);
                test(true);
        }
}

template <std::size_t N>
void test_mesh_files(
        const std::string& name,
        const std::vector<numerical::Vector<N, float>>& vertices,
        const std::vector<std::array<int, N>>& facets,
        progress::Ratio* const progress)
{
        static_assert(N >= 3);

        LOG("Creating mesh for facets...");

        constexpr bool WRITE_LOG = false;
        const std::unique_ptr<const mesh::Mesh<N>> mesh = mesh::create_mesh_for_facets(vertices, facets, WRITE_LOG);

        const std::string comment = [&]
        {
                std::string c;
                c += name + "\n";
                c += "vertices = " + to_string(mesh->vertices.size()) + "\n";
                c += "normals = " + to_string(mesh->normals.size()) + "\n";
                c += "facets = " + to_string(mesh->facets.size());
                return c;
        }();

        test_obj_file(*mesh, name, comment, progress);

        test_stl_file(*mesh, name, comment, progress, /*ascii_format=*/true);
        test_stl_file(*mesh, name, comment, progress, /*ascii_format=*/false);
}

template <std::size_t N>
void test()
{
        LOG("Test mesh files, " + space_name(N));

        const unsigned facet_count = []
        {
                PCG engine;
                return std::uniform_int_distribution<unsigned>(100, 1000)(engine);
        }();

        progress::Ratio progress(nullptr);

        std::vector<numerical::Vector<N, float>> vertices;
        std::vector<std::array<int, N>> facets;
        geometry::shapes::create_sphere(facet_count, &vertices, &facets);

        test_mesh_files(to_string(N - 1) + "-sphere", vertices, facets, &progress);

        LOG("Test mesh files passed");
}

template <std::size_t... I>
auto mesh_file_tests(std::index_sequence<I...>&&) noexcept
{
        return std::to_array({std::make_tuple(
                ns::test::Type::SMALL, "Mesh Files, " + to_upper_first_letters(space_name(I)), test<I>)...});
}

TESTS(mesh_file_tests(settings::Dimensions()))
}
}
