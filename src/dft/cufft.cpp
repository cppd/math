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

#if defined(CUDA_FOUND)

#include "cufft.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>

//#include <cuda.h>
#include <cuda_runtime.h>
#include <cufft.h>
#include <type_traits>

namespace
{
void cuda_check_errors()
{
        if (cudaPeekAtLastError() != cudaSuccess)
        {
                error("CUDA Error: " + std::string(cudaGetErrorString(cudaGetLastError())));
        }
}

void cuda_select_device()
{
        int dev_count;
        int max_proc_count = -1;
        int dev_num = -1;

        cuda_check_errors();
        cudaGetDeviceCount(&dev_count);
        cuda_check_errors();

        for (int i = 0; i < dev_count; ++i)
        {
                cudaDeviceProp p;
                cudaGetDeviceProperties(&p, i);
                cuda_check_errors();
                if (p.multiProcessorCount > max_proc_count)
                {
                        max_proc_count = p.multiProcessorCount;
                        dev_num = i;
                }
        }

        cudaSetDevice(dev_num);
        cuda_check_errors();
        cudaDeviceReset();
        cuda_check_errors();
        cudaFree(nullptr);
        cuda_check_errors();
}

void cuda_device_sync()
{
        cuda_check_errors();
        if (cudaDeviceSynchronize() != cudaSuccess)
        {
                error("CUDA error: Failed to synchronize");
        }
}

class CudaPlan2D
{
        cufftHandle m_plan;

public:
        CudaPlan2D(int x, int y)
        {
                // Именно так y, x
                if (cufftPlan2d(&m_plan, y, x, CUFFT_C2C) != CUFFT_SUCCESS)
                {
                        error("cuFFT create FFT plan error");
                }
        }

        ~CudaPlan2D()
        {
                cufftDestroy(m_plan);
        }

        operator cufftHandle() const
        {
                return m_plan;
        }

        CudaPlan2D(const CudaPlan2D&) = delete;
        CudaPlan2D& operator=(const CudaPlan2D&) = delete;
        CudaPlan2D(CudaPlan2D&&) = delete;
        CudaPlan2D& operator=(CudaPlan2D&&) = delete;
};

template <typename T>
class CudaMemory final
{
        size_t m_size;
        T* d_mem;

public:
        explicit CudaMemory(size_t s) : m_size(s)
        {
                if (m_size < 1)
                {
                        error("CUDA malloc size < 1");
                }

                cuda_check_errors();

                cudaError_t r = cudaMalloc(&d_mem, m_size * sizeof(T));
                if (r != cudaSuccess)
                {
                        error("Error CUDA malloc " + to_string(m_size * sizeof(T)) +
                              " bytes: " + cudaGetErrorString(r));
                }
        }

        ~CudaMemory()
        {
                cudaFree(d_mem);
        }

        operator T*() const
        {
                return d_mem;
        }

        size_t size() const
        {
                return m_size;
        }

        size_t bytes() const
        {
                return m_size * sizeof(T);
        }

        CudaMemory(const CudaMemory&) = delete;
        CudaMemory& operator=(const CudaMemory&) = delete;
        CudaMemory(CudaMemory&&) = delete;
        CudaMemory& operator=(CudaMemory&&) = delete;
};

template <typename M, typename T>
void cuda_memory_copy(CudaMemory<M>& memory, const std::vector<T>& src)
{
        if (memory.bytes() != src.size() * sizeof(T))
        {
                error("CUDA copy size error " + to_string(memory.bytes()) + " <- " + to_string(src.size() * sizeof(T)));
        }

        cuda_check_errors();

        cudaError_t r = cudaMemcpy(memory, src.data(), memory.bytes(), cudaMemcpyHostToDevice);
        if (r != cudaSuccess)
        {
                error("CUDA copy to device error: " + std::string(cudaGetErrorString(r)));
        }
}

template <typename M, typename T>
void cuda_memory_copy(std::vector<T>* dst, const CudaMemory<M>& memory)
{
        if (memory.bytes() != dst->size() * sizeof(T))
        {
                error("CUDA copy size error " + to_string(dst->size() * sizeof(T)) + " <- " +
                      to_string(memory.bytes()));
        }

        cuda_check_errors();

        cudaError_t r = cudaMemcpy(dst->data(), memory, memory.bytes(), cudaMemcpyDeviceToHost);
        if (r != cudaSuccess)
        {
                error("CUDA copy from device error: " + std::string(cudaGetErrorString(r)));
        }
}

class CudaFFT final : public DFT
{
        static_assert(std::is_standard_layout_v<cufftComplex>);
        static_assert(sizeof(std::complex<float>) == sizeof(cufftComplex));
        static_assert(std::is_same_v<decltype(cufftComplex::x), float>);
        static_assert(std::is_same_v<decltype(cufftComplex::y), float>);
        static_assert(offsetof(cufftComplex, x) == 0);
        static_assert(offsetof(cufftComplex, y) == sizeof(float));

        CudaPlan2D m_plan;
        CudaMemory<cufftComplex> m_cuda_data;
        const float m_inv_k;

        void exec(bool inverse, std::vector<std::complex<float>>* data) override
        {
                if (data->size() != m_cuda_data.size())
                {
                        error("Error size cuFFT");
                }

                cuda_memory_copy(m_cuda_data, *data);

                cuda_device_sync();

                double start_time = time_in_seconds();

                if (inverse)
                {
                        if (CUFFT_SUCCESS != cufftExecC2C(m_plan, m_cuda_data, m_cuda_data, CUFFT_INVERSE))
                        {
                                error("cuFFT Error: Unable to execute inverse plan");
                        }
                }
                else
                {
                        if (CUFFT_SUCCESS != cufftExecC2C(m_plan, m_cuda_data, m_cuda_data, CUFFT_FORWARD))
                        {
                                error("cuFFT Error: Unable to execute forward plan");
                        }
                }

                cuda_device_sync();

                LOG("calc cuFFT: " + to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms");

                cuda_memory_copy(data, m_cuda_data);

                if (inverse)
                {
                        std::transform(
                                data->cbegin(), data->cend(), data->begin(),
                                [k = m_inv_k](const std::complex<float>& v) { return v * k; });
                }
        }

public:
        CudaFFT(int x, int y) : m_plan(x, y), m_cuda_data(1ull * x * y), m_inv_k(1.0f / (1ull * x * y))
        {
        }
};
}

std::unique_ptr<DFT> create_cufft(int x, int y)
{
        cuda_select_device();
        return std::make_unique<CudaFFT>(x, y);
}

#endif
