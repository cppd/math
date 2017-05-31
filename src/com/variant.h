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

#pragma once
#ifndef VARIANT_H
#define VARIANT_H

#include "com/error.h"

#include <algorithm>
#include <type_traits>

template <class T>
struct InPlaceT
{
        explicit InPlaceT() = default;
};
template <class T>
constexpr InPlaceT<T> in_place{};

template <typename... T>
class SimpleVariant
{
        static constexpr int ID_NO_DATA = -1;

        std::aligned_storage_t<std::max({sizeof(T)...}), std::max({alignof(T)...})> m_data;
        int m_id;

        template <typename V>
        static constexpr int compute_id()
        {
                static_assert((std::is_same<V, T>::value || ...));
                static_assert(((std::is_same<V, T>::value ? 1 : 0) + ...) == 1);

                int id = -1;
                int i = 0;

                ((!std::is_same<V, T>::value ? ++i : (static_cast<void>(id = i), false)) && ...);

                return id;
        }

public:
        SimpleVariant()
        {
                m_id = ID_NO_DATA;
        }

        template <typename V>
        SimpleVariant(V&& v)
        {
                constexpr int id = compute_id<V>();

                static_assert(id >= 0 && id < sizeof...(T));

                m_id = id;
                new (&m_data) V(std::forward<V>(v));
        }

        template <typename V, typename... Args>
        SimpleVariant(InPlaceT<V>, Args&&... v)
        {
                constexpr int id = compute_id<V>();

                static_assert(id >= 0 && id < sizeof...(T));

                m_id = id;
                new (&m_data) V(std::forward<Args>(v)...);
        }

        ~SimpleVariant()
        {
                if (m_id < 0)
                {
                        return;
                }

                int i = 0;
                bool found = ((i++ != m_id ? false : (reinterpret_cast<const T*>(&m_data)->~T(), true)) || ...);

                ASSERT(found);
        }
        SimpleVariant(const SimpleVariant& v)
        {
                m_id = v.m_id;

                if (m_id < 0)
                {
                        return;
                }

                int i = 0;
                bool found = ((i++ != m_id ? false : (new (&m_data) T(v.get<T>()), true)) || ...);

                ASSERT(found);
        }
        SimpleVariant(SimpleVariant&& v)
        {
                m_id = v.m_id;

                if (m_id < 0)
                {
                        return;
                }

                m_data = v.m_data;
                v.m_id = ID_NO_DATA;
        }

        SimpleVariant& operator=(const SimpleVariant& v) = delete;

        SimpleVariant& operator=(SimpleVariant&& v)
        {
                if (this != &v)
                {
                        this->~SimpleVariant();

                        m_id = v.m_id;

                        if (m_id >= 0)
                        {
                                m_data = v.m_data;
                                v.m_id = ID_NO_DATA;
                        }
                }
                return *this;
        }

        template <typename V>
        const V& get() const
        {
                constexpr int id = compute_id<V>();

                static_assert(id >= 0 && id < sizeof...(T));

                ASSERT(m_id == id);

                return *reinterpret_cast<const V*>(&m_data);
        }
};

#endif
