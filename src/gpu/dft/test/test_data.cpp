/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>

#include <complex>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <random>
#include <vector>

namespace ns::gpu::dft::test
{
template <typename T>
LoadData<T> load_data(const std::filesystem::path& file_name)
{
        constexpr int MAX_DIMENSION_SIZE{1'000'000'000};

        std::fstream file(file_name);

        int n1;
        int n2;

        file >> n1 >> n2;

        if (!file)
        {
                error("Data dimensions read error");
        }

        if (n1 < 1 || n2 < 1)
        {
                error("Dimensions must be positive numbers");
        }

        if (n1 > MAX_DIMENSION_SIZE || n2 > MAX_DIMENSION_SIZE)
        {
                error("Dimensions are too big");
        }

        const long long count = static_cast<long long>(n1) * n2;

        LOG("Loading " + to_string(n1) + "x" + to_string(n2) + ", count " + to_string(count));

        std::vector<std::complex<T>> data(count);
        for (long long i = 0; i < count; ++i)
        {
                T real;
                T imag;
                file >> real >> imag;
                if (!file)
                {
                        error("Error reading number " + to_string(i));
                }
                data[i] = std::complex<T>(real, imag);
        }

        return {.n1 = n1, .n2 = n2, .data = std::move(data)};
}

template <typename T>
void save_data(const std::filesystem::path& file_name, const std::vector<std::complex<T>>& x)
{
        std::ofstream file(file_name);

        file << std::scientific;
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::showpos;
        file << std::showpoint;

        for (const std::complex<T>& c : x)
        {
                file << c.real() << ' ' << c.imag() << '\n';
        }
}

template <typename T>
void generate_random_data(const std::filesystem::path& file_name, const int n1, const int n2)
{
        if (n1 < 1 || n2 < 1)
        {
                error("Wrong size " + to_string(n1) + " " + to_string(n2));
        }

        LOG("Generating " + to_string(n1) + "x" + to_string(n2) + ", total number count " + to_string(n1 * n2));

        PCG engine((static_cast<unsigned long long>(n1) << 32) + n2);
        std::uniform_real_distribution<T> urd(-1, 1);

        std::ofstream file(file_name);

        file << n1 << ' ' << n2 << '\n';

        file << std::scientific;
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::showpos;
        file << std::showpoint;

        for (int i = 0; i < n1 * n2; ++i)
        {
                file << urd(engine) << ' ' << urd(engine) << '\n';
        }
}

template LoadData<float> load_data(const std::filesystem::path&);
template LoadData<double> load_data(const std::filesystem::path&);

template void save_data(const std::filesystem::path&, const std::vector<std::complex<float>>&);
template void save_data(const std::filesystem::path&, const std::vector<std::complex<double>>&);

template void generate_random_data<float>(const std::filesystem::path&, int, int);
template void generate_random_data<double>(const std::filesystem::path&, int, int);
}
