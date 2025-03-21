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

#include "view/write.h"

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
        std::vector<numerical::Matrix<N, N, T>> f_predict;
        std::vector<numerical::Vector<N, T>> x_predict;
        std::vector<numerical::Matrix<N, N, T>> p_predict;
        std::vector<numerical::Vector<N, T>> x;
        std::vector<numerical::Matrix<N, N, T>> p;
        std::vector<T> time;
};

template <std::size_t N, typename T>
void init(const filters::UpdateDetails<N, T>& details, Vectors<N, T>& v)
{
        ASSERT(!details.f_predict);
        ASSERT(!details.x_predict);
        ASSERT(!details.p_predict);

        v.f_predict.clear();
        v.x_predict.clear();
        v.p_predict.clear();
        v.x.clear();
        v.p.clear();
        v.time.clear();

        v.f_predict.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        v.x_predict.push_back(numerical::Vector<N, T>(0));
        v.p_predict.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        v.x.push_back(details.x_update);
        v.p.push_back(details.p_update);
        v.time.push_back(details.time);
}

template <std::size_t N, typename T>
void add(const filters::UpdateDetails<N, T>& details, Vectors<N, T>& v)
{
        ASSERT(details.f_predict);
        ASSERT(details.x_predict);
        ASSERT(details.p_predict);

        v.f_predict.push_back(*details.f_predict);
        v.x_predict.push_back(*details.x_predict);
        v.p_predict.push_back(*details.p_predict);
        v.x.push_back(details.x_update);
        v.p.push_back(details.p_update);
        v.time.push_back(details.time);
}

template <std::size_t N, typename T, std::size_t ORDER>
void write_smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const Vectors<N, T>& v,
        view::Filter<2, T>* const data)
{
        const auto [x, p] = core::smooth(v.f_predict, v.x_predict, v.p_predict, v.x, v.p);

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == v.time.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                data->position.push_back({
                        .time = v.time[i],
                        .point = filter.x_to_position(x[i]),
                });

                data->position_p.push_back({
                        .time = v.time[i],
                        .point = filter.p_to_position_p(p[i]),
                });

                if constexpr (ORDER > 0)
                {
                        data->speed.push_back({
                                .time = v.time[i],
                                .point = numerical::Vector<1, T>(filter.x_to_speed(x[i])),
                        });
                }
        }
}
}

template <std::size_t N, typename T, std::size_t ORDER>
void smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const std::vector<filters::UpdateDetails<N, T>>& details,
        view::Filter<2, T>* const data)
{
        static_assert(N >= 2);
        static_assert((N % 2) == 0);

        if (details.empty())
        {
                return;
        }

        Vectors<N, T> vectors;

        init(details[0], vectors);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& f_p = details[i].f_predict;
                const auto& x_p = details[i].x_predict;
                const auto& p_p = details[i].p_predict;

                if (!(f_p && x_p && p_p))
                {
                        write_smooth(filter, vectors, data);
                        init(details[i], vectors);
                        continue;
                }

                add(details[i], vectors);
        }

        write_smooth(filter, vectors, data);
}

#define TEMPLATE(T)                                                                                        \
        template void smooth(                                                                              \
                const filters::FilterPosition<2, T, 0>&, const std::vector<filters::UpdateDetails<2, T>>&, \
                view::Filter<2, T>*);                                                                      \
        template void smooth(                                                                              \
                const filters::FilterPosition<2, T, 1>&, const std::vector<filters::UpdateDetails<4, T>>&, \
                view::Filter<2, T>*);                                                                      \
        template void smooth(                                                                              \
                const filters::FilterPosition<2, T, 2>&, const std::vector<filters::UpdateDetails<6, T>>&, \
                view::Filter<2, T>*);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)

}
