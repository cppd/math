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

#include "test_cosine_sphere.h"

#include "../cosine_sphere.h"

#include <src/com/log.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vec.h>
#include <src/random/sphere.h>

#include <cmath>
#include <random>
#include <sstream>
#include <version>

namespace ns::painter
{
namespace
{
#if defined(__cpp_lib_math_special_functions) && __cpp_lib_math_special_functions >= 201603L
long double betal(long double x, long double y)
{
        return std::betal(x, y);
}
#else
#if !defined(__clang__)
#error __cpp_lib_math_special_functions
#endif
long double betal(long double x, long double y)
{
        // Β(x, y) = Γ(x) * Γ(y) / Γ(x + y)
        // Β(x, y) = exp(log(Β(x, y)))
        // Β(x, y) = exp(log(Γ(x) * Γ(y) / Γ(x + y)))
        // Β(x, y) = exp(log(Γ(x)) + log(Γ(y)) - log(Γ(x + y)))
        return std::exp(std::lgamma(x) + std::lgamma(y) - std::lgamma(x + y));
}
#endif

void test_compare_with_beta(unsigned n)
{
        long double beta = betal(0.5L, (n - 1) / 2.0L) / betal(1.0L, (n - 1) / 2.0L);
        long double function = cosine_sphere_coefficient(n);
        long double discrepancy_percent = std::abs(beta - function) / function * 100;

        if (discrepancy_percent > 1e-10)
        {
                std::ostringstream oss;
                oss << std::fixed;
                oss << std::setprecision(limits<long double>::max_digits10);
                oss << "N = " << n << ": beta = " << beta << ", function = " << function;
                oss << std::scientific;
                oss << std::setprecision(5);
                oss << ", discrepancy = " << discrepancy_percent << "%";

                LOG(oss.str());

                error("Huge discrepancy between beta and function: " + to_string(discrepancy_percent) + "%");
        }
}

void test_compare_with_beta()
{
        LOG("Compare with beta");

        for (unsigned n = 2; n < 10000; ++n)
        {
                test_compare_with_beta(n);
        }

        for (unsigned n = 10000; n <= 1'000'000; (n & 1) == 0 ? ++n : n += 999)
        {
                test_compare_with_beta(n);
        }

        LOG("Check passed");
}

template <std::size_t N, typename T>
void test()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int count = 10000000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        long double sum = 0;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v;
                T length_square;
                random::random_in_sphere(engine, v, length_square);

                // косинус угла между вектором и последней координатной осью
                T c = v[N - 1] / std::sqrt(length_square);

                sum += std::abs(c);
        }

        long double data = static_cast<long double>(count) / sum;
        long double function = cosine_sphere_coefficient(N);
        long double discrepancy_percent = std::abs(data - function) / function * 100;

        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(limits<long double>::max_digits10);
        oss << std::setw(2) << N << ": data = " << data << ", function = " << function;
        oss << std::setprecision(5);
        oss << ", discrepancy = " << discrepancy_percent << "%";

        LOG(oss.str());

        if (discrepancy_percent > 0.1)
        {
                error("Huge discrepancy between data and function: " + to_string(discrepancy_percent) + "%");
        }
}

template <typename T, std::size_t... I>
void test(std::integer_sequence<std::size_t, I...>)
{
        (test<I + 2, T>(), ...);
}

template <typename T>
void test()
{
        LOG(std::string("Compare with data, ") + type_name<T>());

        test<T>(std::make_integer_sequence<std::size_t, 19>());

        LOG("Check passed");
}
}

void test_cosine_sphere_coefficient()
{
        test_compare_with_beta();
        LOG("");
        test<float>();
        LOG("");
        test<double>();
}
}
