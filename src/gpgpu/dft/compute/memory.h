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

#include "com/error.h"
#include "graphics/opengl/buffers.h"

#include <vector>

template <typename T>
class DeviceMemory final
{
        size_t m_size;
        opengl::StorageBuffer m_buffer;

public:
        DeviceMemory(int size) : m_size(size), m_buffer(size * sizeof(T))
        {
        }

        void write(const std::vector<T>& data) const
        {
                if (data.size() != m_size)
                {
                        error("Storage size error");
                }
                m_buffer.write(data);
        }

        void read(std::vector<T>* data) const
        {
                if (data->size() != m_size)
                {
                        error("Storage size error");
                }
                m_buffer.read(data);
        }

        std::vector<T> read() const
        {
                std::vector<T> v(size());
                read(&v);
                return v;
        }

        void bind(int point) const
        {
                m_buffer.bind(point);
        }

        int size() const noexcept
        {
                return m_size;
        }
};
