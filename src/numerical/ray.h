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

#include "vec.h"

#include <src/com/print.h>

#include <cmath>
#include <string>

namespace ns
{
template <std::size_t N, typename T>
class Ray final
{
        Vector<N, T> org_;
        Vector<N, T> dir_;

public:
        Ray()
        {
        }

        Ray(const Vector<N, T>& org, const Vector<N, T>& dir) : org_(org), dir_(dir.normalized())
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

        Ray& move(T t)
        {
                org_ = this->point(t);
                return *this;
        }

        const Vector<N, T>& org() const
        {
                return org_;
        }

        const Vector<N, T>& dir() const
        {
                return dir_;
        }

        Vector<N, T> point(T t) const
        {
                Vector<N, T> p;
                for (std::size_t i = 0; i < N; ++i)
                {
                        p[i] = std::fma(dir_[i], t, org_[i]);
                }
                return p;
        }

        Ray<N, T> reverse_ray() const
        {
                Ray<N, T> r;
                r.org_ = org_;
                r.dir_ = -dir_;
                return r;
        }
};

template <std::size_t N, typename T>
std::string to_string(const Ray<N, T>& data)
{
        return "(org " + to_string(data.org()) + ", dir " + to_string(data.dir()) + ")";
}
}
