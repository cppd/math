/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <src/dft/cufft.h>
#endif

#if defined(FFTW_FOUND)
#include <src/dft/fftw.h>
#endif

#include "../vulkan/compute.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/utility/file/file.h>
#include <src/utility/file/sys.h>
#include <src/utility/random/engine.h>

#include <array>
#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using complex = std::complex<float>;

namespace gpu::dft
{
namespace
{
#if defined(CUDA_FOUND) || defined(FFTW_FOUND)
constexpr double DISCREPANCY_LIMIT = 1e-4;
#endif

#if defined(CUDA_FOUND) || defined(FFTW_FOUND)
void compare(
        const std::string& name_compute,
        const std::string& name_library,
        const std::vector<complex>& x_compute,
        const std::vector<complex>& x_library)
{
        if (x_compute.size() != x_library.size())
        {
                error("DFT compare data size error: " + to_string(x_compute.size()) + ", "
                      + to_string(x_library.size()));
        }

        double sum = 0;
        double sum2 = 0;
        for (size_t i = 0; i < x_compute.size(); ++i)
        {
                sum += std::abs(x_compute[i] - x_library[i]);
                sum2 += std::abs(x_compute[i]);
        }

        double d = (sum == 0) ? 0 : (sum / sum2);

        LOG("Discrepancy " + name_compute + ": " + to_string(d));

        // Для NaN не работает if (!(d <= DISCREPANCY_LIMIT))
        // Для NaN работает if (d <= DISCREPANCY_LIMIT); else
        if (d <= DISCREPANCY_LIMIT)
        {
                return;
        }

        std::ostringstream oss;
        oss << "DFT failed (comparison " << name_compute << " with " << name_library << ")";
        error(oss.str());
}
#endif

void load_data(const std::string& file_name, int* n1, int* n2, std::vector<complex>* data)
{
        constexpr int MAX_DIMENSION_SIZE = 1e9;

        CFile f(file_name, "r");

        int v1;
        int v2;

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
                double real;
                double imag;
                if (2 != fscanf(f, "%lf%lf", &real, &imag))
                {
                        error("Error read number № " + to_string(i));
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

void compute_vulkan(ComputeVector* dft, bool inverse, int n1, int n2, std::vector<complex>* data)
{
        {
                RandomEngineWithSeed<std::mt19937_64> engine;
                std::uniform_int_distribution<int> uid(1, 3000);
                dft->create_buffers(uid(engine), uid(engine));
                dft->create_buffers(1, 1);
                dft->create_buffers(uid(engine), uid(engine));
                dft->create_buffers(1, uid(engine));
                dft->create_buffers(uid(engine), uid(engine));
                dft->create_buffers(uid(engine), 1);
                dft->create_buffers(uid(engine), uid(engine));
        }

        double start_time = time_in_seconds();

        dft->create_buffers(n1, n2);
        dft->exec(inverse, data);

        LOG((inverse ? "Vulkan inverse time: " : "Vulkan forward time: ") + time_string(start_time));
}

#if defined(CUDA_FOUND)
void compute_cuda(bool inverse, int n1, int n2, std::vector<complex>* data)
{
        if (inverse)
        {
                LOG("----- cuFFT inverse -----");
        }
        else
        {
                LOG("----- cuFFT forward -----");
        }

        double start_time = time_in_seconds();

        std::unique_ptr<DFT> cufft = create_cufft(n1, n2);
        cufft->exec(inverse, data);

        LOG("cuFFT time: " + time_string(start_time));
}
#endif

#if defined(FFTW_FOUND)
void compute_fftw(bool inverse, int n1, int n2, std::vector<complex>* data)
{
        if (inverse)
        {
                LOG("----- FFTW inverse -----");
        }
        else
        {
                LOG("----- FFTW forward -----");
        }

        double start_time = time_in_seconds();

        std::unique_ptr<DFT> fftw = create_fftw(n1, n2);
        fftw->exec(inverse, data);

        LOG("FFTW time: " + time_string(start_time));
}
#endif

void dft_test(
        ComputeVector* dft,
        const int n1,
        const int n2,
        const std::vector<complex>& source_data,
        ProgressRatio* progress,
        const std::string& output_vulkan_file_name,
        const std::string& output_inverse_vulkan_file_name,
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

        //

        std::vector<complex> data_vulkan(source_data);
        compute_vulkan(dft, false, n1, n2, &data_vulkan);
        save_data(output_vulkan_file_name, data_vulkan);

        progress->set(++computation, computation_count);

        std::vector<complex> data_vulkan_inverse(data_vulkan);
        compute_vulkan(dft, true, n1, n2, &data_vulkan_inverse);
        save_data(output_inverse_vulkan_file_name, data_vulkan_inverse);

        progress->set(++computation, computation_count);

        //

#if defined(CUDA_FOUND)
        {
                std::vector<complex> data(source_data);

                compute_cuda(false, n1, n2, &data);
                save_data(output_cuda_file_name, data);
                compare("Vulkan", "cuFFT", data_vulkan, data);

                progress->set(++computation, computation_count);

                compute_cuda(true, n1, n2, &data);
                save_data(output_inverse_cuda_file_name, data);
                compare("Vulkan", "cuFFT", data_vulkan_inverse, data);

                progress->set(++computation, computation_count);
        }
#endif

#if defined(FFTW_FOUND)
        {
                std::vector<complex> data(source_data);

                compute_fftw(false, n1, n2, &data);
                save_data(output_fftw_file_name, data);
                compare("Vulkan", "FFTW", data_vulkan, data);

                progress->set(++computation, computation_count);

                compute_fftw(true, n1, n2, &data);
                save_data(output_inverse_fftw_file_name, data);
                compare("Vulkan", "FFTW", data_vulkan_inverse, data);

                progress->set(++computation, computation_count);
        }
#endif
}

void constant_data_test(ComputeVector* dft, ProgressRatio* progress)
{
        // Fourier[{1, 2, 30}, FourierParameters -> {1, -1}]
        // 1 2 30 -> 33. + 0. I, -15. + 24.2487 I, -15. - 24.2487 I
        // 1 2 -> 3 -1

        LOG("\n----- Constant Data DFT Tests -----");

        // const std::vector<complex> source_data(
        //        {{1, 0}, {2, 0}, {30, 0}, {4, 0}}); //, {-20, 0}, {3, 0}}); //, {-5, 0}});//, {-1, 0}});
        const std::vector<complex> source_data({{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}});
        const int N = source_data.size() / 3;
        const int K = source_data.size() / N;

        LOG("--- Source Data ---\n" + to_string(source_data));

        dft_test(dft, N, K, source_data, progress, "", "", "", "", "", "");

        LOG("---\nDFT check passed");
}

void random_data_test(ComputeVector* dft, const std::array<int, 2>& dimensions, ProgressRatio* progress)
{
        LOG("\n----- Random Data DFT Tests -----");

        const std::string tmp_dir = temp_directory();

        const std::string input_file_name = tmp_dir + "/dft_input.txt";

        const std::string vulkan_file_name = tmp_dir + "/dft_output_vulkan.txt";
        const std::string cuda_file_name = tmp_dir + "/dft_output_cuda.txt";
        const std::string fftw_file_name = tmp_dir + "/dft_output_fftw.txt";

        const std::string inverse_vulkan_file_name = tmp_dir + "/dft_output_inverse_vulkan.txt";
        const std::string inverse_cuda_file_name = tmp_dir + "/dft_output_inverse_cuda.txt";
        const std::string inverse_fftw_file_name = tmp_dir + "/dft_output_inverse_fftw.txt";

        generate_random_data(input_file_name, dimensions[0], dimensions[1]);

        int n1;
        int n2;
        std::vector<complex> source_data;
        load_data(input_file_name, &n1, &n2, &source_data);

        if (dimensions[0] != n1 || dimensions[1] != n2)
        {
                error("Error test data dimensions: saved to file (" + to_string(dimensions[0]) + ", "
                      + to_string(dimensions[1]) + "), loaded from file (" + to_string(n1) + ", " + to_string(n2)
                      + ")");
        }

        dft_test(
                dft, n1, n2, source_data, progress, vulkan_file_name, inverse_vulkan_file_name, cuda_file_name,
                inverse_cuda_file_name, fftw_file_name, inverse_fftw_file_name);

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
        std::uniform_int_distribution<int> uid(1, 10);
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

void test(ProgressRatio* progress)
{
        ASSERT(progress);

        std::unique_ptr<ComputeVector> dft = create_compute_vector();

        // progress два раза проходит от начала до конца для двух типов данных

        constant_data_test(dft.get(), progress);

        const TestSize test_size = find_test_size();
        const std::array<int, 2> dimensions = find_dimensions(test_size);
        random_data_test(dft.get(), dimensions, progress);
}
}
