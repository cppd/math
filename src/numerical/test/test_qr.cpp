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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/numerical/qr.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <span>
#include <tuple>
#include <vector>

namespace ns::numerical
{
namespace
{
template <typename T>
bool equal(const T a, const T b, const T precision)
{
        if (a == b)
        {
                return true;
        }
        const T abs = std::abs(a - b);
        if (abs < precision)
        {
                return true;
        }
        const T rel = abs / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <typename T>
T norm(const std::vector<T>& x)
{
        ASSERT(!x.empty());
        T sum = 0;
        for (std::size_t i = 0; i < x.size(); ++i)
        {
                const T v = x[i];
                sum += v * v;
        }
        return std::sqrt(sum);
}

template <typename T>
void test_reflection(const T precision, PCG& engine)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        std::uniform_int_distribution<std::size_t> uid(1, 100);

        auto [x, norm_x] = [&]
        {
                std::vector<T> res(uid(engine));
                T n;
                do
                {
                        for (T& v : res)
                        {
                                v = urd(engine);
                        }
                        n = norm(res);
                } while (!(n > T{1e-1}));
                return std::tuple(res, n);
        }();

        HouseholderReflection<T> hr;
        hr.v = x;
        householder_reflection(hr);

        reflect(hr, std::span(x));

        if (!equal(std::abs(x[0]), norm_x, precision))
        {
                error("abs((Px)[0]) = " + to_string(std::abs(x[0]))
                      + " is not equal to norm(x) = " + to_string(norm_x));
        }

        if (!equal(norm(x), norm_x, precision))
        {
                error("norm(Px) = " + to_string(norm(x)) + " is not equal to norm(x) = " + to_string(norm_x));
        }

        if (!equal(x[0], hr.sigma, precision))
        {
                error("(Px)[0] = " + to_string(x[0]) + " is not equal to sigma = " + to_string(hr.sigma));
        }
}

void test_qr()
{
        PCG engine;

        LOG("Test QR decomposition");
        for (int i = 0; i < 100; ++i)
        {
                test_reflection<float>(1e-5, engine);
                test_reflection<double>(1e-14, engine);
                test_reflection<long double>(1e-17, engine);
        }
        LOG("Test QR decomposition passed");
}

TEST_SMALL("QR Decomposition", test_qr)
}
}
