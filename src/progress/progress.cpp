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

#include "progress.h"

#include <src/com/atomic_counter.h>
#include <src/com/thread.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>

namespace ns
{
class AtomicTerminate
{
        // Для одного обращения к атомарным данным вместо двух обращений к std::atomic_bool
        // используется хранение двух bool в одном std::atomic<int>.
        using DataType = int;

        static constexpr DataType TERMINATE_QUIETLY = 0b1;
        static constexpr DataType TERMINATE_WITH_MESSAGE = 0b10;

        std::atomic<DataType> m_terminate = 0;

public:
        static constexpr bool is_always_lock_free = std::atomic<DataType>::is_always_lock_free;

        void set_terminate_quietly()
        {
                m_terminate |= TERMINATE_QUIETLY;
        }

        void set_terminate_with_message()
        {
                m_terminate |= TERMINATE_WITH_MESSAGE;
        }

        void check_terminate() const
        {
                // Только одно обращение к атомарным данным и далее работа с копией этих данных
                DataType terminate = m_terminate;

                if (terminate & TERMINATE_QUIETLY)
                {
                        throw_terminate_quietly_exception();
                }

                if (terminate & TERMINATE_WITH_MESSAGE)
                {
                        throw_terminate_with_message_exception();
                }
        }
};

class ProgressRatio::Impl final : public ProgressRatioControl
{
        static constexpr unsigned SHIFT = 32;
        static constexpr unsigned MAX = (1u << 31) - 1;

        // К этим переменным имеются частые обращения, поэтому атомарные без мьютексов
        AtomicCounter<unsigned long long> m_counter{0};
        AtomicTerminate m_terminate;

        // Строка меняется редко в потоках, читается с частотой таймера интерфейса
        // в потоке интерфейса. Работа со строкой с её защитой мьютексом.
        std::string m_text;
        mutable std::mutex m_text_mutex;

        ProgressRatios* m_ratios;

        const std::string m_permanent_text;

public:
        static constexpr bool LOCK_FREE =
                AtomicCounter<unsigned long long>::is_always_lock_free && AtomicTerminate::is_always_lock_free;

        static_assert(LOCK_FREE);

        Impl(ProgressRatios* ratios, std::string permanent_text)
                : m_ratios(ratios), m_permanent_text(std::move(permanent_text))
        {
                set_undefined();

                if (m_ratios)
                {
                        m_ratios->add_progress_ratio(this);
                }
        }

        ~Impl() override
        {
                if (m_ratios)
                {
                        m_ratios->delete_progress_ratio(this);
                }
        }

        void set(unsigned v, unsigned m)
        {
                m_terminate.check_terminate();

                m_counter = (static_cast<unsigned long long>(m & MAX) << SHIFT) | (v & MAX);
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
                std::lock_guard lg(m_text_mutex);
                m_text = text;
        }

        void terminate_quietly() override
        {
                m_terminate.set_terminate_quietly();
        }

        void terminate_with_message() override
        {
                m_terminate.set_terminate_with_message();
        }

        void get(unsigned* v, unsigned* m) const override
        {
                unsigned long long c = m_counter;
                *v = c & MAX;
                *m = c >> SHIFT;
        }

        std::string text() const override
        {
                std::lock_guard lg(m_text_mutex);

                if (m_permanent_text.empty())
                {
                        return m_text;
                }

                if (!m_text.empty())
                {
                        return m_permanent_text + ". " + m_text;
                }
                return m_permanent_text;
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

ProgressRatio::ProgressRatio(ProgressRatios* ratios, const std::string& permanent_text)
        : m_progress(std::make_unique<Impl>(ratios, permanent_text))
{
}
ProgressRatio::~ProgressRatio() = default;
void ProgressRatio::set(unsigned v, unsigned m)
{
        m_progress->set(v, m);
}
void ProgressRatio::set(double v)
{
        m_progress->set(v);
}
void ProgressRatio::set_undefined()
{
        m_progress->set_undefined();
}
void ProgressRatio::set_text(const std::string& text)
{
        m_progress->set_text(text);
}
bool ProgressRatio::lock_free()
{
        return Impl::LOCK_FREE;
}
}
