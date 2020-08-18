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

#pragma once

#include <src/com/error.h>

#include <bitset>
#include <deque>
#include <optional>

template <size_t N>
class Versions
{
        static constexpr size_t MAX_VERSION_COUNT = 10;

        struct Version
        {
                int version;
                std::bitset<N> updates;
                Version(int version, const std::bitset<N>& updates) : version(version), updates(updates)
                {
                }
        };
        std::deque<Version> m_versions{{Version(0, std::bitset<N>().set())}};

public:
        void add(const std::bitset<N>& updates)
        {
                while (m_versions.size() > MAX_VERSION_COUNT)
                {
                        m_versions.pop_front();
                }
                m_versions.emplace_back(m_versions.back().version + 1, updates);
        }

        void updates(std::optional<int>* version, std::bitset<N>* updates) const
        {
                ASSERT(!m_versions.empty());

                if (!version->has_value())
                {
                        *version = m_versions.back().version;
                        updates->set();
                        return;
                }

                updates->reset();

                ASSERT(version->value() <= m_versions.back().version);
                if (version->value() == m_versions.back().version)
                {
                        return;
                }

                int version_from = version->value() + 1;
                auto iter = m_versions.cbegin();
                while (iter != m_versions.cend() && iter->version < version_from)
                {
                        ++iter;
                }
                if (iter == m_versions.cend())
                {
                        error("Version not found");
                }
                for (; iter != m_versions.cend(); ++iter)
                {
                        *updates |= iter->updates;
                }

                *version = m_versions.back().version;
        }
};
