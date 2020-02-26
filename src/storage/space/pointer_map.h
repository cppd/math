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

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

template <typename Index, typename T>
class PointerMap
{
        mutable std::shared_mutex m_mutex;

        std::unordered_map<Index, std::shared_ptr<T>> m_map;

public:
        // Деструкторы могут работать долго, поэтому чтобы не было работы
        // деструкторов при блокировке, помещать во временный объект

        template <typename SetType>
        void set(const Index& id, SetType&& v)
        {
                static_assert(std::is_same_v<std::remove_cvref_t<SetType>, std::shared_ptr<T>>);

                std::shared_ptr<T> tmp;

                {
                        std::unique_lock lock(m_mutex);

                        auto iter = m_map.find(id);
                        if (iter != m_map.cend())
                        {
                                tmp = std::move(iter->second);
                                iter->second = std::forward<SetType>(v);
                        }
                        else
                        {
                                m_map.emplace(id, std::forward<SetType>(v));
                        }
                }
        }

        void reset(const Index& id)
        {
                std::shared_ptr<T> tmp;

                {
                        std::unique_lock lock(m_mutex);

                        auto iter = m_map.find(id);
                        if (iter != m_map.cend())
                        {
                                tmp = std::move(iter->second);
                        }
                }
        }

        void reset_all()
        {
                std::vector<std::shared_ptr<T>> tmp;

                {
                        std::unique_lock lock(m_mutex);

                        tmp.reserve(m_map.size());
                        for (auto& v : m_map)
                        {
                                tmp.push_back(std::move(v.second));
                        }
                }
        }

        std::shared_ptr<T> get(const Index& id) const
        {
                std::shared_lock lock(m_mutex);

                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second : nullptr;
        }
};
