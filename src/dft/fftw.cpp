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

#if defined(FFTW_FOUND)

#include "fftw.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/com/time.h>

#include <algorithm>
#include <atomic>
#include <fftw3.h>

namespace
{
std::atomic_int global_fftw_counter = 0;

class FFTPlanThreads final
{
public:
        FFTPlanThreads()
        {
                if (++global_fftw_counter != 1)
                {
                        --global_fftw_counter;
                        error("Error FFTW init threads");
                }
                fftwf_init_threads();
        }

        ~FFTPlanThreads()
        {
                fftwf_cleanup_threads();
                --global_fftw_counter;
        }

        FFTPlanThreads(const FFTPlanThreads&) = delete;
        FFTPlanThreads& operator=(const FFTPlanThreads&) = delete;
        FFTPlanThreads(FFTPlanThreads&&) = delete;
        FFTPlanThreads& operator=(FFTPlanThreads&&) = delete;
};

class FFTPlan final
{
        fftwf_plan p;

public:
        FFTPlan(bool inverse, int n1, int n2, std::vector<std::complex<float>>* i, std::vector<std::complex<float>>* o)
        {
                ASSERT(1ull * n1 * n2 == i->size());
                ASSERT(i->size() == o->size());

                fftwf_plan_with_nthreads(hardware_concurrency());

                // По документации FFTW тип fftwf_complex является массивом float[2]
                // из действительной части в элементе 0 и мнимой части в элементе 1.
                // Это совпадает с требованиями к комплексным числам 29.5 Complex numbers
                // в стандарте C++17.
                fftwf_complex* in = reinterpret_cast<fftwf_complex*>(i->data());
                fftwf_complex* out = reinterpret_cast<fftwf_complex*>(o->data());

                if (inverse)
                {
                        p = fftwf_plan_dft_2d(n2, n1, in, out, FFTW_BACKWARD, FFTW_MEASURE);
                }
                else
                {
                        p = fftwf_plan_dft_2d(n2, n1, in, out, FFTW_FORWARD, FFTW_MEASURE);
                }
        }

        ~FFTPlan()
        {
                fftwf_destroy_plan(p);
        }

        void execute() const
        {
                fftwf_execute(p);
        }

        FFTPlan(const FFTPlan&) = delete;
        FFTPlan& operator=(const FFTPlan&) = delete;
        FFTPlan(FFTPlan&&) = delete;
        FFTPlan& operator=(FFTPlan&&) = delete;
};

class FFTW final : public DFT
{
        FFTPlanThreads m_threads;
        std::vector<std::complex<float>> m_in, m_out;
        FFTPlan m_forward, m_backward;
        const float m_inv_k;

        void exec(bool inverse, std::vector<std::complex<float>>* data) override
        {
                if (data->size() != m_in.size())
                {
                        error("Error size FFTW");
                }

                std::copy(data->cbegin(), data->cend(), m_in.begin());

                double start_time = time_in_seconds();

                if (inverse)
                {
                        m_backward.execute();

                        std::transform(
                                m_out.cbegin(), m_out.cend(), data->begin(),
                                [k = m_inv_k](const std::complex<float>& v) { return v * k; });
                }
                else
                {
                        m_forward.execute();

                        std::copy(m_out.cbegin(), m_out.cend(), data->begin());
                }

                LOG("calc FFTW: " + to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms");
        }

public:
        FFTW(int n1, int n2)
                : m_in(1ull * n1 * n2),
                  m_out(1ull * n1 * n2),
                  m_forward(false, n1, n2, &m_in, &m_out),
                  m_backward(true, n1, n2, &m_in, &m_out),
                  m_inv_k(1.0f / (1ull * n1 * n2))
        {
        }
};
}

std::unique_ptr<DFT> create_fftw(int x, int y)
{
        return std::make_unique<FFTW>(x, y);
}

#endif
