/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/log.h"
#include "com/random/engine.h"
#include "com/vec.h"
#include "path_tracing/coefficient/cosine_sphere.h"
#include "path_tracing/sampling/sphere.h"

#include <random>
#include <sstream>

namespace
{
void test_compare_with_beta()
{
        LOG("Compare with beta");

        for (unsigned n = 2; n <= 10000; ++n)
        {
                long double beta = std::betal(0.5l, (n - 1) / 2.0l) / std::betal(1.0l, (n - 1) / 2.0l);
                long double func = cosine_sphere_coefficient(n);
                long double discrepancy_percent = std::abs(beta - func) / func * 100;

                if (discrepancy_percent > 1e-10)
                {
                        std::ostringstream oss;
                        oss << std::fixed;
                        oss << std::setprecision(std::numeric_limits<long double>::max_digits10);
                        oss << "N = " << n << ": beta = " << beta << ", func = " << func;
                        oss << std::scientific;
                        oss << std::setprecision(5);
                        oss << ", discrepancy = " << discrepancy_percent << "%";

                        LOG(oss.str());

                        error("Huge discrepancy between beta and function: " + to_string(discrepancy_percent) + "%");
                }
        }

        LOG("Check passed");
}

template <size_t N, typename T>
void test()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int count = 10000000;

        RandomEngineWithSeed<std::mt19937_64> engine;

        long double sum = 0;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v;
                T length_square;
                random_in_sphere(engine, v, length_square);

                // косинус угла между вектором и последней координатной осью
                T c = v[N - 1] / std::sqrt(length_square);

                sum += std::abs(c);
        }

        long double computed = static_cast<long double>(count) / sum;
        long double formula = cosine_sphere_coefficient(N);
        long double discrepancy_percent = std::abs(computed - formula) / formula * 100;

        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(std::numeric_limits<T>::max_digits10);
        oss << std::setw(2) << N << ": computed = " << computed << ", formula = " << formula;
        oss << std::setprecision(5);
        oss << ", discrepancy = " << discrepancy_percent << "%";

        LOG(oss.str());

        if (discrepancy_percent > 0.1)
        {
                error("Huge discrepancy between data and function: " + to_string(discrepancy_percent) + "%");
        }
}

template <typename T>
void test()
{
        LOG(std::string("Compare with data, ") + type_name<T>());

        test<2, T>();
        test<3, T>();
        test<4, T>();
        test<5, T>();
        test<6, T>();
        test<7, T>();
        test<8, T>();
        test<9, T>();
        test<10, T>();
        test<11, T>();
        test<12, T>();
        test<13, T>();
        test<14, T>();
        test<15, T>();

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
