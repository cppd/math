/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "smooth.h"

#include "view/point.h"

#include <src/com/error.h>
#include <src/filter/core/smooth.h>
#include <src/filter/filters/filter.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix_object.h>
#include <src/numerical/vector_object.h>

#include <cstddef>
#include <vector>

namespace ns::filter::test
{
namespace
{
template <std::size_t N, typename T>
struct Vectors
{
        std::vector<numerical::Matrix<N, N, T>> predict_f;
        std::vector<numerical::Vector<N, T>> predict_x;
        std::vector<numerical::Matrix<N, N, T>> predict_p;
        std::vector<numerical::Vector<N, T>> x;
        std::vector<numerical::Matrix<N, N, T>> p;
        std::vector<T> time;
};

template <std::size_t N, typename T>
void init(const filters::UpdateDetails<N, T>& details, Vectors<N, T>& v)
{
        ASSERT(!details.predict_f);
        ASSERT(!details.predict_x);
        ASSERT(!details.predict_p);

        v.predict_f.clear();
        v.predict_x.clear();
        v.predict_p.clear();
        v.x.clear();
        v.p.clear();
        v.time.clear();

        v.predict_f.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        v.predict_x.push_back(numerical::Vector<N, T>(0));
        v.predict_p.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        v.x.push_back(details.update_x);
        v.p.push_back(details.update_p);
        v.time.push_back(details.time);
}

template <std::size_t N, typename T>
void add(const filters::UpdateDetails<N, T>& details, Vectors<N, T>& v)
{
        ASSERT(details.predict_f);
        ASSERT(details.predict_x);
        ASSERT(details.predict_p);

        v.predict_f.push_back(*details.predict_f);
        v.predict_x.push_back(*details.predict_x);
        v.predict_p.push_back(*details.predict_p);
        v.x.push_back(details.update_x);
        v.p.push_back(details.update_p);
        v.time.push_back(details.time);
}

template <std::size_t N, typename T, std::size_t ORDER>
view::Point<2, T> make_point(
        const T time,
        const numerical::Vector<N, T>& x,
        const numerical::Matrix<N, N, T>& p,
        const filters::FilterPosition<2, T, ORDER>& filter)
{
        T speed = 0;
        T speed_p = 0;

        if constexpr (ORDER > 0)
        {
                speed = filter.x_to_speed(x);
                speed_p = filter.xp_to_speed_p(x, p);
        }

        return {
                .time = time,
                .position = filter.x_to_position(x),
                .position_p = filter.p_to_position_p(p),
                .speed = speed,
                .speed_p = speed_p,
        };
}

template <std::size_t N, typename T, std::size_t ORDER>
void write_smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const Vectors<N, T>& v,
        std::vector<view::Point<2, T>>& points)
{
        const auto [x, p] = core::smooth(v.predict_f, v.predict_x, v.predict_p, v.x, v.p);

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == v.time.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                points.push_back(make_point(v.time[i], x[i], p[i], filter));
        }
}
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<view::Point<2, T>> smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const std::vector<filters::UpdateDetails<N, T>>& details)
{
        static_assert(N >= 2);
        static_assert((N % 2) == 0);

        if (details.empty())
        {
                return {};
        }

        std::vector<view::Point<2, T>> res;

        Vectors<N, T> vectors;

        init(details[0], vectors);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& p_f = details[i].predict_f;
                const auto& p_x = details[i].predict_x;
                const auto& p_p = details[i].predict_p;

                if (!(p_f && p_x && p_p))
                {
                        write_smooth(filter, vectors, res);
                        init(details[i], vectors);
                        continue;
                }

                add(details[i], vectors);
        }

        write_smooth(filter, vectors, res);

        return res;
}

#define TEMPLATE(T)                                                                                         \
        template std::vector<view::Point<2, T>> smooth(                                                     \
                const filters::FilterPosition<2, T, 0>&, const std::vector<filters::UpdateDetails<2, T>>&); \
        template std::vector<view::Point<2, T>> smooth(                                                     \
                const filters::FilterPosition<2, T, 1>&, const std::vector<filters::UpdateDetails<4, T>>&); \
        template std::vector<view::Point<2, T>> smooth(                                                     \
                const filters::FilterPosition<2, T, 2>&, const std::vector<filters::UpdateDetails<6, T>>&);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)

}
