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

#pragma once

#include "surface_facet.h"

#include <src/com/error.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/geometry/shapes/sphere_simplex.h>

#include <cmath>
#include <sstream>

namespace ns::sampling::test
{
template <std::size_t N, typename T>
class Bucket final : public SurfaceFacet<N, T>
{
        long long m_sample_count;
        long long m_uniform_count;
        long long m_pdf_count;
        double m_pdf_sum;

public:
        Bucket(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& vertex_indices)
                : SurfaceFacet<N, T>(vertices, vertex_indices)
        {
                clear();
        }

        void clear()
        {
                m_sample_count = 0;
                m_uniform_count = 0;
                m_pdf_count = 0;
                m_pdf_sum = 0;
        }

        void add_sample()
        {
                ++m_sample_count;
        }

        long long sample_count() const
        {
                return m_sample_count;
        }

        void add_uniform()
        {
                ++m_uniform_count;
        }

        long long uniform_count() const
        {
                return m_uniform_count;
        }

        double area(long long all_uniform_count) const
        {
                static constexpr double SPHERE_AREA = geometry::sphere_area(N);
                double bucket_area = double(m_uniform_count) / all_uniform_count * SPHERE_AREA;
                if constexpr (N == 3)
                {
                        double geometry_bucket_area = geometry::sphere_simplex_area(this->vertices());
                        double relative_error = std::abs(bucket_area - geometry_bucket_area)
                                                / std::max(geometry_bucket_area, bucket_area);
                        if (!(relative_error < 0.025))
                        {
                                std::ostringstream oss;
                                oss << "bucket area relative error = " << relative_error << '\n';
                                oss << "bucket area = " << bucket_area << '\n';
                                oss << "geometry bucket area = " << geometry_bucket_area << '\n';
                                oss << "uniform count = " << m_uniform_count << '\n';
                                oss << "all uniform count = " << all_uniform_count;
                                error(oss.str());
                        }
                        bucket_area = geometry_bucket_area;
                }
                return bucket_area;
        }

        void add_pdf(double pdf)
        {
                m_pdf_count += 1;
                m_pdf_sum += pdf;
        }

        double pdf() const
        {
                if (!(m_pdf_count > 0))
                {
                        error("Bucket PDF not computed");
                }
                return m_pdf_sum / m_pdf_count;
        }

        void merge(const Bucket& bucket)
        {
                m_sample_count += bucket.m_sample_count;
                m_uniform_count += bucket.m_uniform_count;
                m_pdf_count += bucket.m_pdf_count;
                m_pdf_sum += bucket.m_pdf_sum;
        }
};
}
