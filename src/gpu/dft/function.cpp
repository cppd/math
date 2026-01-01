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

#include "function.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <bit>
#include <complex>
#include <string>
#include <vector>

namespace ns::gpu::dft
{
int compute_m(const int n)
{
        static constexpr int MAX_POWER_OF_2 = 1 << (Limits<int>::digits() - 1);
        if (n < 1 || n > MAX_POWER_OF_2 / 2)
        {
                error("Error size " + std::to_string(n) + " for compute m");
        }

        if (std::has_single_bit(static_cast<unsigned>(n)))
        {
                return n;
        }
        return std::bit_ceil(static_cast<unsigned>(2 * n - 2));
}

// Compute the symmetric Toeplitz H: for given N, compute the scalar constants
// 13.4, 13.22.
std::vector<std::complex<double>> compute_h(const int n, const bool inverse, const double coef)
{
        std::vector<std::complex<double>> h(n);

        for (int l = 0; l <= n - 1; ++l)
        {
                // theta = (inverse ? 1 : -1) * 2 * pi / n * (-0.5 * l * l) = (inverse ? -pi : pi) / n * l * l

                // h[l] = std::polar(Coef, (inverse ? -PI : PI) / n * l * l);

                // Instead of l*l/n compute mod(l*l/n, 2) so that trigonometric
                // functions work with numbers no more than 2π.
                const long long dividend = static_cast<long long>(l) * l;
                const long long quotient = dividend / n;
                const long long remainder = dividend - quotient * n;
                // factor = (quotient mod 2) + (remainder / n).
                const double factor = (quotient & 1) + static_cast<double>(remainder) / n;

                h[l] = std::polar(coef, (inverse ? -PI<double> : PI<double>)*factor);
        }

        return h;
}

// Embed H in the circulant H(2)
// Based on corrected formulas 13.11, 13.23, 13.24, 13.25.
std::vector<std::complex<double>> compute_h2(const int n, const int m, const std::vector<std::complex<double>>& h)
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
unsigned shared_size(const unsigned dft_size, const unsigned max_shared_memory_size)
{
        // minimum of
        // 1) requested size, but not less than 128 so that a group
        //  has at least 64 threads, one thread for 2 elements.
        //  NVIDIA warp size is 32, AMD wavefront size is 64.
        // 2) bit_floor(element count)
        const unsigned s1 = std::max(128u, dft_size);
        const unsigned s2 = std::bit_floor(max_shared_memory_size / sizeof(T));
        return std::min(s1, s2);
}

template <typename T>
unsigned group_size(
        const unsigned dft_size,
        const unsigned max_group_size_x,
        const unsigned max_group_invocations,
        const unsigned max_shared_memory_size)
{
        // no more than 1 thread for 2 elements
        const unsigned max_threads_required = shared_size<T>(dft_size, max_shared_memory_size) / 2;
        const unsigned max_threads_supported = std::min(max_group_size_x, max_group_invocations);
        return std::min(max_threads_required, max_threads_supported);
}

template unsigned shared_size<std::complex<float>>(unsigned, unsigned);
template unsigned group_size<std::complex<float>>(unsigned, unsigned, unsigned, unsigned);
}
