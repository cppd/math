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

#include "test_data.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

#include <fstream>
#include <random>

namespace ns::gpu::dft
{
template <typename T>
void load_data(const std::filesystem::path& file_name, int* n1, int* n2, std::vector<std::complex<T>>* data)
{
        constexpr int MAX_DIMENSION_SIZE = 1e9;

        std::fstream file(file_name);

        long long v1;
        long long v2;

        file >> v1 >> v2;
        if (!file)
        {
                error("Data dimensions read error");
        }

        if (v1 < 1 || v2 < 1)
        {
                error("Dimensions must be positive numbers");
        }
        if (v1 > MAX_DIMENSION_SIZE || v2 > MAX_DIMENSION_SIZE)
        {
                error("Dimensions are too big");
        }

        const long long count = v1 * v2;

        LOG("Loading " + to_string(v1) + "x" + to_string(v2) + ", total number count " + to_string(count));

        std::vector<std::complex<T>> x(count);

        for (long long i = 0; i < count; ++i)
        {
                T real;
                T imag;
                file >> real >> imag;
                if (!file)
                {
                        error("Error read number № " + to_string(i));
                }
                x[i] = std::complex<T>(real, imag);
        }

        *n1 = v1;
        *n2 = v2;
        *data = std::move(x);
}

template <typename T>
void save_data(const std::filesystem::path& file_name, const std::vector<std::complex<T>>& x)
{
        std::ofstream file(file_name);

        file << std::scientific;
        file << std::setprecision(limits<T>::max_digits10);
        file << std::showpos;
        file << std::showpoint;

        for (const std::complex<T>& c : x)
        {
                file << c.real() << ' ' << c.imag() << '\n';
        }
}

template <typename T>
void generate_random_data(const std::filesystem::path& file_name, int n1, int n2)
{
        if (n1 < 1 || n2 < 1)
        {
                error("Wrong size " + to_string(n1) + " " + to_string(n2));
        }

        LOG("Generating " + to_string(n1) + "x" + to_string(n2) + ", total number count " + to_string(n1 * n2));

        std::mt19937_64 gen((static_cast<unsigned long long>(n1) << 32) + n2);
        std::uniform_real_distribution<T> urd(-1, 1);

        std::ofstream file(file_name);

        file << n1 << ' ' << n2 << '\n';

        file << std::scientific;
        file << std::setprecision(limits<T>::max_digits10);
        file << std::showpos;
        file << std::showpoint;

        for (int i = 0; i < n1 * n2; ++i)
        {
                file << urd(gen) << ' ' << urd(gen) << '\n';
        }
}

template void load_data(
        const std::filesystem::path& file_name,
        int* n1,
        int* n2,
        std::vector<std::complex<float>>* data);
template void load_data(
        const std::filesystem::path& file_name,
        int* n1,
        int* n2,
        std::vector<std::complex<double>>* data);

template void save_data(const std::filesystem::path& file_name, const std::vector<std::complex<float>>& x);
template void save_data(const std::filesystem::path& file_name, const std::vector<std::complex<double>>& x);

template void generate_random_data<float>(const std::filesystem::path& file_name, int n1, int n2);
template void generate_random_data<double>(const std::filesystem::path& file_name, int n1, int n2);
}
