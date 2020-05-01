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

#pragma once

#include "object_id.h"
#include "volume.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>

#include <memory>
#include <string>

namespace volume
{
template <size_t N>
class VolumeObject final
{
        std::unique_ptr<const Volume<N>> m_volume;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;
        float m_level_min = 0;
        float m_level_max = 1;

public:
        VolumeObject(
                std::unique_ptr<const Volume<N>>&& volume,
                const Matrix<N + 1, N + 1, double>& matrix,
                const std::string& name)
                : m_volume(std::move(volume)), m_matrix(matrix), m_name(name)
        {
                ASSERT(m_volume);
        }

        const Volume<N>& volume() const
        {
                return *m_volume;
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_matrix;
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                m_matrix = matrix;
        }

        const std::string& name() const
        {
                return m_name;
        }

        const ObjectId& id() const
        {
                return m_id;
        }

        float level_min() const
        {
                return m_level_min;
        }

        float level_max() const
        {
                return m_level_max;
        }

        void set_levels(float min, float max)
        {
                m_level_min = min;
                m_level_max = max;
        }
};
}
