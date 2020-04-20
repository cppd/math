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

#include <complex>
#include <vector>

namespace gpu::dft
{
int compute_m(int n);
std::vector<std::complex<double>> compute_h(int n, bool inverse, double coef);
std::vector<std::complex<double>> compute_h2(int n, int m, const std::vector<std::complex<double>>& h);

template <typename T>
int shared_size(unsigned dft_size, unsigned max_shared_memory_size);
template <typename T>
int group_size(
        unsigned dft_size,
        unsigned max_group_size_x,
        unsigned max_group_invocations,
        unsigned max_shared_memory_size);

template <typename Dst, typename Src>
std::vector<std::complex<Dst>> conv(const std::vector<std::complex<Src>>& data)
{
        if constexpr (std::is_same_v<Dst, Src>)
        {
                return data;
        }
        else
        {
                std::vector<std::complex<Dst>> res(data.size());
                for (size_t i = 0; i < data.size(); ++i)
                {
                        res[i] = {static_cast<Dst>(data[i].real()), static_cast<Dst>(data[i].imag())};
                }
                return res;
        }
}

template <typename Dst, typename Src>
std::enable_if_t<std::is_same_v<Dst, Src>, std::vector<std::complex<Dst>>&&> conv(std::vector<std::complex<Src>>&& data)
{
        return std::move(data);
}
}
