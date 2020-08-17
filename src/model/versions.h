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

#include <deque>
#include <optional>
#include <unordered_set>

template <typename Update, Update ALL>
class Versions
{
        static constexpr size_t MAX_VERSION_COUNT = 10;

        struct Version
        {
                int version;
                std::unordered_set<Update> updates;
                Version(int version, std::unordered_set<Update>&& updates)
                        : version(version), updates(std::move(updates))
                {
                }
        };
        std::deque<Version> m_versions{{Version(0, {ALL})}};

public:
        void add(std::unordered_set<Update>&& updates)
        {
                while (m_versions.size() > MAX_VERSION_COUNT)
                {
                        m_versions.pop_front();
                }
                m_versions.emplace_back(m_versions.back().version + 1, std::move(updates));
        }

        void updates(std::optional<int>* version, std::unordered_set<Update>* updates) const
        {
                ASSERT(!m_versions.empty());

                if (!version->has_value())
                {
                        *version = m_versions.back().version;
                        *updates = {ALL};
                        return;
                }

                updates->clear();

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
                        updates->insert(iter->updates.cbegin(), iter->updates.cend());
                }

                *version = m_versions.back().version;
        }
};
