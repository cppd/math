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

#include "../eigen.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>

namespace ns::numerical
{
namespace
{
bool equal(double a, double b)
{
        return std::abs(a - b) < 1e-8;
}

bool equal(const Vector<3, double>& a, const Vector<3, double>& b)
{
        for (unsigned i = 0; i < 3; ++i)
        {
                if (!equal(a[i], b[i]))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
struct MatrixWithDeterminant
{
        Matrix<N, N, T> matrix;
        T determinant;
};

template <std::size_t N, typename T>
std::vector<MatrixWithDeterminant<N, T>> random_symmetric_matrices(unsigned count, T min, T max)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        std::uniform_real_distribution<T> urd(min, max);

        std::vector<MatrixWithDeterminant<N, T>> matrices;
        matrices.reserve(count);
        for (std::size_t n = 0; n < count; ++n)
        {
                MatrixWithDeterminant<N, T>& m = matrices.emplace_back();
                do
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                m.matrix(i, i) = urd(random_engine);
                                for (unsigned j = i + 1; j < N; ++j)
                                {
                                        T v = urd(random_engine);
                                        m.matrix(i, j) = v;
                                        m.matrix(j, i) = v;
                                }
                        }
                        m.determinant = m.matrix.determinant();
                } while (!(is_finite(m.determinant) && m.determinant >= T(0.001)));
        }
        return matrices;
}

void test_eigen_defined()
{
        constexpr double TOLERANCE = 1e-10;

        Matrix<3, 3, double> a;
        a.row(0) = Vector<3, double>(1.2, 3.4, 5.6);
        a.row(1) = Vector<3, double>(3.4, 7.8, 9.10);
        a.row(2) = Vector<3, double>(5.6, 9.10, 11.12);

        Vector<3, double> eigenvalues;
        std::array<Vector<3, double>, 3> eigenvectors;

        try
        {
                numerical::eigen_symmetric_upper_triangular(a, TOLERANCE, &eigenvalues, &eigenvectors);
        }
        catch (const numerical::EigenException& e)
        {
                error(e.what());
        }

        if (!equal(eigenvalues, Vector<3, double>(-1.453829508, 0.722976163, 20.850853345)))
        {
                error("Eigenvalues error");
        }

        if (!equal(eigenvectors[0], Vector<3, double>(0.831214283, 0.203404459, -0.517406456)))
        {
                error("Eigenvalues error");
        }
        if (!equal(eigenvectors[1], Vector<3, double>(-0.458978533, 0.776240332, -0.432191683)))
        {
                error("Eigenvalues error");
        }
        if (!equal(eigenvectors[2], Vector<3, double>(0.313722043, 0.596722357, 0.738580332)))
        {
                error("Eigenvalues error");
        }
}

template <std::size_t N, typename T>
void test_eigen_random(unsigned count)
{
        constexpr T TOLERANCE = limits<T>::epsilon() * 100;

        for (const MatrixWithDeterminant<N, T>& m : random_symmetric_matrices<N, T>(count, -1, 1))
        {
                Vector<N, T> eigenvalues;
                std::array<Vector<N, T>, N> eigenvectors;
                try
                {
                        numerical::eigen_symmetric_upper_triangular(m.matrix, TOLERANCE, &eigenvalues, &eigenvectors);
                }
                catch (const numerical::EigenException& e)
                {
                        error(e.what());
                }

                const T trace = m.matrix.trace();
                const T det = m.determinant;
                T sum = 0;
                T product = 1;
                for (unsigned i = 0; i < N; ++i)
                {
                        sum += eigenvalues[i];
                        product *= eigenvalues[i];
                }
                if (std::abs((trace - sum) / std::max(std::abs(trace), std::abs(sum))) > T(0.01))
                {
                        error(std::string("Eigenvalues error for ") + type_name<T>() + ": trace " + to_string(trace)
                              + " and sum " + to_string(sum) + " are not equal");
                }
                if (std::abs((det - product) / std::max(std::abs(det), std::abs(product))) > T(0.01))
                {
                        error(std::string("Eigenvalues error for ") + type_name<T>() + ": determinant " + to_string(det)
                              + " and product " + to_string(product) + " are not equal");
                }
        }
}

template <typename T>
void test_eigen_random(unsigned count)
{
        test_eigen_random<3, T>(count);
        test_eigen_random<4, T>(count);
        test_eigen_random<5, T>(count);
}

void test_eigen(ProgressRatio* progress)
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

TEST_SMALL("Eigen", test_eigen)
}
}
