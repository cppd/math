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

#include <complex>
#include <vector>

int dft_compute_m(int n);
std::vector<std::complex<double>> dft_compute_h(int n, bool inverse, double coef);
std::vector<std::complex<double>> dft_compute_h2(int n, int m, const std::vector<std::complex<double>>& h);

template <typename T>
int dft_shared_size(unsigned dft_size, unsigned max_shared_memory_size);
template <typename T>
int dft_group_size(unsigned dft_size, unsigned max_group_size_x, unsigned max_group_invocations, unsigned max_shared_memory_size);
