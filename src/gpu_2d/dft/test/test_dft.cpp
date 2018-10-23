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

#include "test_dft.h"

#if defined(CUDA_FOUND)
#include "dft_cufft.h"
#endif
#if defined(FFTW_FOUND)
#include "dft_fftw.h"
#endif

#include "com/error.h"
#include "com/file/file.h"
#include "com/file/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/random/engine.h"
#include "com/time.h"
#include "gpu_2d/dft/comp/dft_gl2d.h"
#include "window/opengl/window.h"

#include <array>
#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using complex = std::complex<float>;

#if defined(CUDA_FOUND) || defined(FFTW_FOUND)
constexpr double DISCREPANCY_LIMIT = 1e-4;
#endif

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

void check_discrepancy(const std::string& name, const std::vector<complex>& d1, const std::vector<complex>& d2)
{
        double d = discrepancy(d1, d2);
        LOG("Discrepancy: " + to_string(d));

        if (d > DISCREPANCY_LIMIT)
        {
                error("Huge discrepancy (" + name + ")");
        }
}
#endif

void load_data(const std::string& file_name, int* n1, int* n2, std::vector<complex>* data)
{
        constexpr int MAX_DIMENSION_SIZE = 1e9;

        CFile f(file_name, "r");

        int v1, v2;

        if (2 != fscanf(f, "%d%d", &v1, &v2))
        {
                error("Data dimensions read error");
        }

        if (v1 < 1 || v2 < 1)
        {
                error("Dimensions must be positive numbers");
        }
        if (v1 > MAX_DIMENSION_SIZE || v2 > MAX_DIMENSION_SIZE)
        {
                error("Dimensions are too big");
        }

        const long long count = v1 * v2;

        LOG("Loading " + to_string(v1) + "x" + to_string(v2) + ", total number count " + to_string(count));

        std::vector<complex> x(count);

        for (long long i = 0; i < count; ++i)
        {
                double real, imag;
                if (2 != fscanf(f, "%lf%lf", &real, &imag))
                {
                        error("Error read number № " + std::to_string(i));
                }
                x[i] = complex(real, imag);
        }

        *n1 = v1;
        *n2 = v2;
        *data = std::move(x);
}

void save_data(const std::string& file_name, const std::vector<complex>& x)
{
        if (file_name.empty())
        {
                LOG("Data: " + to_string(x));
                return;
        }

        CFile f(file_name, "w");

        for (const complex& c : x)
        {
                double r = c.real();
                double i = c.imag();
                fprintf(f, "%18.15f %18.15f\n", r, i);
        }
}

