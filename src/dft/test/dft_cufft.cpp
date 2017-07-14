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

#if defined(CUDA_FOUND)

#include "dft_cufft.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"

//#include <cuda.h>
#include <cuda_runtime.h>
#include <cufft.h>

namespace
{
void check_cuda_errors()
{
        if (cudaPeekAtLastError() != cudaSuccess)
        {
                error("Cuda Error: " + std::string(cudaGetErrorString(cudaGetLastError())));
        }
}

void cuda_select_device()
{
        int dev_count, max_proc_count = -1, dev_num = -1;

        check_cuda_errors();
        cudaGetDeviceCount(&dev_count);
        check_cuda_errors();

        for (int i = 0; i < dev_count; ++i)
        {
                cudaDeviceProp p;
                cudaGetDeviceProperties(&p, i);
                check_cuda_errors();
                if (p.multiProcessorCount > max_proc_count)
                {
                        max_proc_count = p.multiProcessorCount;
                        dev_num = i;
                }
        }

        cudaSetDevice(dev_num);
        check_cuda_errors();
        cudaDeviceReset();
        check_cuda_errors();
        cudaFree(nullptr);
        check_cuda_errors();
}

void cuda_device_sync()
{
        check_cuda_errors();
        if (cudaDeviceSynchronize() != cudaSuccess)
        {
                error("Cuda error: Failed to synchronize");
        }
}

template <typename T>
class CudaMemory final
{
        size_t m_size;
        T* d_mem;

public:
        CudaMemory(size_t s) : m_size(s)
        {
                if (m_size < 1)
                {
                        error("Cuda malloc size < 1");
                }

                check_cuda_errors();

                cudaError_t r = cudaMalloc(&d_mem, m_size * sizeof(T));
                if (r != cudaSuccess)
                {
                        error("Cuda malloc error " + std::to_string(m_size * sizeof(T)) + ": " + cudaGetErrorString(r));
                }
        }
        ~CudaMemory()
        {
                cudaFree(d_mem);
        }
        operator T*()
        {
                return d_mem;
        }
        operator const T*() const
        {
                return d_mem;
        }
        size_t size() const
        {
                return m_size;
        }

        CudaMemory(const CudaMemory&) = delete;
        CudaMemory& operator=(const CudaMemory&) = delete;
        CudaMemory(CudaMemory&&) = delete;
        CudaMemory& operator=(CudaMemory&&) = delete;
};

template <typename T>
void cuda_memory_copy(CudaMemory<T>& dst, const std::vector<T>& src)
{
        if (dst.size() != src.size())
        {
                error("Cuda copy size error " + std::to_string(dst.size()) + " " + std::to_string(src.size()));
        }

        check_cuda_errors();

        cudaError_t r = cudaMemcpy(dst, src.data(), dst.size() * sizeof(T), cudaMemcpyHostToDevice);
        if (r != cudaSuccess)
        {
                error("Cuda copy to device error: " + std::string(cudaGetErrorString(r)));
        }
}

template <typename T>
void cuda_memory_copy(std::vector<T>* dst, const CudaMemory<T>& src)
{
        if (dst->size() != src.size())
        {
                error("Cuda copy size error " + std::to_string(dst->size()) + " " + std::to_string(src.size()));
        }

        check_cuda_errors();

        cudaError_t r = cudaMemcpy(dst->data(), src, src.size() * sizeof(T), cudaMemcpyDeviceToHost);
        if (r != cudaSuccess)
        {
                error("Cuda copy from device error: " + std::string(cudaGetErrorString(r)));
        }
}

class CuFFT final : public IFourierCuda
{
        cufftHandle m_plan;
        CudaMemory<cufftComplex> m_data;

        void exec(bool inv, std::vector<std::complex<float>>* src) override
        {
                if (src->size() != m_data.size())
                {
                        error("CuFFT input size error: input " + std::to_string(src->size()) + ", must be " +
                              std::to_string(m_data.size()));
                }

                std::vector<cufftComplex> cuda_x(src->size());

                for (size_t i = 0; i < src->size(); ++i)
                {
                        cuda_x[i].x = (*src)[i].real();
                        cuda_x[i].y = (*src)[i].imag();
                }

                cuda_memory_copy(m_data, cuda_x);

                cuda_device_sync();

                double start_time = get_time_seconds();

                if (inv)
                {
                        if (cufftExecC2C(m_plan, m_data, m_data, CUFFT_INVERSE) != CUFFT_SUCCESS)
                        {
                                error("CUFFT Error: Unable to execute plan");
                        }
                }
                else
                {
                        if (cufftExecC2C(m_plan, m_data, m_data, CUFFT_FORWARD) != CUFFT_SUCCESS)
                        {
                                error("CUFFT Error: Unable to execute plan");
                        }
                }

                cuda_device_sync();

                LOG("calc CUFFT: " + to_string_fixed(1000.0 * (get_time_seconds() - start_time), 5) + " ms");

                cuda_memory_copy(&cuda_x, m_data);

                if (inv)
                {
                        float k = 1.0f / m_data.size();

                        for (size_t i = 0; i < src->size(); ++i)
                        {
                                (*src)[i] = std::complex<float>(cuda_x[i].x, cuda_x[i].y) * k;
                        }
                }
                else
                {
                        for (size_t i = 0; i < src->size(); ++i)
                        {
                                (*src)[i] = std::complex<float>(cuda_x[i].x, cuda_x[i].y);
                        }
                }
        }

public:
        CuFFT(int x, int y) : m_data(x * y)
        {
                if (m_data.size() < 1)
                {
                        error("Error CuFFT sizes " + std::to_string(x) + "x" + std::to_string(y));
                }

                // Именно так y, x
                if (cufftPlan2d(&m_plan, y, x, CUFFT_C2C) != CUFFT_SUCCESS)
                {
                        error("CUFFT create FFT plan error");
                }
        }
        ~CuFFT() override
        {
                cufftDestroy(m_plan);
        }
        CuFFT(const CuFFT&) = delete;
        CuFFT& operator=(const CuFFT&) = delete;
        CuFFT(CuFFT&&) = delete;
        CuFFT& operator=(CuFFT&&) = delete;
};
}

std::unique_ptr<IFourierCuda> create_fft_cufft(int x, int y)
{
        cuda_select_device();
        return std::unique_ptr<CuFFT>(new CuFFT(x, y));
}

#endif
