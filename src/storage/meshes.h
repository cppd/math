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

template <typename Index, typename Mesh>
class Meshes
{
        mutable std::shared_mutex m_mutex;

        std::unordered_map<Index, std::shared_ptr<Mesh>> m_meshes;

public:
        // Деструкторы могут работать долго, поэтому чтобы не было работы
        // деструкторов при блокировке, помещать во временный объект

        template <typename T>
        void set(const Index& id, T&& mesh)
        {
                static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<T>>, std::shared_ptr<Mesh>>);

                std::shared_ptr<Mesh> tmp;

                {
                        std::unique_lock lock(m_mutex);

                        auto iter = m_meshes.find(id);
                        if (iter != m_meshes.cend())
                        {
                                tmp = std::move(iter->second);
                                iter->second = std::forward<T>(mesh);
                        }
                        else
                        {
                                m_meshes.emplace(id, std::forward<T>(mesh));
                        }
                }
        }

        void reset(const Index& id)
        {
                std::shared_ptr<Mesh> tmp;

                {
                        std::unique_lock lock(m_mutex);

                        auto iter = m_meshes.find(id);
                        if (iter != m_meshes.cend())
                        {
                                tmp = std::move(iter->second);
                        }
                }
        }

        void reset_all()
        {
                std::vector<std::shared_ptr<Mesh>> tmp;

                {
                        std::unique_lock lock(m_mutex);

                        tmp.reserve(m_meshes.size());
                        for (auto& mesh : m_meshes)
                        {
                                tmp.push_back(std::move(mesh.second));
                        }
                }
        }

        std::shared_ptr<Mesh> get(const Index& id) const
        {
                std::shared_lock lock(m_mutex);

                auto iter = m_meshes.find(id);
                return (iter != m_meshes.cend()) ? iter->second : nullptr;
        }
};