void generate_random_data(const std::string& file_name, int n1, int n2)
{
        if (n1 < 1 || n2 < 1)
        {
                error("Wrong size " + to_string(n1) + " " + to_string(n2));
        }

        LOG("Generating " + to_string(n1) + "x" + to_string(n2) + ", total number count " + to_string(n1 * n2));

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

std::string time_string(double start_time)
{
        return to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms";
}

void compute_gl2d(bool inverse, int n1, int n2, std::vector<complex>* data)
{
        LOG("----- GL2D -----");
        double start_time = time_in_seconds();

        std::unique_ptr<IFourierGL1> gl2d = create_fft_gl2d(n1, n2);
        gl2d->exec(inverse, data);

        LOG("GL2D time: " + time_string(start_time));
}

#if defined(CUDA_FOUND)
void compute_cuda(bool inverse, int n1, int n2, std::vector<complex>* data)
{
        LOG("----- cuFFT -----");
        double start_time = time_in_seconds();

        std::unique_ptr<IFourierCuda> cufft = create_fft_cufft(n1, n2);
        cufft->exec(inverse, data);

        LOG("cuFFT time: " + time_string(start_time));
}
#endif

#if defined(FFTW_FOUND)
void compute_fftw(bool inverse, int n1, int n2, std::vector<complex>* data)
{
        LOG("----- FFTW -----");
        double start_time = time_in_seconds();

        std::unique_ptr<IFourierFFTW> fftw = create_dft_fftw(n1, n2);
        fftw->exec(inverse, data);

        LOG("FFTW time: " + time_string(start_time));
}
#endif

void dft_test(const int n1, const int n2, const std::vector<complex>& source_data, ProgressRatio* progress,
              const std::string& output_gl2d_file_name, const std::string& output_inverse_gl2d_file_name,
              [[maybe_unused]] const std::string& output_cuda_file_name,
              [[maybe_unused]] const std::string& output_inverse_cuda_file_name,
              [[maybe_unused]] const std::string& output_fftw_file_name,
              [[maybe_unused]] const std::string& output_inverse_fftw_file_name)
{
        int computation_count = 2;

#if defined(CUDA_FOUND)
        computation_count += 2;
#endif
#if defined(FFTW_FOUND)
        computation_count += 2;
#endif

        int computation = 0;
        progress->set(0, computation_count);

        std::vector<complex> data_gl2d(source_data);
        compute_gl2d(false, n1, n2, &data_gl2d);
        save_data(output_gl2d_file_name, data_gl2d);

        progress->set(++computation, computation_count);

        std::vector<complex> data_gl2d_inverse(data_gl2d);
        compute_gl2d(true, n1, n2, &data_gl2d_inverse);
        save_data(output_inverse_gl2d_file_name, data_gl2d_inverse);

        progress->set(++computation, computation_count);

#if defined(CUDA_FOUND)
        {
                std::vector<complex> data(source_data);

                compute_cuda(false, n1, n2, &data);
                save_data(output_cuda_file_name, data);
                check_discrepancy("cuFFT", data_gl2d, data);

                progress->set(++computation, computation_count);

                compute_cuda(true, n1, n2, &data);
                save_data(output_inverse_cuda_file_name, data);
                check_discrepancy("Inverse cuFFT", data_gl2d_inverse, data);

                progress->set(++computation, computation_count);
        }
#endif

#if defined(FFTW_FOUND)
        {
                std::vector<complex> data(source_data);

                compute_fftw(false, n1, n2, &data);
                save_data(output_fftw_file_name, data);
                check_discrepancy("FFTW", data_gl2d, data);

                progress->set(++computation, computation_count);

                compute_fftw(true, n1, n2, &data);
                save_data(output_inverse_fftw_file_name, data);
                check_discrepancy("Inverse FFTW", data_gl2d_inverse, data);

                progress->set(++computation, computation_count);
        }
#endif
}

void constant_data_test(ProgressRatio* progress)
{
        // Fourier[{1, 2, 30}, FourierParameters -> {1, -1}]
        // 1 2 30 -> 33. + 0. I, -15. + 24.2487 I, -15. - 24.2487 I
        // 1 2 -> 3 -1

        LOG("\n----- Context For Constant Data DFT Tests -----");

        OpenGLContext opengl_context;

        LOG("\n----- Constant Data DFT Tests -----");

        // const std::vector<complex> source_data(
        //        {{1, 0}, {2, 0}, {30, 0}, {4, 0}}); //, {-20, 0}, {3, 0}}); //, {-5, 0}});//, {-1, 0}});
        const std::vector<complex> source_data({{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}});
        const int N = source_data.size() / 3;
        const int K = source_data.size() / N;

        LOG("--- Source Data ---\n" + to_string(source_data));

        dft_test(N, K, source_data, progress, "", "", "", "", "", "");

        LOG("---\nDFT check passed");
}

void random_data_test(const std::array<int, 2>& dimensions, ProgressRatio* progress)
{
        LOG("\n----- Context For Random Data DFT Tests -----");

        OpenGLContext opengl_context;

        LOG("\n----- Random Data DFT Tests -----");

        const std::string tmp_dir = temp_directory();
        const std::string input_file_name = tmp_dir + "/dft_input.txt";
        const std::string gl2d_file_name = tmp_dir + "/dft_output_gl2d.txt";
        const std::string cuda_file_name = tmp_dir + "/dft_output_cuda.txt";
        const std::string fftw_file_name = tmp_dir + "/dft_output_fftw.txt";
        const std::string inverse_gl2d_file_name = tmp_dir + "/dft_output_inverse_gl2d.txt";
        const std::string inverse_cuda_file_name = tmp_dir + "/dft_output_inverse_cuda.txt";
        const std::string inverse_fftw_file_name = tmp_dir + "/dft_output_inverse_fftw.txt";

        generate_random_data(input_file_name, dimensions[0], dimensions[1]);

        int n1, n2;
        std::vector<complex> source_data;
        load_data(input_file_name, &n1, &n2, &source_data);

        if (dimensions[0] != n1 || dimensions[1] != n2)
        {
                error("Error test data dimensions: saved to file (" + to_string(dimensions[0]) + ", " + to_string(dimensions[1]) +
                      "), loaded from file (" + to_string(n1) + ", " + to_string(n2) + ")");
        }

        dft_test(n1, n2, source_data, progress, gl2d_file_name, inverse_gl2d_file_name, cuda_file_name, inverse_cuda_file_name,
                 fftw_file_name, inverse_fftw_file_name);

        LOG("---\nDFT check passed");
}

enum class TestSize
{
        Small,
        Big
};

TestSize find_test_size()
{
        RandomEngineWithSeed<std::mt19937_64> engine;
        std::uniform_int_distribution<int> uid(1, 20);
        return (uid(engine) != 1) ? TestSize::Small : TestSize::Big;
}

std::array<int, 2> find_dimensions(TestSize test_size)
{
        switch (test_size)
        {
        case TestSize::Small:
        {
                RandomEngineWithSeed<std::mt19937_64> engine;
                std::uniform_int_distribution<int> uid(1, 100);
                return {uid(engine), uid(engine)};
        }
        case TestSize::Big:
        {
                int n1 = 3001; // 3001; // 61;//1001; 1009
                int n2 = 997; // 61;//997;
                return {n1, n2};
        }
        }
        error_fatal("Unknown DFT test size");
}
}

void test_dft(ProgressRatio* progress)
{
        ASSERT(progress);

        // progress два раза проходит от начала до конца для двух типов данных

        constant_data_test(progress);

        const TestSize test_size = find_test_size();
        const std::array<int, 2> dimensions = find_dimensions(test_size);
        random_data_test(dimensions, progress);
}
