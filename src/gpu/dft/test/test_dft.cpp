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

#if defined(CUDA_FOUND)
#include <src/dft/cufft.h>
#endif

#if defined(FFTW_FOUND)
#include <src/dft/fftw.h>
#endif

#include "test_data.h"

#include "../compute.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <complex>
#include <filesystem>
#include <string>
#include <vector>

using Complex = std::complex<float>;

namespace ns::gpu::dft
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
        const std::vector<Complex>& x_compute,
        const std::vector<Complex>& x_library)
{
        if (x_compute.size() != x_library.size())
        {
                error("DFT compare data size error: " + to_string(x_compute.size()) + ", "
                      + to_string(x_library.size()));
        }

        double sum = 0;
        double sum2 = 0;
        for (std::size_t i = 0; i < x_compute.size(); ++i)
        {
                sum += std::abs(x_compute[i] - x_library[i]);
                sum2 += std::abs(x_compute[i]);
        }

        double d = (sum == 0) ? 0 : (sum / sum2);

        LOG("Discrepancy " + name_compute + ": " + to_string(d));

        if (d <= DISCREPANCY_LIMIT)
        {
                return;
        }

        error("DFT failed (comparison " + name_compute + " with " + name_library + ")");
}
#endif

std::string time_string(const std::string& name, const TimePoint& start_time)
{
        return name + " time: " + to_string_fixed(1000.0 * duration_from(start_time), 5) + " ms";
}

void log_data(const std::vector<Complex>& data)
{
        if (data.size() > 10)
        {
                return;
        }
        std::string s = "Data:";
        if (!data.empty())
        {
                for (const Complex& c : data)
                {
                        s += "\n  " + to_string(c);
                }
        }
        else
        {
                s += " empty";
        }
        LOG(s);
}

void compute_vulkan(ComputeVector* dft, bool inverse, int n1, int n2, std::vector<Complex>* data)
{
        if (inverse)
        {
                LOG("----- Vulkan inverse -----");
        }
        else
        {
                LOG("----- Vulkan forward -----");
        }

        {
                std::mt19937_64 engine = create_engine<std::mt19937_64>();
                std::uniform_int_distribution<int> uid(1, 3000);
                dft->create_buffers(uid(engine), uid(engine));
                dft->create_buffers(1, 1);
                dft->create_buffers(uid(engine), uid(engine));
                dft->create_buffers(1, uid(engine));
                dft->create_buffers(uid(engine), uid(engine));
                dft->create_buffers(uid(engine), 1);
                dft->create_buffers(uid(engine), uid(engine));
        }

        TimePoint start_time = time();

        dft->create_buffers(n1, n2);
        dft->exec(inverse, data);

        LOG(time_string("Vulkan", start_time));
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

        TimePoint start_time = time();

        std::unique_ptr<ns::dft::DFT> cufft = ns::dft::create_cufft(n1, n2);
        cufft->exec(inverse, data);

        LOG(time_string("cuFFT", start_time));
}
#endif

#if defined(FFTW_FOUND)
void compute_fftw(bool inverse, int n1, int n2, std::vector<Complex>* data)
{
        if (inverse)
        {
                LOG("----- FFTW inverse -----");
        }
        else
        {
                LOG("----- FFTW forward -----");
        }

        TimePoint start_time = time();

        std::unique_ptr<ns::dft::DFT> fftw = ns::dft::create_fftw(n1, n2);
        fftw->exec(inverse, data);

        LOG(time_string("FFTW", start_time));
}
#endif

std::filesystem::path make_path(const std::string_view& name)
{
        return std::filesystem::temp_directory_path() / path_from_utf8(name);
}

struct DftData
{
        std::vector<Complex> forward;
        std::vector<Complex> inverse;
};

DftData run_vulkan(
        const std::string& test_name,
        ComputeVector* dft,
        const int n1,
        const int n2,
        const std::vector<Complex>& source_data,
        ProgressRatio* progress,
        int* computation,
        int computation_count)
{
        DftData dft_data;

        std::vector<Complex> data(source_data);

        compute_vulkan(dft, false, n1, n2, &data);
        log_data(data);
        save_data(make_path(test_name + "_dft_output_forward_vulkan.txt"), data);
        dft_data.forward = data;

        progress->set(++(*computation), computation_count);

        compute_vulkan(dft, true, n1, n2, &data);
        log_data(data);
        save_data(make_path(test_name + "_dft_output_inverse_vulkan.txt"), data);
        dft_data.inverse = data;

        progress->set(++(*computation), computation_count);

        return dft_data;
}

