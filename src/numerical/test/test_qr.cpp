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
std::vector<std::vector<T>> random_matrix(const std::size_t row, const std::size_t col, PCG& engine)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<std::vector<T>> res;
        res.resize(col);
        for (std::size_t c = 0; c < col; ++c)
        {
                res[c].resize(row);
                for (std::size_t r = 0; r < row; ++r)
                {
                        res[c][r] = urd(engine);
                }
        }
        return res;
}

template <typename T>
std::vector<T> random_vector(const std::size_t size, PCG& engine)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<T> res(size);
        for (std::size_t i = 0; i < size; ++i)
        {
                res[i] = urd(engine);
        }
        return res;
}

template <typename T>
std::vector<T> mul(const std::vector<std::vector<T>>& a, const std::vector<T>& b)
{
        ASSERT(a.size() == b.size());
        ASSERT(!a.empty());

        const std::size_t cols = a.size();
        const std::size_t rows = a[0].size();
        ASSERT(rows > 0);

        std::vector<T> res(rows);
        for (std::size_t r = 0; r < rows; ++r)
        {
                res[r] = 0;
                for (std::size_t c = 0; c < cols; ++c)
                {
                        ASSERT(a[c].size() == rows);
                        res[r] += a[c][r] * b[c];
                }
        }
        return res;
}

template <typename T>
void test_reflection(const T precision, PCG& engine)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        std::uniform_int_distribution<std::size_t> uid(1, 100);

        const auto [data, data_norm] = [&]
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

        const HouseholderReflection<T> hr = [&]
        {
                HouseholderReflection<T> res;
                res.v = data;
                householder_reflection(res);
                return res;
        }();

        std::vector<T> x = data;
        reflect(hr, std::span(x));

        if (!equal(std::abs(x[0]), data_norm, precision))
        {
                error("abs((Px)[0]) = " + to_string(std::abs(x[0]))
                      + " is not equal to norm(x) = " + to_string(data_norm));
        }

        if (!equal(norm(x), data_norm, precision))
        {
                error("norm(Px) = " + to_string(norm(x)) + " is not equal to norm(x) = " + to_string(data_norm));
        }

        if (!equal(x[0], hr.sigma, precision))
        {
                error("(Px)[0] = " + to_string(x[0]) + " is not equal to sigma = " + to_string(hr.sigma));
        }
}

template <typename T>
void test_solve(const std::size_t size, const T precision, PCG& engine)
{
        const std::vector<std::vector<T>> a = random_matrix<T>(size, size, engine);
        const std::vector<T> b = random_vector<T>(size, engine);

        const std::vector<T> x = [&]
        {
                std::vector<std::vector<T>> as = a;
                std::vector<T> bs = b;
                return solve_qr(as, bs);
        }();

        if (x.size() != size)
        {
                error("Solution size (" + to_string(x.size()) + ") is not equal to system size (" + to_string(size)
                      + ")");
        }

        const std::vector<T> multiplied = mul(a, x);

        for (std::size_t i = 0; i < size; ++i)
        {
                if (!equal(multiplied[i], b[i], precision))
                {
                        error("Failed to solve:\nx = " + to_string(x) + "\nb = " + to_string(b)
                              + "\na*x = " + to_string(multiplied));
                }
        }
}

template <typename T>
void test_solve_ls(const std::size_t size, const T precision, PCG& engine)
{
        const std::size_t n = size;
        const std::size_t m = 1000 * n;

        const std::vector<std::vector<T>> a = random_matrix<T>(m, n, engine);
        const std::vector<T> x_truth = random_vector<T>(n, engine);

        const std::vector<T> b = [&]
        {
                std::normal_distribution<T> nd(0, 1);
                std::vector<T> res = mul(a, x_truth);
                for (T& v : res)
                {
                        v += nd(engine);
                }
                return res;
        }();

        const std::vector<T> x = [&]
        {
                std::vector<std::vector<T>> as = a;
                std::vector<T> bs = b;
                return solve_qr(as, bs);
        }();

        if (x.size() != n)
        {
                error("Solution size (" + to_string(x.size()) + ") is not equal to system size (" + to_string(n) + ")");
        }

        for (std::size_t i = 0; i < n; ++i)
        {
                if (!equal(x_truth[i], x[i], precision))
                {
                        error("Failed to solve:\nx truth = " + to_string(x_truth) + "\nx = " + to_string(x));
                }
        }
}

template <typename T>
void test_solve(const T precision, PCG& engine)
{
        for (std::size_t size = 1; size <= 20; ++size)
        {
                test_solve(size, precision, engine);
        }
}

template <typename T>
void test_solve_ls(const T precision, PCG& engine)
{
        for (std::size_t size = 1; size <= 10; ++size)
        {
                test_solve_ls(size, precision, engine);
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

                test_solve<double>(1e-7, engine);
                test_solve<long double>(1e-10, engine);
        }

        test_solve_ls<double>(1e-1, engine);
        test_solve_ls<long double>(1e-1, engine);

        LOG("Test QR decomposition passed");
}

TEST_SMALL("QR Decomposition", test_qr)
}
}
