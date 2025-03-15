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
template <std::size_t N, typename T>
void smooth(const std::vector<filters::UpdateDetails<N, T>>& details, view::Filter<2, T>* const data)
{
        static_assert(N >= 2);
        static_assert((N % 2) == 0);

        static constexpr std::size_t SLICE_STEP = N / 2;

        if (details.empty())
        {
                return;
        }

        std::vector<numerical::Matrix<N, N, T>> f_predict;
        std::vector<numerical::Vector<N, T>> x_predict;
        std::vector<numerical::Matrix<N, N, T>> p_predict;

        std::vector<numerical::Vector<N, T>> x;
        std::vector<numerical::Matrix<N, N, T>> p;

        std::vector<T> time;

        const auto init = [&](const std::size_t i)
        {
                ASSERT(!details[i].f_predict);
                ASSERT(!details[i].x_predict);
                ASSERT(!details[i].p_predict);

                f_predict.clear();
                x_predict.clear();
                p_predict.clear();
                x.clear();
                p.clear();
                time.clear();

                f_predict.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
                x_predict.push_back(numerical::Vector<N, T>(0));
                p_predict.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));

                x.push_back(details[i].x_update);
                p.push_back(details[i].p_update);

                time.push_back(details[i].time);
        };

        const auto write = [&]
        {
                for (std::size_t i = 0; i < x.size(); ++i)
                {
                        data->position.push_back({
                                .time = time[i],
                                .point = numerical::slice<0, SLICE_STEP>(x[i]),
                        });
                }
        };

        const auto smooth = [&]
        {
                std::tie(x, p) = core::smooth(f_predict, x_predict, p_predict, x, p);

                write();
        };

        init(0);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& f_p = details[i].f_predict;
                const auto& x_p = details[i].x_predict;
                const auto& p_p = details[i].p_predict;

                if (!(f_p && x_p && p_p))
                {
                        smooth();
                        init(i);
                        continue;
                }

                f_predict.push_back(*f_p);
                x_predict.push_back(*x_p);
                p_predict.push_back(*p_p);

                x.push_back(details[i].x_update);
                p.push_back(details[i].p_update);

                time.push_back(details[i].time);
        }

        smooth();
}

#define TEMPLATE(T)                                                                                  \
        template void smooth(const std::vector<filters::UpdateDetails<2, T>>&, view::Filter<2, T>*); \
        template void smooth(const std::vector<filters::UpdateDetails<4, T>>&, view::Filter<2, T>*); \
        template void smooth(const std::vector<filters::UpdateDetails<6, T>>&, view::Filter<2, T>*);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)

}
