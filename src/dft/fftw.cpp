/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "dft.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>

#include <fftw3.h>

#include <algorithm>
#include <complex>
#include <memory>
#include <mutex>
#include <vector>

namespace ns::dft
{
namespace
{
class FFTPlanThreads final
{
        inline static std::mutex mutex_;
        inline static int counter_ = 0;

public:
        FFTPlanThreads()
        {
                const std::lock_guard lg(mutex_);
                if (++counter_ == 1)
                {
                        fftwf_init_threads();
                }
        }

        ~FFTPlanThreads()
        {
                const std::lock_guard lg(mutex_);
                if (--counter_ == 0)
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
        fftwf_plan plan_;

public:
        FFTPlan(const bool inverse,
                const unsigned n1,
                const unsigned n2,
                std::vector<std::complex<float>>* const in,
                std::vector<std::complex<float>>* const out)
        {
                ASSERT(static_cast<unsigned long long>(n1) * n2 == in->size());
                ASSERT(in->size() == out->size());

                fftwf_plan_with_nthreads(hardware_concurrency());

                static_assert(sizeof(fftwf_complex) == sizeof(std::complex<float>));
                static_assert(alignof(fftwf_complex) == alignof(std::complex<float>));
                auto* const in_fftw = reinterpret_cast<fftwf_complex*>(in->data());
                auto* const out_fftw = reinterpret_cast<fftwf_complex*>(out->data());

                if (inverse)
                {
                        plan_ = fftwf_plan_dft_2d(n2, n1, in_fftw, out_fftw, FFTW_BACKWARD, FFTW_MEASURE);
                }
                else
                {
                        plan_ = fftwf_plan_dft_2d(n2, n1, in_fftw, out_fftw, FFTW_FORWARD, FFTW_MEASURE);
                }
        }

        ~FFTPlan()
        {
                fftwf_destroy_plan(plan_);
        }

        void execute() const
        {
                fftwf_execute(plan_);
        }

        FFTPlan(const FFTPlan&) = delete;
        FFTPlan& operator=(const FFTPlan&) = delete;
        FFTPlan(FFTPlan&&) = delete;
        FFTPlan& operator=(FFTPlan&&) = delete;
};

class Impl final : public DFT
{
        FFTPlanThreads threads_;
        std::vector<std::complex<float>> in_;
        std::vector<std::complex<float>> out_;
        FFTPlan forward_;
        FFTPlan backward_;
        const float inv_k_;

        void exec(const bool inverse, std::vector<std::complex<float>>* const data) override
        {
                if (data->size() != in_.size())
                {
                        error("Error FFTW size");
                }

                const Clock::time_point start_time = Clock::now();

                std::ranges::copy(*data, in_.begin());

                if (inverse)
                {
                        backward_.execute();

                        std::ranges::transform(
                                out_, data->begin(),
                                [k = inv_k_](const std::complex<float>& v)
                                {
                                        return v * k;
                                });
                }
                else
                {
                        forward_.execute();

                        std::ranges::copy(out_, data->begin());
                }

                LOG("calc FFTW: " + to_string_fixed(1000.0 * duration_from(start_time), 5) + " ms");
        }

public:
        Impl(const unsigned long long n1, const unsigned long long n2)
                : in_(n1 * n2),
                  out_(n1 * n2),
                  forward_(false, n1, n2, &in_, &out_),
                  backward_(true, n1, n2, &in_, &out_),
                  inv_k_(1.0 / (n1 * n2))
        {
        }
};
}

std::unique_ptr<DFT> create_fftw(const int x, const int y)
{
        if (!(x > 0 && y > 0))
        {
                error("Error FFTW data size");
        }

        return std::make_unique<Impl>(x, y);
}
}

#endif
