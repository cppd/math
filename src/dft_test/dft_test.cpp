/*
Copyright (C) 2017 Topological Manifold

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

#include "dft_test.h"

#if defined(CUDA_FOUND)
#include "dft_cufft.h"
#endif
#if defined(FFTW_FOUND)
#include "dft_fftw.h"
#endif

#include "com/error.h"
#include "com/file.h"
#include "com/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/random.h"
#include "com/time.h"
#include "dft_comp/dft_gl2d.h"
#include "window/window.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using complex = std::complex<float>;

constexpr double DISCREPANCY_LIMIT = 1e-4;

namespace
{
#if defined(CUDA_FOUND) || defined(FFTW_FOUND)
double discrepancy(const std::vector<complex>& x1, const std::vector<complex>& x2)
{
        if (x1.size() != x2.size())
        {
                error("discrepancy size error: input " + std::to_string(x1.size()) + ", " + std::to_string(x2.size()));
        }

        double sum = 0;
        double sum2 = 0;
        for (size_t i = 0; i < x1.size(); ++i)
        {
                sum += std::abs(x1[i] - x2[i]);
                sum2 += std::abs(x1[i]);
        }
        return sum / sum2;
}
#endif

std::vector<complex> load_data(const std::string& file_name, int* n1, int* n2)
{
        CFile f(file_name, "r");

        int v1, v2;

        if (!fscanf(f, "%d%d", &v1, &v2))
        {
                error("Data dimensions read error");
        }

        if (v1 <= 0 || v2 <= 0)
        {
                error("Dimensions must be positive numbers");
        }

        const size_t count = v1 * v2;

        LOG("Loading " + to_string(v1) + "x" + to_string(v2) + ", total number count " + to_string(count));

        std::vector<complex> x(count);

        for (size_t i = 0; i < count; ++i)
        {
                double real, imag;
                if (!fscanf(f, "%lf%lf", &real, &imag))
                {
                        error("Error read number № " + std::to_string(i));
                }
                x[i] = complex(real, imag);
        }

        *n1 = v1;
        *n2 = v2;

        return x;
}

void save_data(const std::string& file_name, const std::vector<complex>& x)
{
        CFile f(file_name, "w");

        // fprintf(f, "%ld\n", static_cast<long>(x.size()));

        for (const complex& c : x)
        {
                double r = c.real();
                double i = c.imag();
                fprintf(f, "%18.15f %18.15f\n", r, i);
        }
}

void generate_random_data(const std::string& file_name, int n1, int n2)
{
        std::mt19937_64 gen((static_cast<long long>(n1) << 32) + n2);
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        CFile f(file_name, "w");

        fprintf(f, "%ld %ld\n", static_cast<long>(n1), static_cast<long>(n2));

        for (int i = 0; i < n1 * n2; ++i)
        {
                double real = urd(gen);
                double imag = urd(gen);
                fprintf(f, "%18.15f %18.15f\n", real, imag);
        }
}

void test_fft_impl(bool big_test)
{
#if 0

        std::unique_ptr<sf::Context> context;
        create_gl_context_1x1(MAJOR_GL_VERSION, MINOR_GL_VERSION, required_extensions(), &context);

        // const std::vector<complex> data(
        //        {{1, 0}, {2, 0}, {30, 0}, {4, 0}}); //, {-20, 0}, {3, 0}}); //, {-5, 0}});//, {-1, 0}});
        const std::vector<complex> data({{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}});
        int N = data.size() / 3;
        int K = data.size() / N;

        LOG(to_string(data));

        {
                LOG("-------GL----------");
                std::unique_ptr<IFourierGL1> gl2d = create_fft_gl2d(N, K);
                std::vector<complex> data1(data);
                gl2d->exec(false, &data1);
                // gl2d->exec(true, &data1);

                LOG("-----------------");
                LOG(to_string(data1));
        }
#if defined(CUDA_FOUND)
        {
                LOG("-------CUDA--------");
                std::unique_ptr<IFourierCuda> cufft = create_fft_cufft(N, K);
                std::vector<complex> data1(data);
                cufft->exec(false, &data1);
                // cufft->exec(true, &data1);
                LOG("-----------------");
                LOG(to_string(data1));
        }
#endif
#if defined(FFTW_FOUND)
        {
                LOG("-------FFTW--------");
                std::unique_ptr<IFourierFFTW> fftw = create_dft_fftw(N, K);
                std::vector<complex> data1(data);
                fftw->exec(false, &data1);
                // cufft->exec(true, &data1);
                LOG("-----------------");
                LOG(to_string(data1));
        }
#endif

// Fourier[{1, 2, 30}, FourierParameters -> {1, -1}]
// 1 2 30 -> 33. + 0. I, -15. + 24.2487 I, -15. - 24.2487 I
// 1 2 -> 3 -1

#else
        const std::string tmp_dir = temp_directory();
        const std::string input = tmp_dir + "/input.txt";
        const std::string output_gl2d = tmp_dir + "/output_gl2d.txt";
        const std::string output_cuda = tmp_dir + "/output_cuda.txt";
        const std::string output_fftw = tmp_dir + "/output_fftw.txt";

        std::unique_ptr<sf::Context> context;
        create_gl_context_1x1(MAJOR_GL_VERSION, MINOR_GL_VERSION, required_extensions(), &context);

        LOG("-----------------");

        int n1, n2;
        if (!big_test)
        {
                std::mt19937_64 engine(get_random_seed<std::mt19937_64>());
                std::uniform_int_distribution<int> uid(1, 100);
                n1 = uid(engine);
                n2 = uid(engine);
        }
        else
        {
                n1 = 3001; // 3001; // 61;//1001; 1009
                n2 = 997; // 61;//997;
        }

        if (n1 >= 1 && n2 >= 1)
        {
                LOG("Generating " + to_string(n1) + "x" + to_string(n2) + ", total number count " + to_string(n1 * n2));
                generate_random_data(input, n1, n2);
                LOG("Data done");
        }
        else
        {
                error("Wrong size " + std::to_string(n1) + " " + std::to_string(n2));
        }

        const std::vector<complex> source_data(load_data(input, &n1, &n2));

        std::vector<complex> gl2d_x(source_data);

        {
                double start_time = get_time_seconds();

                std::unique_ptr<IFourierGL1> gl2d = create_fft_gl2d(n1, n2);
                gl2d->exec(false, &gl2d_x);
                // gl2d->exec(true, &gl2d_x);

                LOG("gl2d: " + to_string_fixed(1000.0 * (get_time_seconds() - start_time), 5) + " ms");

                save_data(output_gl2d, gl2d_x);
        }

#if defined(CUDA_FOUND)
        {
                LOG("----- Cuda -----");
                std::vector<complex> cufft_x(source_data);

                double start_time = get_time_seconds();

                std::unique_ptr<IFourierCuda> cufft = create_fft_cufft(n1, n2);
                cufft->exec(false, &cufft_x);
                // cufft->exec(true, &cufft_x);

                LOG("CUFFT: " + to_string_fixed(1000.0 * (get_time_seconds() - start_time), 5) + " ms");

                save_data(output_cuda, cufft_x);

                double d = discrepancy(gl2d_x, cufft_x);
                LOG("Discrepancy gl2d-cufft: " + to_string(d));
                if (d > DISCREPANCY_LIMIT)
                {
                        error("HUGE discrepancy");
                }
        }
#endif
#if defined(FFTW_FOUND)
        {
                LOG("----- FFTW -----");
                std::vector<complex> fftw_x(source_data);

                double start_time = get_time_seconds();

                std::unique_ptr<IFourierFFTW> fftw = create_dft_fftw(n1, n2);
                fftw->exec(false, &fftw_x);
                // fftw->exec(true, &fftw_x);

                LOG("FFTW: " + to_string_fixed(1000.0 * (get_time_seconds() - start_time), 5) + " ms");

                save_data(output_fftw, fftw_x);

                double d = discrepancy(gl2d_x, fftw_x);
                LOG("Discrepancy gl2d-FFTW: " + to_string(d));
                if (d > DISCREPANCY_LIMIT)
                {
                        error("HUGE discrepancy");
                }
        }
#endif
        LOG("check passed");

#endif
}
}

void dft_test()
{
        test_fft_impl(false);
        LOG("");
}
