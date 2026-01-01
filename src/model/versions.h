/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <cstddef>
#include <deque>
#include <optional>

namespace ns::model
{
template <std::size_t N>
class Versions final
{
        static constexpr std::size_t MAX_VERSION_COUNT = 10;

        struct Version final
        {
                int version;
                std::bitset<N> updates;

                Version(int version, const std::bitset<N>& updates)
                        : version(version),
                          updates(updates)
                {
                }
        };

        std::deque<Version> versions_{{Version(0, std::bitset<N>().set())}};

public:
        void add(const std::bitset<N>& updates)
        {
                while (versions_.size() > MAX_VERSION_COUNT)
                {
                        versions_.pop_front();
                }
                versions_.emplace_back(versions_.back().version + 1, updates);
        }

        std::bitset<N> updates(std::optional<int>* const version) const
        {
                ASSERT(version);
                ASSERT(!versions_.empty());

                if (!version->has_value())
                {
                        *version = versions_.back().version;
                        return std::bitset<N>().set();
                }

                ASSERT(version->value() <= versions_.back().version);
                if (version->value() == versions_.back().version)
                {
                        return std::bitset<N>();
                }

                std::bitset<N> updates;

                const int version_from = version->value() + 1;
                auto iter = versions_.cbegin();
                while (iter != versions_.cend() && iter->version < version_from)
                {
                        ++iter;
                }
                if (iter == versions_.cend())
                {
                        error("Version not found");
                }
                for (; iter != versions_.cend(); ++iter)
                {
                        updates |= iter->updates;
                }

                *version = versions_.back().version;
                return updates;
        }
};
}
