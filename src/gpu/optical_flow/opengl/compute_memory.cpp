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

#include "graphics/opengl/functions.h"

namespace gpu_opengl
{
OpticalFlowGrayscaleMemory::OpticalFlowGrayscaleMemory(const opengl::Texture& src, const opengl::Texture& image_dst)
        : m_buffer(sizeof(Images), GL_MAP_WRITE_BIT)
{
        ASSERT(image_dst.format() == GL_R32F);

        Images images;

        images.src = src.texture_handle();
        images.image_dst = image_dst.image_handle_write_only();

        opengl::map_and_write_to_buffer(m_buffer, images);
}

void OpticalFlowGrayscaleMemory::bind() const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, IMAGES_BINDING, m_buffer);
}

//

OpticalFlowDownsampleMemory::OpticalFlowDownsampleMemory(const opengl::Texture& image_big, const opengl::Texture& image_small)
        : m_buffer(sizeof(Images), GL_MAP_WRITE_BIT)
{
        ASSERT(image_big.format() == GL_R32F);
        ASSERT(image_small.format() == GL_R32F);

        Images images;

        images.image_big = image_big.image_handle_read_only();
        images.image_small = image_small.image_handle_write_only();

        opengl::map_and_write_to_buffer(m_buffer, images);
}

void OpticalFlowDownsampleMemory::bind() const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, IMAGES_BINDING, m_buffer);
}

//

OpticalFlowSobelMemory::OpticalFlowSobelMemory(const opengl::Texture& image_i, const opengl::Texture& image_dx,
                                               const opengl::Texture& image_dy)
        : m_buffer(sizeof(Images), GL_MAP_WRITE_BIT)
{
        ASSERT(image_i.format() == GL_R32F);
        ASSERT(image_dx.format() == GL_R32F);
        ASSERT(image_dy.format() == GL_R32F);

        Images images;

        images.image_i = image_i.image_handle_read_only();
        images.image_dx = image_dx.image_handle_write_only();
        images.image_dy = image_dy.image_handle_write_only();

        opengl::map_and_write_to_buffer(m_buffer, images);
}

void OpticalFlowSobelMemory::bind() const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, IMAGES_BINDING, m_buffer);
}

//

OpticalFlowDataMemory::OpticalFlowDataMemory() : m_buffer(sizeof(Data), GL_MAP_WRITE_BIT)
{
}

void OpticalFlowDataMemory::set_top_points(const opengl::Buffer* top_points)
{
        m_top_points = top_points;
}

void OpticalFlowDataMemory::set_flow_guess(const opengl::Buffer* flow_guess)
{
        m_flow_guess = flow_guess;
}

void OpticalFlowDataMemory::set_flow(const opengl::Buffer* flow)
{
        m_flow = flow;
}

void OpticalFlowDataMemory::set_data(const Data& data)
{
        opengl::map_and_write_to_buffer(m_buffer, data);
}

void OpticalFlowDataMemory::bind() const
{
        ASSERT(m_flow);

        if (m_top_points)
        {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_BINDING, *m_top_points);
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_FLOW_BINDING, *m_flow);

        if (m_flow_guess)
        {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_FLOW_GUESS_BINDING, *m_flow_guess);
        }

        glBindBufferBase(GL_UNIFORM_BUFFER, DATA_BINDING, m_buffer);
}

//

OpticalFlowImagesMemory::OpticalFlowImagesMemory(const opengl::Texture& image_dx, const opengl::Texture& image_dy,
                                                 const opengl::Texture& image_i, const opengl::Texture& texture_j)
        : m_buffer(sizeof(Images), GL_MAP_WRITE_BIT)
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

        opengl::map_and_write_to_buffer(m_buffer, images);
}

void OpticalFlowImagesMemory::bind() const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, IMAGES_BINDING, m_buffer);
}
}
