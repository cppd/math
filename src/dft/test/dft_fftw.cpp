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

#if defined(FFTW_FOUND)

#include "dft_fftw.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/thread.h"
#include "com/time.h"

#include <fftw3.h>
#include <thread>

namespace
{
class FFTPlan final
{
        fftwf_plan p;

public:
        FFTPlan(bool inv, int n1, int n2, std::vector<fftwf_complex>* i, std::vector<fftwf_complex>* o)
        {
                fftwf_plan_with_nthreads(get_hardware_concurrency());

                if (inv)
                {
                        p = fftwf_plan_dft_2d(n2, n1, i->data(), o->data(), FFTW_BACKWARD, FFTW_MEASURE);
                }
                else
                {
                        p = fftwf_plan_dft_2d(n2, n1, i->data(), o->data(), FFTW_FORWARD, FFTW_MEASURE);
                }
        }
        ~FFTPlan()
        {
                fftwf_destroy_plan(p);
        }
        void execute()
        {
                fftwf_execute(p);
        }

        FFTPlan(const FFTPlan&) = delete;
        FFTPlan& operator=(const FFTPlan&) = delete;
        FFTPlan(FFTPlan&&) = delete;
        FFTPlan& operator=(FFTPlan&&) = delete;
};

class FFTPlanThreads final
{
public:
        FFTPlanThreads()
        {
                fftwf_init_threads();
        }
        ~FFTPlanThreads()
        {
                fftwf_cleanup_threads();
        }
};

class DFT final : public IFourierFFTW
{
        FFTPlanThreads m_threads;
        int m_n1, m_n2;
        std::vector<fftwf_complex> m_src, m_res;
        FFTPlan m_forward, m_backward;

public:
        DFT(int n1, int n2)
                : m_n1(n1),
                  m_n2(n2),
                  m_src(n1 * n2),
                  m_res(n1 * n2),
                  m_forward(false, n1, n2, &m_src, &m_res),
                  m_backward(true, n1, n2, &m_src, &m_res)
        {
        }
        ~DFT() override
        {
        }

        void exec(bool inv, std::vector<std::complex<float>>* data) override
        {
                if (data->size() != m_src.size())
                {
                        error("Error size FFTW");
                }

                for (size_t i = 0; i < m_res.size(); ++i)
                {
                        m_src[i][0] = (*data)[i].real();
                        m_src[i][1] = (*data)[i].imag();
                }

                double start_time = get_time_seconds();

                if (inv)
                {
                        m_backward.execute();

                        float k = 1.0f / (m_n1 * m_n2);
                        for (size_t i = 0; i < m_res.size(); ++i)
                        {
                                (*data)[i] = std::complex<float>(m_res[i][0], m_res[i][1]) * k;
                        }
                }
                else
                {
                        m_forward.execute();

                        for (size_t i = 0; i < m_res.size(); ++i)
                        {
                                (*data)[i] = std::complex<float>(m_res[i][0], m_res[i][1]);
                        }
                }

                LOG("calc FFTW: " + to_string_fixed(1000.0 * (get_time_seconds() - start_time), 5) + " ms");
        }
};
}

std::unique_ptr<IFourierFFTW> create_dft_fftw(int x, int y)
{
        return std::make_unique<DFT>(x, y);
}
#endif
