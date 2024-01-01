/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "vector.h"

#include <cstddef>
#include <string>

namespace ns
{
template <std::size_t N, typename T>
class Ray final
{
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> org_;
        Vector<N, T> dir_;

public:
        Ray()
        {
        }

        Ray(const Vector<N, T>& org, const Vector<N, T>& dir)
                : org_(org),
                  dir_(dir.normalized())
        {
        }

        Ray& set_org(const Vector<N, T>& org)
        {
                org_ = org;
                return *this;
        }

        Ray& set_dir(const Vector<N, T>& dir)
        {
                dir_ = dir.normalized();
                return *this;
        }

        Ray& move(const T& t)
        {
                org_ = this->point(t);
                return *this;
        }

        [[nodiscard]] const Vector<N, T>& org() const
        {
                return org_;
        }

        [[nodiscard]] const Vector<N, T>& dir() const
        {
                return dir_;
        }

        [[nodiscard]] Vector<N, T> point(const T& t) const
        {
                Vector<N, T> p;
                for (std::size_t i = 0; i < N; ++i)
                {
                        p[i] = dir_[i] * t + org_[i];
                }
                return p;
        }

        [[nodiscard]] Ray<N, T> reversed() const
        {
                Ray<N, T> res;
                res.org_ = org_;
                res.dir_ = -dir_;
                return res;
        }

        [[nodiscard]] Ray<N, T> moved(const T& t) const
        {
                Ray<N, T> res;
                res.org_ = this->point(t);
                res.dir_ = dir_;
                return res;
        }
};

template <std::size_t N, typename T>
[[nodiscard]] std::string to_string(const Ray<N, T>& ray)
{
        return "(org " + to_string(ray.org()) + ", dir " + to_string(ray.dir()) + ")";
}
}
