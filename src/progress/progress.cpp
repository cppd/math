/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "progress.h"

#include <src/com/exception.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>

namespace ns
{
namespace
{
constexpr bool LOCK_FREE = true;

template <typename T>
class RelaxedAtomic
{
        std::atomic<T> counter_{0};

        static_assert(decltype(counter_)::is_always_lock_free == LOCK_FREE);

public:
        RelaxedAtomic& operator=(const T& v)
        {
                counter_.store(v, std::memory_order_relaxed);
                return *this;
        }

        operator T() const&
        {
                return counter_.load(std::memory_order_relaxed);
        }
        operator T() const&& = delete;

        void operator|=(const T& v)
        {
                counter_.fetch_or(v, std::memory_order_relaxed);
        }
};
}

class AtomicTerminate
{
        using DataType = std::uint_least8_t;

        static constexpr DataType TERMINATE_QUIETLY = 0b1;
        static constexpr DataType TERMINATE_WITH_MESSAGE = 0b10;

        RelaxedAtomic<DataType> terminate_;

public:
        void set_terminate_quietly()
        {
                terminate_ |= TERMINATE_QUIETLY;
        }

        void set_terminate_with_message()
        {
                terminate_ |= TERMINATE_WITH_MESSAGE;
        }

        void check_terminate() const
        {
                const DataType terminate = terminate_;

                if (terminate & TERMINATE_QUIETLY)
                {
                        throw TerminateQuietlyException();
                }

                if (terminate & TERMINATE_WITH_MESSAGE)
                {
                        throw TerminateWithMessageException();
                }
        }
};

class ProgressRatio::Impl final : public ProgressRatioControl
{
        using CounterType = std::uint_least64_t;

        static constexpr unsigned SHIFT = 32;
        static constexpr unsigned MAX = (1ull << SHIFT) - 1;

        RelaxedAtomic<CounterType> counter_;
        AtomicTerminate terminate_;

        std::string text_;
        mutable std::mutex text_mutex_;

        ProgressRatios* ratios_;

        const std::string permanent_text_;

public:
        Impl(ProgressRatios* const ratios, std::string permanent_text)
                : ratios_(ratios),
                  permanent_text_(std::move(permanent_text))
        {
                set_undefined();

                if (ratios_)
                {
                        ratios_->add_progress_ratio(this);
                }
        }

        ~Impl()
        {
                if (ratios_)
                {
                        ratios_->delete_progress_ratio(this);
                }
        }

        void set(const unsigned value, const unsigned maximum)
        {
                terminate_.check_terminate();

                counter_ = (static_cast<CounterType>(maximum & MAX) << SHIFT) | (value & MAX);
        }

        void set(double v)
        {
                v = std::clamp(v, 0.0, 1.0);
                set(std::lround(v * MAX), MAX);
        }

        void set_undefined()
        {
                set(0, 0);
        }

        void set_text(const std::string& text)
        {
                std::lock_guard lg(text_mutex_);
                text_ = text;
        }

        void terminate_quietly() override
        {
                terminate_.set_terminate_quietly();
        }

        void terminate_with_message() override
        {
                terminate_.set_terminate_with_message();
        }

        void get(unsigned* const value, unsigned* const maximum) const override
        {
                CounterType c = counter_;
                *value = c & MAX;
                *maximum = c >> SHIFT;
        }

        std::string text() const override
        {
                std::lock_guard lg(text_mutex_);

                if (permanent_text_.empty())
                {
                        return text_;
                }

                if (!text_.empty())
                {
                        return permanent_text_ + ". " + text_;
                }
                return permanent_text_;
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

ProgressRatio::ProgressRatio(ProgressRatios* const ratios, const std::string& permanent_text)
        : progress_(std::make_unique<Impl>(ratios, permanent_text))
{
}
ProgressRatio::~ProgressRatio() = default;
void ProgressRatio::set(const unsigned value, const unsigned maximum)
{
        progress_->set(value, maximum);
}
void ProgressRatio::set(const double v)
{
        progress_->set(v);
}
void ProgressRatio::set_undefined()
{
        progress_->set_undefined();
}
void ProgressRatio::set_text(const std::string& text)
{
        progress_->set_text(text);
}
bool ProgressRatio::lock_free()
{
        return LOCK_FREE;
}
}