#if defined(CUDA_FOUND)
void run_cuda(
        const std::string& test_name,
        const int n1,
        const int n2,
        const std::vector<complex>& source_data,
        ProgressRatio* progress,
        int* computation,
        int computation_count,
        const DftData& vulkan_data)
{
        std::vector<complex> data(source_data);

        try
        {
                compute_cuda(false, n1, n2, &data);
        }
        catch (const std::exception& e)
        {
                LOG(std::string("CUDA forward DFT error\n") + e.what());
                return;
        }
        catch (...)
        {
                LOG("CUDA forward DFT unknown error");
                return;
        }

        log_data(data);
        save_data(make_path(test_name + "_dft_output_forward_cuda.txt"), data);
        compare("Vulkan", "cuFFT", vulkan_data.forward, data);

        progress->set(++(*computation), computation_count);

        try
        {
                compute_cuda(true, n1, n2, &data);
        }
        catch (const std::exception& e)
        {
                LOG(std::string("CUDA inverse DFT error\n") + e.what());
                return;
        }
        catch (...)
        {
                LOG("CUDA inverse DFT unknown error");
                return;
        }

        log_data(data);
        save_data(make_path(test_name + "_dft_output_inverse_cuda.txt"), data);
        compare("Vulkan", "cuFFT", vulkan_data.inverse, data);

        progress->set(++(*computation), computation_count);
}
#endif

#if defined(FFTW_FOUND)
void run_fftw(
        const std::string& test_name,
        const int n1,
        const int n2,
        const std::vector<Complex>& source_data,
        ProgressRatio* progress,
        int* computation,
        int computation_count,
        const DftData& vulkan_data)
{
        std::vector<Complex> data(source_data);

        try
        {
                compute_fftw(false, n1, n2, &data);
        }
        catch (const std::exception& e)
        {
                LOG(std::string("FFTW forward DFT error\n") + e.what());
                return;
        }
        catch (...)
        {
                LOG("FFTW forward DFT unknown error");
                return;
        }

        log_data(data);
        save_data(make_path(test_name + "_dft_output_forward_fftw.txt"), data);
        compare("Vulkan", "FFTW", vulkan_data.forward, data);

        progress->set(++(*computation), computation_count);

        try
        {
                compute_fftw(true, n1, n2, &data);
        }
        catch (const std::exception& e)
        {
                LOG(std::string("FFTW inverse DFT error\n") + e.what());
                return;
        }
        catch (...)
        {
                LOG("FFTW inverse DFT unknown error");
                return;
        }

        log_data(data);
        save_data(make_path(test_name + "_dft_output_inverse_fftw.txt"), data);
        compare("Vulkan", "FFTW", vulkan_data.inverse, data);

        progress->set(++(*computation), computation_count);
}
#endif

void dft_test(
        const std::string& test_name,
        ComputeVector* dft,
        const int n1,
        const int n2,
        const std::vector<Complex>& source_data,
        ProgressRatio* progress)
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

        const DftData vulkan_data =
                run_vulkan(test_name, dft, n1, n2, source_data, progress, &computation, computation_count);

#if defined(CUDA_FOUND)
        run_cuda(test_name, n1, n2, source_data, progress, &computation, computation_count, vulkan_data);
#endif

#if defined(FFTW_FOUND)
        run_fftw(test_name, n1, n2, source_data, progress, &computation, computation_count, vulkan_data);
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
        const std::vector<Complex> source_data({{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}});
        const int n = source_data.size() / 3;
        const int k = source_data.size() / n;

        log_data(source_data);

        dft_test("constant", dft, n, k, source_data, progress);

        LOG("---\nDFT check passed");
}

void random_data_test(ComputeVector* dft, const std::array<int, 2>& dimensions, ProgressRatio* progress)
{
        LOG("\n----- Random Data DFT Tests -----");

        const std::filesystem::path input_file_name = make_path("dft_input.txt");

        generate_random_data<Complex::value_type>(input_file_name, dimensions[0], dimensions[1]);

        int n1;
        int n2;
        std::vector<Complex> source_data;
        load_data(input_file_name, &n1, &n2, &source_data);

        if (dimensions[0] != n1 || dimensions[1] != n2)
        {
                error("Error test data dimensions: saved to file (" + to_string(dimensions[0]) + ", "
                      + to_string(dimensions[1]) + "), loaded from file (" + to_string(n1) + ", " + to_string(n2)
                      + ")");
        }

        dft_test("random", dft, n1, n2, source_data, progress);

        LOG("---\nDFT check passed");
}

enum class TestSize
{
        Small,
        Big
};

TestSize find_test_size()
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();
        std::bernoulli_distribution small_test(0.9);
        return small_test(engine) ? TestSize::Small : TestSize::Big;
}

std::array<int, 2> find_dimensions(TestSize test_size)
{
        switch (test_size)
        {
        case TestSize::Small:
        {
                std::mt19937_64 engine = create_engine<std::mt19937_64>();
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

void test(ProgressRatio* progress)
{
        ASSERT(progress);

        std::unique_ptr<ComputeVector> dft = create_compute_vector();

        constant_data_test(dft.get(), progress);

        const TestSize test_size = find_test_size();
        const std::array<int, 2> dimensions = find_dimensions(test_size);
        random_data_test(dft.get(), dimensions, progress);
}

TEST_SMALL("DFT, 2-Space", test)
}
}
