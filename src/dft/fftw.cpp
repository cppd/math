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

#if defined(FFTW_FOUND)

#include "fftw.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/com/time.h>

#include <algorithm>
#include <fftw3.h>
#include <mutex>

namespace ns::dft
{
namespace
{
class FFTPlanThreads final
{
        inline static std::mutex m_mutex;
        inline static int m_counter = 0;

public:
        FFTPlanThreads()
        {
                std::lock_guard lg(m_mutex);
                if (++m_counter == 1)
                {
                        fftwf_init_threads();
                }
        }

        ~FFTPlanThreads()
        {
                std::lock_guard lg(m_mutex);
                if (--m_counter == 0)
                {
                        fftwf_cleanup_threads();
                }
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
        FFTPlan(const bool inverse,
                const int n1,
                const int n2,
                std::vector<std::complex<float>>* const in,
                std::vector<std::complex<float>>* const out)
        {
                ASSERT(1ull * n1 * n2 == in->size());
                ASSERT(in->size() == out->size());

                fftwf_plan_with_nthreads(hardware_concurrency());

                static_assert(sizeof(fftwf_complex) == sizeof(std::complex<float>));
                static_assert(alignof(fftwf_complex) == alignof(std::complex<float>));
                fftwf_complex* in_fftw = reinterpret_cast<fftwf_complex*>(in->data());
                fftwf_complex* out_fftw = reinterpret_cast<fftwf_complex*>(out->data());

                if (inverse)
                {
                        p = fftwf_plan_dft_2d(n2, n1, in_fftw, out_fftw, FFTW_BACKWARD, FFTW_MEASURE);
                }
                else
                {
                        p = fftwf_plan_dft_2d(n2, n1, in_fftw, out_fftw, FFTW_FORWARD, FFTW_MEASURE);
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
        std::vector<std::complex<float>> m_in;
        std::vector<std::complex<float>> m_out;
        FFTPlan m_forward;
        FFTPlan m_backward;
        const float m_inv_k;

        void exec(const bool inverse, std::vector<std::complex<float>>* const data) override
        {
                if (data->size() != m_in.size())
                {
                        error("Error size FFTW");
                }

                const TimePoint start_time = time();

                std::copy(data->cbegin(), data->cend(), m_in.begin());

                if (inverse)
                {
                        m_backward.execute();

                        std::transform(
                                m_out.cbegin(), m_out.cend(), data->begin(),
                                [k = m_inv_k](const std::complex<float>& v)
                                {
                                        return v * k;
                                });
                }
                else
                {
                        m_forward.execute();

                        std::copy(m_out.cbegin(), m_out.cend(), data->begin());
                }

                LOG("calc FFTW: " + to_string_fixed(1000.0 * duration_from(start_time), 5) + " ms");
        }

public:
        FFTW(const int n1, const int n2)
                : m_in(1ull * n1 * n2),
                  m_out(1ull * n1 * n2),
                  m_forward(false, n1, n2, &m_in, &m_out),
                  m_backward(true, n1, n2, &m_in, &m_out),
                  m_inv_k(1.0f / (1ull * n1 * n2))
        {
        }
};
}

std::unique_ptr<DFT> create_fftw(const int x, const int y)
{
        return std::make_unique<FFTW>(x, y);
}
}

#endif
