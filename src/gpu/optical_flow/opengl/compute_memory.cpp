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

#include "compute_memory.h"

namespace gpu_opengl
{
OpticalFlowGrayscaleMemory::OpticalFlowGrayscaleMemory(const opengl::Texture& image_src, const opengl::Texture& image_dst)
        : m_buffer(sizeof(Images))
{
        ASSERT(image_src.format() == GL_RGBA32F);
        ASSERT(image_dst.format() == GL_R32F);

        Images images;

        images.image_src = image_src.image_handle_read_only();
        images.image_dst = image_dst.image_handle_write_only();

        m_buffer.copy(images);
}

void OpticalFlowGrayscaleMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}

//

OpticalFlowDownsampleMemory::OpticalFlowDownsampleMemory(const opengl::Texture& image_big, const opengl::Texture& image_small)
        : m_buffer(sizeof(Images))
{
        ASSERT(image_big.format() == GL_R32F);
        ASSERT(image_small.format() == GL_R32F);

        Images images;

        images.image_big = image_big.image_handle_read_only();
        images.image_small = image_small.image_handle_write_only();

        m_buffer.copy(images);
}

void OpticalFlowDownsampleMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}

//

OpticalFlowSobelMemory::OpticalFlowSobelMemory(const opengl::Texture& image_i, const opengl::Texture& image_dx,
                                               const opengl::Texture& image_dy)
        : m_buffer(sizeof(Images))
{
        ASSERT(image_i.format() == GL_R32F);
        ASSERT(image_dx.format() == GL_R32F);
        ASSERT(image_dy.format() == GL_R32F);

        Images images;

        images.image_i = image_i.image_handle_read_only();
        images.image_dx = image_dx.image_handle_write_only();
        images.image_dy = image_dy.image_handle_write_only();

        m_buffer.copy(images);
}

void OpticalFlowSobelMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}

//

OpticalFlowDataMemory::OpticalFlowDataMemory() : m_buffer(sizeof(Data))
{
}

void OpticalFlowDataMemory::set_top_points(const opengl::StorageBuffer* top_points)
{
        m_top_points = top_points;
}

void OpticalFlowDataMemory::set_flow_guess(const opengl::StorageBuffer* flow_guess)
{
        m_flow_guess = flow_guess;
}

void OpticalFlowDataMemory::set_flow(const opengl::StorageBuffer* flow)
{
        m_flow = flow;
}

void OpticalFlowDataMemory::set_data(const Data& data)
{
        m_buffer.copy(data);
}

void OpticalFlowDataMemory::bind() const
{
        ASSERT(m_flow);

        if (m_top_points)
        {
                m_top_points->bind(POINTS_BINDING);
        }

        m_flow->bind(POINTS_FLOW_BINDING);

        if (m_flow_guess)
        {
                m_flow_guess->bind(POINTS_FLOW_GUESS_BINDING);
        }

        m_buffer.bind(DATA_BINDING);
}

//

OpticalFlowImagesMemory::OpticalFlowImagesMemory(const opengl::Texture& image_dx, const opengl::Texture& image_dy,
                                                 const opengl::Texture& image_i, const opengl::Texture& texture_j)
        : m_buffer(sizeof(Images))
{
        ASSERT(image_dx.format() == GL_R32F);
        ASSERT(image_dy.format() == GL_R32F);
        ASSERT(image_i.format() == GL_R32F);
        ASSERT(texture_j.format() == GL_R32F);

        Images images;

        images.image_dx = image_dx.image_handle_read_only();
        images.image_dy = image_dy.image_handle_read_only();
        images.image_i = image_i.image_handle_read_only();
        images.texture_j = texture_j.texture_handle();

        m_buffer.copy(images);
}

void OpticalFlowImagesMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}
}
