/*
Copyright (C) 2017, 2018 Topological Manifold

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

// Clang 5 не работает с std::variant из библиотеки libstdc++

#pragma once

#if defined(__clang__)

#include "com/error.h"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <variant>

template <typename A, typename... T>
class SimpleVariant
{
        static constexpr int EMPTY = -1;

        std::aligned_storage_t<std::max({sizeof(A), sizeof(T)...}), std::max({alignof(A), alignof(T)...})> m_data;

        int m_id = EMPTY;

        template <typename V>
        static constexpr int compute_id()
        {
                static_assert(std::is_same_v<V, A> || (std::is_same_v<V, T> || ...));
                static_assert((std::is_same_v<V, A> ? 1 : 0) + ((std::is_same_v<V, T> ? 1 : 0) + ...) == 1);

                if (std::is_same_v<V, A>)
                {
                        return 0;
                }
                else
                {
                        int id = -1;
                        int i = 1;

                        ((!std::is_same_v<V, T> ? ++i : (static_cast<void>(id = i), false)) && ...);

                        return id;
                }
        }

        void constructor_copy(const SimpleVariant& v)
        {
                ASSERT(m_id == EMPTY);

                if (v.m_id == 0)
                {
                        new (&m_data) A(*reinterpret_cast<const A*>(&v.m_data));
                }
                else
                {
                        int i = 1;
                        ((i != v.m_id ? ++i : (new (&m_data) T(*reinterpret_cast<const T*>(&v.m_data)), false)) && ...);
                }

                m_id = v.m_id;
        }

        void constructor_move(SimpleVariant&& v)
        {
                ASSERT(m_id == EMPTY);

                if (v.m_id == 0)
                {
                        new (&m_data) A(std::move(*reinterpret_cast<A*>(&v.m_data)));
                }
                else
                {
                        int i = 1;
                        ((i != v.m_id ? ++i : (new (&m_data) T(std::move(*reinterpret_cast<T*>(&v.m_data))), false)) && ...);
                }

                m_id = v.m_id;
        }

        void assignment_copy(const SimpleVariant& v)
        {
                ASSERT(m_id == v.m_id);

                if (m_id == 0)
                {
                        *reinterpret_cast<A*>(&m_data) = *reinterpret_cast<const A*>(&v.m_data);
                }
                else
                {
                        int i = 1;
                        ((i != m_id ? ++i : (*reinterpret_cast<T*>(&m_data) = *reinterpret_cast<const T*>(&v.m_data), false)) &&
                         ...);
                }
        }

        void assignment_move(SimpleVariant&& v)
        {
                ASSERT(m_id == v.m_id);

                if (m_id == 0)
                {
                        *reinterpret_cast<A*>(&m_data) = std::move(*reinterpret_cast<A*>(&v.m_data));
                }
                else
                {
                        int i = 1;
                        ((i != m_id ? ++i :
                                      (*reinterpret_cast<T*>(&m_data) = std::move(*reinterpret_cast<T*>(&v.m_data)), false)) &&
                         ...);
                }
        }

        void destructor() noexcept
        {
                if (m_id == EMPTY)
                {
                        return;
                }

                if (m_id == 0)
                {
                        reinterpret_cast<const A*>(&m_data)->~A();
                }
                else
                {
                        int i = 1;
                        ((i != m_id ? ++i : (reinterpret_cast<const T*>(&m_data)->~T(), false)) && ...);
                }

                m_id = EMPTY;
        }

        static void error_bad_variant_access()
        {
                error("bad SimpleVariant access");
        }

        template <typename Src, typename Dst>
        using SameConstSrcDst = std::conditional_t<std::is_const_v<Src>, const Dst, Dst>;

        template <typename F, typename Data, typename ID>
        static void visit_impl(const F& function, Data* data, ID id)
        {
                if (id == 0)
                {
                        function(*reinterpret_cast<SameConstSrcDst<Data, A>*>(data));
                }
                else
                {
                        int i = 1;
                        ((i != id ? ++i : (function(*reinterpret_cast<SameConstSrcDst<Data, T>*>(data)), false)) && ...) &&
                                (static_cast<void>(error_bad_variant_access()), false);
                }
        }

public:
        SimpleVariant()
        {
                new (&m_data) A();
                m_id = 0;
        }

        template <typename Type, typename... Args>
        SimpleVariant(std::in_place_type_t<Type>, Args&&... v)
        {
                constexpr int id = compute_id<Type>();

                static_assert(id >= 0 && id < sizeof...(T) + 1);

                new (&m_data) Type(std::forward<Args>(v)...);
                m_id = id;
        }

        template <typename Type>
        SimpleVariant(Type&& v)
        {
                constexpr int id = compute_id<Type>();

                static_assert(id >= 0 && id < sizeof...(T) + 1);

                new (&m_data) Type(std::forward<Type>(v));
                m_id = id;
        }

        ~SimpleVariant()
        {
                destructor();
        }

        SimpleVariant(const SimpleVariant& v)
        {
                constructor_copy(v);
        }

        SimpleVariant(SimpleVariant&& v)
        {
                constructor_move(std::move(v));
        }

        SimpleVariant& operator=(const SimpleVariant& v)
        {
                if (this != &v)
                {
                        if (m_id == v.m_id)
                        {
                                assignment_copy(v);
                        }
                        else
                        {
                                destructor();
                                constructor_copy(v);
                        }
                }
                return *this;
        }

        SimpleVariant& operator=(SimpleVariant&& v)
        {
                if (this != &v)
                {
                        if (m_id == v.m_id)
                        {
                                assignment_move(std::move(v));
                        }
                        else
                        {
                                destructor();
                                constructor_move(std::move(v));
                        }
                }
                return *this;
        }

        template <typename V>
        const V& get() const
        {
                constexpr int id = compute_id<V>();

                static_assert(id >= 0 && id < sizeof...(T) + 1);

                if (m_id != id)
                {
                        error_bad_variant_access();
                }

                return *reinterpret_cast<const V*>(&m_data);
        }

        template <typename F>
        void visit(const F& f) const
        {
                visit_impl(f, &m_data, m_id);
        }

        template <typename F>
        void visit(const F& f)
        {
                visit_impl(f, &m_data, m_id);
        }
};

template <typename Visitor, typename SimpleVariant>
void simple_visit(const Visitor& visitor, SimpleVariant&& simple_variant)
{
        simple_variant.visit(visitor);
}

#endif
