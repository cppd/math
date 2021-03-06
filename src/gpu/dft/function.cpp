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

#include "function.h"

#include <src/com/bits.h>
#include <src/com/constant.h>

namespace ns::gpu::dft
{
// Или само число степень двух,
// или минимальная степень двух, равная или больше 2N-2
int compute_m(int n)
{
        int log2_n = log_2(n);
        if ((1 << log2_n) == n)
        {
                return n;
        }

        int t = (2 * n - 2);
        int log2_t = log_2(t);
        if ((1 << log2_t) == t)
        {
                return t;
        }
        return (1 << log2_t) << 1;
}

// Compute the symmetric Toeplitz H: for given N, compute the scalar constants
// Формулы 13.4, 13.22.
std::vector<std::complex<double>> compute_h(int n, bool inverse, double coef)
{
        std::vector<std::complex<double>> h(n);

        for (int l = 0; l <= n - 1; ++l)
        {
                // theta = (inverse ? 1 : -1) * 2 * pi / n * (-0.5 * l * l) = (inverse ? -pi : pi) / n * l * l

                // h[l] = std::polar(Coef, (inverse ? -PI : PI) / n * l * l);

                // Вместо l * l / n нужно вычислить mod(l * l / n, 2), чтобы в тригонометрические функции
                // поступало не больше 2 * PI.
                long long dividend = l * l;
                long long quotient = dividend / n;
                long long remainder = dividend - quotient * n;
                // factor = (quotient mod 2) + (remainder / n).
                double factor = (quotient & 1) + static_cast<double>(remainder) / n;

                h[l] = std::polar(coef, (inverse ? -PI<double> : PI<double>)*factor);
        }

        return h;
}

// Embed H in the circulant H(2)
// На основе исправленных формул 13.11, 13.23, 13.24, 13.25.
// Об исправлении в комментарии о книге.
std::vector<std::complex<double>> compute_h2(int n, int m, const std::vector<std::complex<double>>& h)
{
        std::vector<std::complex<double>> h2(m);

        for (int l = 0; l <= n - 1; ++l)
        {
                h2[l] = h[l];
        }
        for (int l = n; l <= m - n; ++l)
        {
                h2[l] = std::complex<double>(0, 0);
        }
        for (int l = m - n + 1; l <= m - 1; ++l)
        {
                h2[l] = h[m - l];
        }
        return h2;
}

template <typename T>
int shared_size(unsigned dft_size, unsigned max_shared_memory_size)
{
        // минимум из
        // 1) требуемый размер, но не меньше 128, чтобы в группе было хотя бы 64 потока по потоку на 2 элемента:
        //   NVIDIA работает по 32 потока вместе (warp), AMD по 64 потока вместе (wavefront).
        // 2) максимальная степень 2, которая меньше или равна вместимости разделяемой памяти
        return std::min(std::max(128u, dft_size), 1u << log_2(max_shared_memory_size / sizeof(T)));
}

template <typename T>
int group_size(
        unsigned dft_size,
        unsigned max_group_size_x,
        unsigned max_group_invocations,
        unsigned max_shared_memory_size)
{
        // не больше 1 потока на 2 элемента
        int max_threads_required = shared_size<T>(dft_size, max_shared_memory_size) / 2;
        int max_threads_supported = std::min(max_group_size_x, max_group_invocations);
        return std::min(max_threads_required, max_threads_supported);
}

template int shared_size<std::complex<float>>(unsigned, unsigned);
template int group_size<std::complex<float>>(unsigned, unsigned, unsigned, unsigned);
}
