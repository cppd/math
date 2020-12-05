/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "process.h"

#include "../dialogs/file_dialog.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/file.h>
#include <src/image/flip.h>
#include <src/model/volume_utility.h>
#include <src/process/dimension.h>
#include <src/process/load.h>
#include <src/progress/progress.h>
#include <src/utility/file/path.h>

#include <cstring>

namespace gui::painter_window
{
namespace
{
constexpr const char* IMAGE_FILE_FORMAT = "png";

constexpr unsigned char ALPHA_FOR_FILES = 255;
constexpr unsigned char ALPHA_FOR_VOLUME = 1;

template <size_t N, typename T>
std::array<T, N> to_array(const std::vector<T>& vector)
{
        ASSERT(vector.size() == N);
        std::array<T, N> array;
        for (size_t i = 0; i < N; ++i)
        {
                array[i] = vector[i];
        }
        return array;
}

void set_alpha_bgra(std::vector<std::byte>* pixels, unsigned char alpha)
{
        ASSERT(pixels->size() % 4 == 0);
        ASSERT(alpha > 0);

        for (auto iter = pixels->begin(); iter != pixels->end(); std::advance(iter, 4))
        {
                std::memcpy(&(*(iter + 3)), &alpha, 1);
        }
}

void correct_alpha_bgra(std::vector<std::byte>* pixels, unsigned char alpha)
{
        ASSERT(pixels->size() % 4 == 0);
        ASSERT(alpha > 0);

        for (auto iter = pixels->begin(); iter != pixels->end(); std::advance(iter, 4))
        {
                unsigned char a;
                std::memcpy(&a, &(*(iter + 3)), 1);
                if (a != 0)
                {
                        std::memcpy(&(*(iter + 3)), &alpha, 1);
                }
                else
                {
                        std::array<unsigned char, 4> c{0, 0, 0, 0};
                        std::memcpy(&(*iter), c.data(), 4);
                }
        }
}

std::vector<std::byte> conv_bgra_to_rgb(const std::span<const std::byte>& pixels)
{
        if (pixels.size() % 4 != 0)
        {
                error("Error byte count (" + to_string(pixels.size()) + " for format BGRA");
        }
        std::vector<std::byte> result(3 * (pixels.size() / 4));
        auto src = pixels.begin();
        auto dst = result.begin();
        for (; src != pixels.end(); std::advance(src, 4), std::advance(dst, 3))
        {
                std::memcpy(&(*dst), &(*src), 3);
                std::swap(*dst, *(dst + 2));
        }
        return result;
}

std::vector<std::byte> conv_bgra_to_rgba(const std::span<const std::byte>& pixels)
{
        if (pixels.size() % 4 != 0)
        {
                error("Error byte count (" + to_string(pixels.size()) + " for format BGRA");
        }
        std::vector<std::byte> result(pixels.size());
        std::memcpy(result.data(), pixels.data(), data_size(result));
        for (auto dst = result.begin(); dst != result.end(); std::advance(dst, 4))
        {
                std::swap(*dst, *(dst + 2));
        }
        return result;
}

std::vector<std::byte> conv_bgra(const std::span<const std::byte>& pixels, image::ColorFormat to_format)
{
        if (to_format == image::ColorFormat::R8G8B8_SRGB)
        {
                return conv_bgra_to_rgb(pixels);
        }
        if (to_format == image::ColorFormat::R8G8B8A8_SRGB)
        {
                return conv_bgra_to_rgba(pixels);
        }
        error("Unsupported format conversion from BGRA to " + image::format_to_string(to_format));
}
}

std::function<void(ProgressRatioList*)> save_to_file(
        const std::vector<int>& screen_size,
        bool without_background,
        std::vector<std::byte>&& pixels_bgra)
{
        const std::string caption = "Save";
        dialog::FileFilter filter;
        filter.name = "Images";
        filter.file_extensions = {IMAGE_FILE_FORMAT};
        const bool read_only = true;
        std::optional<std::string> file_name_string = dialog::save_file(caption, {filter}, read_only);
        if (!file_name_string)
        {
                return nullptr;
        }

        return [pixels_bgra = std::make_shared<std::vector<std::byte>>(std::move(pixels_bgra)),
                file_name_string = std::move(file_name_string), screen_size,
                without_background](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Saving");
                progress.set(0);

                image::ColorFormat format;
                if (without_background)
                {
                        correct_alpha_bgra(pixels_bgra.get(), ALPHA_FOR_FILES);
                        format = image::ColorFormat::R8G8B8A8_SRGB;
                }
                else
                {
                        format = image::ColorFormat::R8G8B8_SRGB;
                }

                image::save(
                        path_from_utf8(*file_name_string),
                        image::ImageView<2>({screen_size[0], screen_size[1]}, format, conv_bgra(*pixels_bgra, format)));
        };
}

std::function<void(ProgressRatioList*)> save_all_to_files(
        const std::vector<int>& screen_size,
        bool without_background,
        std::vector<std::byte>&& pixels_bgra)
{
        if (screen_size.size() < 3)
        {
                return nullptr;
        }

        const std::string caption = "Save All";
        const bool read_only = false;
        std::optional<std::string> directory_string = dialog::select_directory(caption, read_only);
        if (!directory_string)
        {
                return nullptr;
        }

        return [pixels_bgra = std::make_shared<std::vector<std::byte>>(std::move(pixels_bgra)),
                directory_string = std::move(directory_string), screen_size,
                without_background](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Saving");
                progress.set(0);

                image::ColorFormat format;
                if (without_background)
                {
                        correct_alpha_bgra(pixels_bgra.get(), ALPHA_FOR_FILES);
                        format = image::ColorFormat::R8G8B8A8_SRGB;
                }
                else
                {
                        format = image::ColorFormat::R8G8B8_SRGB;
                }

                const int N = screen_size.size() + 1;
                process::apply_for_dimension(
                        N,
                        [&]<size_t N>(const process::Dimension<N>&)
                        {
                                constexpr int N_IMAGE = N - 1;
                                if constexpr (N_IMAGE >= 3)
                                {
                                        volume::save_to_images(
                                                path_from_utf8(*directory_string), IMAGE_FILE_FORMAT,
                                                image::ImageView<N - 1>(
                                                        to_array<N - 1, int>(screen_size), format,
                                                        conv_bgra(*pixels_bgra, format)),
                                                &progress);
                                }
                        });
        };
}

std::function<void(ProgressRatioList*)> add_volume(
        const std::vector<int>& screen_size,
        bool without_background,
        std::vector<std::byte>&& pixels_bgra)
{
        if (screen_size.size() < 3)
        {
                return nullptr;
        }

        return [pixels_bgra = std::make_shared<std::vector<std::byte>>(std::move(pixels_bgra)), screen_size,
                without_background](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Adding volume");
                progress.set(0);

                if (without_background)
                {
                        correct_alpha_bgra(pixels_bgra.get(), ALPHA_FOR_VOLUME);
                }
                else
                {
                        set_alpha_bgra(pixels_bgra.get(), ALPHA_FOR_VOLUME);
                }

                const int N = screen_size.size() + 1;
                process::apply_for_dimension(
                        N,
                        [&]<size_t N>(const process::Dimension<N>&)
                        {
                                constexpr int N_IMAGE = N - 1;
                                if constexpr (N_IMAGE >= 3)
                                {
                                        image::Image<N_IMAGE> image;
                                        image.size = to_array<N - 1, int>(screen_size);
                                        image.color_format = image::ColorFormat::R8G8B8A8_SRGB;
                                        image.pixels = conv_bgra(*pixels_bgra, image.color_format);

                                        image::flip_vertically(&image);

                                        process::load_volume<N_IMAGE>("Painter Image", std::move(image));
                                }
                        });
        };
}

}
