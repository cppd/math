/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "progress_interfaces.h"

#include <src/com/exception.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace ns::progress
{
namespace
{
template <typename T>
class RelaxedAtomic final
{
        std::atomic<T> counter_{0};

        static_assert(std::is_integral_v<T>);
        static_assert(decltype(counter_)::is_always_lock_free);

public:
        void set(const T v)
        {
                counter_.store(v, std::memory_order_relaxed);
        }

        [[nodiscard]] T value() const
        {
                return counter_.load(std::memory_order_relaxed);
        }

        void operator|=(const T v)
        {
                counter_.fetch_or(v, std::memory_order_relaxed);
        }
};

class AtomicTerminate final
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
                const DataType terminate = terminate_.value();

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
}

class Ratio::Impl final : public RatioControl
{
        using CounterType = std::uint_least64_t;

        static constexpr unsigned SHIFT = 32;
        static constexpr unsigned MAX = (1ull << SHIFT) - 1;

        RelaxedAtomic<CounterType> counter_;
        AtomicTerminate terminate_;

        std::string text_;
        mutable std::mutex text_mutex_;

        Ratios* const ratios_;

        const std::string permanent_text_;

public:
        Impl(Ratios* const ratios, std::string&& permanent_text)
                : ratios_(ratios),
                  permanent_text_(std::move(permanent_text))
        {
                set_undefined();

                if (ratios_)
                {
                        ratios_->add_ratio(this);
                }
        }

        ~Impl()
        {
                if (ratios_)
                {
                        ratios_->delete_ratio(this);
                }
        }

        void set(const unsigned value, const unsigned maximum)
        {
                terminate_.check_terminate();

                counter_.set((static_cast<CounterType>(maximum & MAX) << SHIFT) | (value & MAX));
        }

        void set(const double v)
        {
                set(std::llround(std::clamp(v, 0.0, 1.0) * MAX), MAX);
        }

        void set_undefined()
        {
                set(0, 0);
        }

        void set_text(std::string&& text)
        {
                const std::lock_guard lg(text_mutex_);
                text_ = std::move(text);
        }

        void terminate_quietly() override
        {
                terminate_.set_terminate_quietly();
        }

        void terminate_with_message() override
        {
                terminate_.set_terminate_with_message();
        }

        RatioInfo info() const override
        {
                const CounterType c = counter_.value();
                const unsigned value = c & MAX;
                const unsigned maximum = c >> SHIFT;

                return {.value = value, .maximum = maximum, .text = text()};
        }

        std::string text() const
        {
                const std::lock_guard lg(text_mutex_);

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

Ratio::Ratio(Ratios* const ratios, std::string permanent_text)
        : progress_(std::make_unique<Impl>(ratios, std::move(permanent_text)))
{
}

Ratio::~Ratio() = default;

void Ratio::set(const unsigned value, const unsigned maximum)
{
        progress_->set(value, maximum);
}

void Ratio::set(const double v)
{
        progress_->set(v);
}

void Ratio::set_undefined()
{
        progress_->set_undefined();
}

void Ratio::set_text(std::string text)
{
        progress_->set_text(std::move(text));
}
}
