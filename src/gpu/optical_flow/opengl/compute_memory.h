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

namespace gpu_opengl
{
class OpticalFlowGrayscaleMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::Buffer m_buffer;

        struct Images
        {
                GLuint64 src;
                alignas(16) GLuint64 image_dst;
        };

public:
        OpticalFlowGrayscaleMemory(const opengl::Texture& src, const opengl::Texture& image_dst);

        void bind() const;
};

class OpticalFlowDownsampleMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::Buffer m_buffer;

        struct Images
        {
                GLuint64 image_big;
                alignas(16) GLuint64 image_small;
        };

public:
        OpticalFlowDownsampleMemory(const opengl::Texture& image_big, const opengl::Texture& image_small);

        void bind() const;
};

class OpticalFlowSobelMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::Buffer m_buffer;

        struct Images
        {
                GLuint64 image_i;
                alignas(16) GLuint64 image_dx;
                alignas(16) GLuint64 image_dy;
        };

public:
        OpticalFlowSobelMemory(const opengl::Texture& image_i, const opengl::Texture& image_dx, const opengl::Texture& image_dy);

        void bind() const;
};

class OpticalFlowDataMemory final
{
        static constexpr int POINTS_BINDING = 0;
        static constexpr int POINTS_FLOW_BINDING = 1;
        static constexpr int POINTS_FLOW_GUESS_BINDING = 2;
        static constexpr int DATA_BINDING = 3;

        const opengl::Buffer* m_top_points = nullptr;
        const opengl::Buffer* m_flow = nullptr;
        const opengl::Buffer* m_flow_guess = nullptr;

        opengl::Buffer m_buffer;

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

        OpticalFlowDataMemory();

        void set_top_points(const opengl::Buffer* top_points);
        void set_flow_guess(const opengl::Buffer* flow_guess);
        void set_flow(const opengl::Buffer* flow);
        void set_data(const Data& data);

        void bind() const;
};

class OpticalFlowImagesMemory final
{
        static constexpr int IMAGES_BINDING = 4;

        opengl::Buffer m_buffer;

        struct Images
        {
                GLuint64 image_dx;
                alignas(16) GLuint64 image_dy;
                alignas(16) GLuint64 image_i;
                alignas(16) GLuint64 texture_j;
        };

public:
        OpticalFlowImagesMemory(const opengl::Texture& image_dx, const opengl::Texture& image_dy, const opengl::Texture& image_i,
                                const opengl::Texture& texture_j);

        void bind() const;
};
}
