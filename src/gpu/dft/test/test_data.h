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

#include <complex>
#include <filesystem>
#include <vector>

namespace ns::gpu::dft::test
{
template <typename T>
struct LoadData final
{
        int n1;
        int n2;
        std::vector<std::complex<T>> data;
};

template <typename T>
LoadData<T> load_data(const std::filesystem::path& file_name);

template <typename T>
void save_data(const std::filesystem::path& file_name, const std::vector<std::complex<T>>& x);

template <typename T>
void generate_random_data(const std::filesystem::path& file_name, int n1, int n2);
}
