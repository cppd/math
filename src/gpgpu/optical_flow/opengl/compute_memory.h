/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "graphics/opengl/buffers.h"

namespace gpgpu_optical_flow_compute_opengl_implementation
{
class GrayscaleMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_src;
                alignas(16) GLuint64 image_dst;
        };

public:
        GrayscaleMemory(const opengl::TextureRGBA32F& image_src, const opengl::TextureR32F& image_dst);

        void bind() const;
};

class DownsampleMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_big;
                alignas(16) GLuint64 image_small;
        };

public:
        DownsampleMemory(const opengl::TextureR32F& image_big, const opengl::TextureR32F& image_small);

        void bind() const;
};

class SobelMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_i;
                alignas(16) GLuint64 image_dx;
                alignas(16) GLuint64 image_dy;
        };

public:
        SobelMemory(const opengl::TextureR32F& image_i, const opengl::TextureR32F& image_dx, const opengl::TextureR32F& image_dy);

        void bind() const;
};

class FlowDataMemory final
{
        static constexpr int POINTS_BINDING = 0;
        static constexpr int POINTS_FLOW_BINDING = 1;
        static constexpr int POINTS_FLOW_GUESS_BINDING = 2;
        static constexpr int DATA_BINDING = 3;

        const opengl::StorageBuffer* m_top_points = nullptr;
        const opengl::StorageBuffer* m_flow = nullptr;
        const opengl::StorageBuffer* m_flow_guess = nullptr;

        opengl::UniformBuffer m_buffer;

public:
        struct Data
        {
                GLint point_count_x;
                GLint point_count_y;
                GLuint use_all_points;
                GLuint use_guess;
                GLint guess_kx;
                GLint guess_ky;
                GLint guess_width;
        };

        FlowDataMemory();

        void set_top_points(const opengl::StorageBuffer* top_points);
        void set_flow_guess(const opengl::StorageBuffer* flow_guess);
        void set_flow(const opengl::StorageBuffer* flow);
        void set_data(const Data& data);

        void bind() const;
};

class FlowImagesMemory final
{
        static constexpr int IMAGES_BINDING = 4;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_dx;
                alignas(16) GLuint64 image_dy;
                alignas(16) GLuint64 image_i;
                alignas(16) GLuint64 texture_j;
        };

public:
        FlowImagesMemory(const opengl::TextureR32F& image_dx, const opengl::TextureR32F& image_dy,
                         const opengl::TextureR32F& image_i, const opengl::TextureR32F& texture_j);

        void bind() const;
};
}
