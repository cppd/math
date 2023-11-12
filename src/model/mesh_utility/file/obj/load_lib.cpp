/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "load_lib.h"

#include "data_read.h"
#include "name.h"

#include "../data_read.h"
#include "../lines.h"

#include <src/com/file/path.h>
#include <src/com/file/read.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/image/file_load.h>
#include <src/image/flip.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

namespace ns::model::mesh::file::obj
{
namespace
{
color::Color read_color(const char* const str)
{
        Vector<3, color::Color::DataType> rgb;
        read(str, &rgb);

        for (int i = 0; i < 3; ++i)
        {
                if (!(rgb[i] >= 0 && rgb[i] <= 1))
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
                error("Reading " + to_string(N - 1) + "-dimensional images for " + obj_name(N) + " is not supported");
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
        const std::string_view image_name,
        std::map<std::filesystem::path, int>* const image_index,
        std::vector<image::Image<N - 1>>* const images,
        int* const index)
{
        std::filesystem::path file_name = path_from_utf8(trim(image_name));

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
        Mesh<N>::Material* material_ = nullptr;
        const std::filesystem::path* lib_dir_;
        Mesh<N>* mesh_;
        std::map<std::string, int>* material_index_;
        std::map<std::filesystem::path, int>* image_index_;

public:
        ReadLib(const std::filesystem::path* const lib_dir,
                Mesh<N>* const mesh,
                std::map<std::string, int>* const material_index,
                std::map<std::filesystem::path, int>* const image_index)
                : lib_dir_(lib_dir),
                  mesh_(mesh),
                  material_index_(material_index),
                  image_index_(image_index)
        {
        }

        bool read_line(const std::string_view first, const char* const second_b, const char* const second_e)
        {
                if (first == "newmtl")
                {
                        if (material_index_->empty())
                        {
                                return false;
                        }

                        const std::string name{read_name("material", second_b, second_e)};

                        const auto iter = material_index_->find(name);
                        if (iter != material_index_->end())
                        {
                                material_ = &(mesh_->materials[iter->second]);
                                material_index_->erase(name);
                        }
                        else
                        {
                                material_ = nullptr;
                        }

                        return true;
                }

                if (!material_)
                {
                        return true;
                }

                if (first == "Kd")
                {
                        material_->color = read_color(second_b);
                }
                else if (first == "map_Kd")
                {
                        const std::string_view name{read_name("file", second_b, second_e)};
                        load_image<N>(*lib_dir_, name, image_index_, &mesh_->images, &material_->image);
                }

                return true;
        }
};
}

template <std::size_t N>
void read_lib(
        const std::filesystem::path& dir_name,
        const std::filesystem::path& file_name,
        progress::Ratio* const progress,
        std::map<std::string, int>* const material_index,
        std::map<std::filesystem::path, int>* const image_index,
        Mesh<N>* const mesh)
{
        const std::filesystem::path lib_name = dir_name / file_name;

        const Lines lines(read_file(lib_name));

        const std::filesystem::path lib_dir = lib_name.parent_path();

        const std::size_t count = lines.size();
        const double count_reciprocal = 1.0 / count;

        ReadLib<N> read_lib(&lib_dir, mesh, material_index, image_index);

        for (std::size_t i = 0; i < count; ++i)
        {
                if ((i & 0xfff) == 0xfff)
                {
                        progress->set(i * count_reciprocal);
                }

                const Split split = split_string(lines.c_str_view(i));

                try
                {
                        if (!read_lib.read_line(split.first, split.second_b, split.second_e))
                        {
                                break;
                        }
                }
                catch (const std::exception& e)
                {
                        error("Library: " + generic_utf8_filename(lib_name) + "\n" + "Line " + to_string(i) + ": "
                              + std::string(split.first) + " " + std::string(split.second_b, split.second_e) + "\n"
                              + e.what());
                }
                catch (...)
                {
                        error("Library: " + generic_utf8_filename(lib_name) + "\n" + "Line " + to_string(i) + ": "
                              + std::string(split.first) + " " + std::string(split.second_b, split.second_e) + "\n"
                              + "Unknown error");
                }
        }
}

#define TEMPLATE(N)                                                                           \
        template void read_lib(                                                               \
                const std::filesystem::path&, const std::filesystem::path&, progress::Ratio*, \
                std::map<std::string, int>*, std::map<std::filesystem::path, int>*, Mesh<(N)>*);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
