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

#include "load_mtl.h"

#include "data_read.h"
#include "file_lines.h"

#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/image/file.h>
#include <src/image/flip.h>

namespace ns::mesh::file
{
namespace
{
std::string obj_type_name(const std::size_t n)
{
        return "OBJ-" + to_string(n);
}

color::Color read_color(const char* const str)
{
        static constexpr float MIN = 0;
        static constexpr float MAX = 1;

        std::array<float, 3> rgb;
        read_float(str, &rgb[0], &rgb[1], &rgb[2]);

        for (int i = 0; i < 3; ++i)
        {
                if (!(check_range(rgb[i], MIN, MAX)))
                {
                        error("RGB components (" + to_string(rgb) + ") are not in the range [0, 1]");
                }
        }

        return {rgb[0], rgb[1], rgb[2]};
}

template <std::size_t N>
image::Image<N> read_image_from_file(const std::filesystem::path& file_name)
{
        if constexpr (N != 2)
        {
                error("Reading " + to_string(N - 1) + "-dimensional images for " + obj_type_name(N)
                      + " is not supported");
        }
        else
        {
                image::Image<2> obj_image = image::load_rgba(file_name);
                image::flip_vertically(&obj_image);
                return obj_image;
        }
}

template <std::size_t N>
void load_image(
        const std::filesystem::path& dir_name,
        const std::filesystem::path& image_name,
        std::map<std::string, int>* const image_index,
        std::vector<image::Image<N - 1>>* const images,
        int* const index)
{
        std::filesystem::path file_name = path_from_utf8(trim(generic_utf8_filename(image_name)));

        if (file_name.empty())
        {
                error("No image file name");
        }

        file_name = dir_name / file_name;

        if (auto iter = image_index->find(file_name); iter != image_index->end())
        {
                *index = iter->second;
                return;
        }

        images->push_back(read_image_from_file<N - 1>(file_name));
        *index = images->size() - 1;
        image_index->emplace(file_name, *index);
}

template <std::size_t N>
class ReadLib final
{
        const std::filesystem::path* lib_dir_;
        const std::vector<char>* data_;
        Mesh<N>* mesh_;
        std::map<std::string, int>* material_index_;
        std::map<std::string, int>* image_index_;

public:
        ReadLib(const std::filesystem::path* const lib_dir,
                const std::vector<char>* const data,
                Mesh<N>* const mesh,
                std::map<std::string, int>* const material_index,
                std::map<std::string, int>* const image_index)
                : lib_dir_(lib_dir),
                  data_(data),
                  mesh_(mesh),
                  material_index_(material_index),
                  image_index_(image_index)
        {
        }

        bool read_line(const char* first, long long second_b, long long second_e, typename Mesh<N>::Material** material)
                const;
};

template <std::size_t N>
bool ReadLib<N>::read_line(
        const char* const first,
        const long long second_b,
        const long long second_e,
        typename Mesh<N>::Material** const material) const
{
        auto& mtl = *material;

        if (!*first)
        {
                return true;
        }

        if (str_equal(first, "newmtl"))
        {
                if (material_index_->empty())
                {
                        return false;
                }

                std::string name;
                read_name("material", *data_, second_b, second_e, &name);

                auto iter = material_index_->find(name);
                if (iter != material_index_->end())
                {
                        mtl = &(mesh_->materials[iter->second]);
                        material_index_->erase(name);
                }
                else
                {
                        mtl = nullptr;
                }
        }
        else if (str_equal(first, "Kd"))
        {
                if (!mtl)
                {
                        return true;
                }
                try
                {
                        mtl->color = read_color(&(*data_)[second_b]);
                }
                catch (const std::exception& e)
                {
                        error("Reading Kd in material " + mtl->name + "\n" + e.what());
                }
        }
        else if (str_equal(first, "map_Kd"))
        {
                if (!mtl)
                {
                        return true;
                }

                std::string name;
                read_name("file", *data_, second_b, second_e, &name);
                load_image<N>(*lib_dir_, name, image_index_, &mesh_->images, &mtl->image);
        }

        return true;
}
}

template <std::size_t N>
void read_lib(
        const std::filesystem::path& dir_name,
        const std::filesystem::path& file_name,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        std::map<std::string, int>* const image_index,
        Mesh<N>* const mesh)
{
        std::vector<char> data;
        std::vector<long long> line_begin;

        const std::filesystem::path lib_name = dir_name / file_name;

        read_file_lines(lib_name, &data, &line_begin);

        const std::filesystem::path lib_dir = lib_name.parent_path();

        const long long line_count = line_begin.size();
        const double line_count_reciprocal = 1.0 / line_begin.size();

        ReadLib<N> read_lib(&lib_dir, &data, mesh, material_index, image_index);

        typename Mesh<N>::Material* mtl = nullptr;

        for (long long line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                const char* first;
                const char* second;
                long long second_b;
                long long second_e;

                split_line(&data, line_begin, line_num, &first, &second, &second_b, &second_e);

                try
                {
                        if (!read_lib.read_line(first, second_b, second_e, &mtl))
                        {
                                break;
                        }
                }
                catch (const std::exception& e)
                {
                        error("Library: " + generic_utf8_filename(lib_name) + "\n" + "Line " + to_string(line_num)
                              + ": " + first + " " + second + "\n" + e.what());
                }
                catch (...)
                {
                        error("Library: " + generic_utf8_filename(lib_name) + "\n" + "Line " + to_string(line_num)
                              + ": " + first + " " + second + "\n" + "Unknown error");
                }
        }
}

#define READ_LIB_INSTANTIATION(N)                                                           \
        template void read_lib(                                                             \
                const std::filesystem::path&, const std::filesystem::path&, ProgressRatio*, \
                std::map<std::string, int>*, std::map<std::string, int>*, Mesh<(N)>*);

READ_LIB_INSTANTIATION(3)
READ_LIB_INSTANTIATION(4)
READ_LIB_INSTANTIATION(5)
READ_LIB_INSTANTIATION(6)
}
