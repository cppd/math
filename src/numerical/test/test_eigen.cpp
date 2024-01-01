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

#include "../eigen.h"
#include "../matrix.h"
#include "../vector.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/progress/progress.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace ns::numerical
{
namespace
{
template <std::size_t N, typename T>
[[nodiscard]] bool equal(const Vector<N, T>& a, const Vector<N, T>& b, const T precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(std::abs(a[i] - b[i]) < precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
[[nodiscard]] bool equal(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (a == b)
        {
                return true;
        }
        const T rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <std::size_t N, typename T>
struct MatrixWithDeterminant final
{
        Matrix<N, N, T> matrix;
        T determinant;
};

template <std::size_t N, typename T, typename RandomEngine>
void set_matrix(MatrixWithDeterminant<N, T>* const m, std::uniform_real_distribution<T>& urd, RandomEngine& engine)
{
        do
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        m->matrix(i, i) = urd(engine);
                        for (std::size_t j = i + 1; j < N; ++j)
                        {
                                const T v = urd(engine);
                                m->matrix(i, j) = v;
                                m->matrix(j, i) = v;
                        }
                }
                m->determinant = m->matrix.determinant();
        } while (!(std::isfinite(m->determinant) && m->determinant >= T{0.001}));
}

template <std::size_t N, typename T>
std::vector<MatrixWithDeterminant<N, T>> random_symmetric_matrices(const std::size_t count, const T min, const T max)
{
        PCG engine;
        std::uniform_real_distribution<T> urd(min, max);

        std::vector<MatrixWithDeterminant<N, T>> res;
        res.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                set_matrix(&res.emplace_back(), urd, engine);
        }
        return res;
}

void test_eigen_defined()
{
        constexpr double TOLERANCE = 1e-10;
        constexpr double PRECISION = 1e-8;

        Matrix<3, 3, double> a;
        a.row(0) = Vector<3, double>(1.2, 3.4, 5.6);
        a.row(1) = Vector<3, double>(3.4, 7.8, 9.10);
        a.row(2) = Vector<3, double>(5.6, 9.10, 11.12);

        const Eigen eigen = eigen_symmetric_upper_triangular(a, TOLERANCE);

        if (!equal(eigen.values, Vector<3, double>(-1.453829508, 0.722976163, 20.850853345), PRECISION))
        {
                error("Eigenvalues error");
        }

        if (!equal(eigen.vectors[0], Vector<3, double>(0.831214283, 0.203404459, -0.517406456), PRECISION))
        {
                error("Eigenvalues error");
        }
        if (!equal(eigen.vectors[1], Vector<3, double>(-0.458978533, 0.776240332, -0.432191683), PRECISION))
        {
                error("Eigenvalues error");
        }
        if (!equal(eigen.vectors[2], Vector<3, double>(0.313722043, 0.596722357, 0.738580332), PRECISION))
        {
                error("Eigenvalues error");
        }
}

template <std::size_t N, typename T>
void test_eigen_random(const std::size_t count)
{
        constexpr T TOLERANCE = Limits<T>::epsilon() * 100;
        constexpr T PRECISION = 0.01;

        for (const MatrixWithDeterminant<N, T>& m : random_symmetric_matrices<N, T>(count, -1, 1))
        {
                const Eigen eigen = eigen_symmetric_upper_triangular(m.matrix, TOLERANCE);

                const T trace = m.matrix.trace();
                const T det = m.determinant;

                T sum = 0;
                T product = 1;
                for (std::size_t i = 0; i < N; ++i)
                {
                        sum += eigen.values[i];
                        product *= eigen.values[i];
                }

                if (!equal(trace, sum, PRECISION))
                {
                        error(std::string("Eigenvalues error for ") + type_name<T>() + ": trace " + to_string(trace)
                              + " and sum " + to_string(sum) + " are not equal");
                }

                if (!equal(det, product, PRECISION))
                {
                        error(std::string("Eigenvalues error for ") + type_name<T>() + ": determinant " + to_string(det)
                              + " and product " + to_string(product) + " are not equal");
                }
        }
}

template <typename T>
void test_eigen_random(const std::size_t count)
{
        test_eigen_random<3, T>(count);
        test_eigen_random<4, T>(count);
        test_eigen_random<5, T>(count);
}

void test(progress::Ratio* const progress)
{
        LOG("Test eigenvalues and eigenvectors");
        progress->set(0);

        test_eigen_defined();
        test_eigen_random<float>(100);
        progress->set(1, 3);

        test_eigen_random<double>(100);
        progress->set(2, 3);

        test_eigen_random<long double>(100);
        progress->set(3, 3);

        LOG("Test eigenvalues and eigenvectors passed");
}

void test_eigen(progress::Ratio* const progress)
{
        try
        {
                test(progress);
        }
        catch (const EigenException& e)
        {
                error(e.what());
        }
}

TEST_SMALL("Eigen", test_eigen)
}
}
