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

#include "vector.h"

#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/interpolation.h>
#include <src/com/print.h>

#include <algorithm>
#include <array>
#include <vector>

namespace ns
{
template <std::size_t N, typename DataType, typename InterpolationType>
class Interpolation
{
        enum class Wrap
        {
                CLAMP_TO_EDGE,
                REPEATE
        };

        static constexpr Wrap WRAP = Wrap::CLAMP_TO_EDGE;

        GlobalIndex<N, long long> global_index_;
        std::array<int, N> size_;
        std::array<int, N> max_;
        std::vector<DataType> data_;

public:
        Interpolation(const std::array<int, N>& size, std::vector<DataType>&& data) : global_index_(size), size_(size)
        {
                if (!std::all_of(
                            size_.cbegin(), size_.cend(),
                            [](const int v)
                            {
                                    return v > 0;
                            }))
                {
                        error("Error interpolation size " + to_string(size_));
                }

                if (static_cast<std::size_t>(global_index_.count()) != data.size())
                {
                        error("Interpolation data size " + to_string(data.size()) + " is not equal to "
                              + to_string(global_index_.count()));
                }

                data_ = std::move(data);

                for (unsigned i = 0; i < N; ++i)
                {
                        max_[i] = size_[i] - 1;
                }
        }

        template <typename T>
        DataType compute(const Vector<N, T>& p) const
        {
                static_assert(std::is_floating_point_v<T>);

                // Vulkan: Texel Coordinate Systems, Wrapping Operation.

                std::array<int, N> x0;
                std::array<int, N> x1;
                std::array<InterpolationType, N> x;

                for (unsigned i = 0; i < N; ++i)
                {
                        const T v = p[i] * size_[i] - T(0.5);
                        const T floor = std::floor(v);

                        x[i] = v - floor;
                        x0[i] = floor;
                        x1[i] = x0[i] + 1;

                        static_assert(WRAP == Wrap::CLAMP_TO_EDGE || WRAP == Wrap::REPEATE);
                        if constexpr (WRAP == Wrap::CLAMP_TO_EDGE)
                        {
                                x0[i] = std::clamp(x0[i], 0, max_[i]);
                                x1[i] = std::clamp(x1[i], 0, max_[i]);
                        }
                        else
                        {
                                x0[i] = x0[i] % size_[i];
                                x1[i] = x1[i] % size_[i];
                        }
                }

                std::array<DataType, (1 << N)> data;

                for (unsigned i = 0; i < data.size(); ++i)
                {
                        long long index = 0;
                        for (unsigned n = 0; n < N; ++n)
                        {
                                const int coordinate = ((1 << n) & i) ? x1[n] : x0[n];
                                index += global_index_.stride(n) * coordinate;
                        }
                        data[i] = data_[index];
                }

                return interpolation(data, x);
        }
};
}
