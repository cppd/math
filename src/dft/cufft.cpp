/*
Copyright (C) 2017-2025 Topological Manifold

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

#ifdef CUDA_FOUND

#include "cufft.h"

#include <src/com/chrono.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

// #include <cuda.h>
#include <cuda_runtime.h>
#include <cufft.h>

#include <type_traits>

namespace ns::dft
{
namespace
{
void check_errors()
{
        if (cudaPeekAtLastError() != cudaSuccess)
        {
                error("CUDA Error: " + std::string(cudaGetErrorString(cudaGetLastError())));
        }
}

int device_count()
{
        check_errors();
        int res;
        cudaGetDeviceCount(&res);
        check_errors();
        return res;
}

void select_device()
{
        const int count = device_count();

        int max_processor_count = 0;
        int device_number = -1;

        for (int i = 0; i < count; ++i)
        {
                cudaDeviceProp p;
                cudaGetDeviceProperties(&p, i);
                check_errors();
                if (p.multiProcessorCount > max_processor_count)
                {
                        max_processor_count = p.multiProcessorCount;
                        device_number = i;
                }
        }

        if (max_processor_count <= 0)
        {
                error("Cuda device not found");
        }

        ASSERT(device_number >= 0);
        cudaSetDevice(device_number);
        check_errors();

        cudaDeviceReset();
        check_errors();

        cudaFree(nullptr);
        check_errors();
}

void device_sync()
{
        check_errors();
        if (cudaDeviceSynchronize() != cudaSuccess)
        {
                error("CUDA error: Failed to synchronize");
        }
}

class CudaPlan2D final
{
        cufftHandle plan_;

public:
        CudaPlan2D(const int x, const int y)
        {
                // y, x
                if (cufftPlan2d(&plan_, y, x, CUFFT_C2C) != CUFFT_SUCCESS)
                {
                        error("cuFFT create FFT plan error");
                }
        }

        ~CudaPlan2D()
        {
                cufftDestroy(plan_);
        }

        [[nodiscard]] cufftHandle handle() const
        {
                return plan_;
        }

        CudaPlan2D(const CudaPlan2D&) = delete;
        CudaPlan2D& operator=(const CudaPlan2D&) = delete;
        CudaPlan2D(CudaPlan2D&&) = delete;
        CudaPlan2D& operator=(CudaPlan2D&&) = delete;
};

template <typename T>
class CudaMemory final
{
        std::size_t size_;
        T* memory_;

public:
        explicit CudaMemory(const std::size_t size)
                : size_(size)
        {
                if (size_ < 1)
                {
                        error("CUDA malloc size < 1");
                }

                check_errors();

                const auto r = cudaMalloc(&memory_, size_ * sizeof(T));
                if (r != cudaSuccess)
                {
                        error("Error CUDA malloc " + to_string(size_ * sizeof(T)) + " bytes: " + cudaGetErrorString(r));
                }
        }

        ~CudaMemory()
        {
                cudaFree(memory_);
        }

        [[nodiscard]] T* data() const
        {
                return memory_;
        }

        [[nodiscard]] std::size_t size() const
        {
                return size_;
        }

        [[nodiscard]] std::size_t bytes() const
        {
                return size_ * sizeof(T);
        }

        CudaMemory(const CudaMemory&) = delete;
        CudaMemory& operator=(const CudaMemory&) = delete;
        CudaMemory(CudaMemory&&) = delete;
        CudaMemory& operator=(CudaMemory&&) = delete;
};

template <typename M, typename T>
void memory_copy(CudaMemory<M>& memory, const std::vector<T>& src)
{
        if (memory.bytes() != data_size(src))
        {
                error("CUDA copy size error " + to_string(memory.bytes()) + " <- " + to_string(data_size(src)));
        }

        check_errors();

        const auto r = cudaMemcpy(memory.data(), src.data(), memory.bytes(), cudaMemcpyHostToDevice);
        if (r != cudaSuccess)
        {
                error(std::string("CUDA copy to device error: ") + cudaGetErrorString(r));
        }
}

template <typename M, typename T>
void memory_copy(std::vector<T>* const dst, const CudaMemory<M>& memory)
{
        if (memory.bytes() != data_size(*dst))
        {
                error("CUDA copy size error " + to_string(data_size(*dst)) + " <- " + to_string(memory.bytes()));
        }

        check_errors();

        const auto r = cudaMemcpy(dst->data(), memory.data(), memory.bytes(), cudaMemcpyDeviceToHost);
        if (r != cudaSuccess)
        {
                error(std::string("CUDA copy from device error: ") + cudaGetErrorString(r));
        }
}

class Impl final : public DFT
{
        static_assert(std::is_standard_layout_v<cufftComplex>);
        static_assert(sizeof(std::complex<float>) == sizeof(cufftComplex));
        static_assert(std::is_same_v<decltype(cufftComplex::x), float>);
        static_assert(std::is_same_v<decltype(cufftComplex::y), float>);
        static_assert(offsetof(cufftComplex, x) == 0);
        static_assert(offsetof(cufftComplex, y) == sizeof(float));

        CudaPlan2D plan_;
        CudaMemory<cufftComplex> memory_;
        const float inv_k_;

        void exec(const bool inverse, std::vector<std::complex<float>>* const data) override
        {
                if (data->size() != memory_.size())
                {
                        error("Error cuFFT size");
                }

                memory_copy(memory_, *data);

                device_sync();

                const Clock::time_point start_time = Clock::now();

                if (inverse)
                {
                        const auto r = cufftExecC2C(plan_.handle(), memory_.data(), memory_.data(), CUFFT_INVERSE);
                        if (r != CUFFT_SUCCESS)
                        {
                                error("cuFFT Error: Unable to execute inverse plan");
                        }
                }
                else
                {
                        const auto r = cufftExecC2C(plan_.handle(), memory_.data(), memory_.data(), CUFFT_FORWARD);
                        if (r != CUFFT_SUCCESS)
                        {
                                error("cuFFT Error: Unable to execute forward plan");
                        }
                }

                device_sync();

                LOG("calc cuFFT: " + to_string_fixed(1000.0 * duration_from(start_time), 5) + " ms");

                memory_copy(data, memory_);

                if (inverse)
                {
                        std::transform(
                                data->cbegin(), data->cend(), data->begin(),
                                [k = inv_k_](const std::complex<float>& v)
                                {
                                        return v * k;
                                });
                }
        }

public:
        Impl(const unsigned long long x, const unsigned long long y)
                : plan_(x, y),
                  memory_(x * y),
                  inv_k_(1.0 / (x * y))
        {
        }
};
}

std::unique_ptr<DFT> create_cufft(const int x, const int y)
{
        if (!(x > 0 && y > 0))
        {
                error("Error cuFFT data size");
        }

        select_device();
        return std::make_unique<Impl>(x, y);
}
}

#endif
