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

#include "progress.h"

#include "com/thread.h"

class ProgressRatio::Impl final : public IProgressRatioControl
{
        static constexpr unsigned HIGH_SHIFT = 32;
        static constexpr unsigned long long LOW_MASK = (1ull << HIGH_SHIFT) - 1;
        static constexpr unsigned MAX = 1ull << (HIGH_SHIFT - 2);

        // К этим переменным имеются частые обращения, поэтому атомарные без мьютексов
        std::atomic_ullong m_counter{0};
        std::atomic_bool m_terminate_request{false};

        // Строка меняется редко в потоках, читается с частотой таймера интерфейса
        // в потоке интерфейса. Работа со строкой с её защитой мьютексом.
        std::string m_text;
        mutable std::mutex m_text_mutex;

        IProgressRatioList* m_ratios;

        const std::string m_permanent_text;

public:
        static constexpr bool LOCK_FREE = std::atomic_ullong::is_always_lock_free && std::atomic_bool::is_always_lock_free;

        Impl(IProgressRatioList* list, const std::string& permanent_text) : m_ratios(list), m_permanent_text(permanent_text)
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
                if (m_terminate_request)
                {
                        throw TerminateRequestException();
                }

                unsigned long long high = static_cast<unsigned long long>(m) << HIGH_SHIFT;
                unsigned low = v;
                m_counter = high + low;
        }

        void set(double v)
        {
                set(v * MAX, MAX);
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

        void set_terminate() noexcept override
        {
                m_terminate_request = true;
        }

        void get(unsigned* v, unsigned* m) const override
        {
                unsigned long long c = m_counter;
                *v = c & LOW_MASK;
                *m = c >> HIGH_SHIFT;
        }

        std::string get_text() const override
        {
                std::lock_guard lg(m_text_mutex);

                if (m_permanent_text.size() == 0)
                {
                        return m_text;
                }

                if (m_text.size() != 0)
                {
                        return m_permanent_text + ". " + m_text;
                }
                else
                {
                        return m_permanent_text;
                }
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};

ProgressRatio::ProgressRatio(IProgressRatioList* list, const std::string& permanent_text)
        : m_progress(std::make_unique<Impl>(list, permanent_text))
{
}
ProgressRatio::~ProgressRatio()
{
}
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
