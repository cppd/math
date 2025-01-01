/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

namespace ns
{
namespace
{
template <typename T>
class Interface
{
protected:
        ~Interface() = default;

public:
        [[nodiscard]] virtual T value() const = 0;
};

template <typename T>
class Impl final : public Interface<T>
{
        std::array<std::byte, 5> padding_1_;
        T data_;
        std::array<std::byte, 7> padding_2_;

        [[nodiscard]] T value() const override
        {
                return data_;
        }

public:
        explicit Impl(std::type_identity_t<T>&& v)
                : data_(std::move(v))
        {
        }
};

template <typename T, typename Distribution>
std::vector<const Interface<T>*> create_data(const std::size_t object_count, PCG engine, Distribution d)
{
        std::vector<const Interface<T>*> res;
        res.reserve(object_count);
        for (std::size_t i = 0; i < object_count; ++i)
        {
                res.push_back(make_arena_ptr<Impl<T>>(d(engine)));
        }
        return res;
}

template <typename T, typename Distribution>
void compare_data(const std::vector<const Interface<T>*>& pointers, PCG engine, Distribution d)
{
        if (pointers.empty())
        {
                error("No pointer data");
        }
        for (const Interface<T>* const pointer : pointers)
        {
                const T v = d(engine);
                const T p = pointer->value();
                if (!(p == v))
                {
                        error("Error value " + to_string(p) + ", expected " + to_string(v));
                }
        }
}

template <typename T>
constexpr std::size_t OBJECTS_IN_BLOCK = MemoryArena::block_size() / sizeof(Impl<T>);

template <typename T>
std::size_t compute_block_count(const std::size_t object_count)
{
        return (object_count + OBJECTS_IN_BLOCK<T> - 1) / OBJECTS_IN_BLOCK<T>;
}

template <typename T>
std::size_t compute_byte_count(const std::size_t object_count)
{
        const std::size_t block_count = compute_block_count<T>(object_count);
        return object_count * sizeof(Impl<T>)
               + (block_count - 1) * (MemoryArena::block_size() - OBJECTS_IN_BLOCK<T> * sizeof(Impl<T>));
}

template <typename T>
void check_arena(const std::size_t object_count)
{
        {
                const std::size_t expected_blocks = compute_block_count<T>(object_count);
                const std::size_t used_blocks = MemoryArena::thread_local_instance().used_blocks();

                if (expected_blocks != used_blocks)
                {
                        error("Expected block count " + to_string(expected_blocks)
                              + " is not equal to used block count " + to_string(used_blocks));
                }
        }
        {
                const std::size_t expected_bytes = compute_byte_count<T>(object_count);
                const std::size_t used_bytes = MemoryArena::thread_local_instance().used_bytes();

                if (expected_bytes != used_bytes)
                {
                        error("Expected byte count " + to_string(expected_bytes) + " is not equal to used byte count "
                              + to_string(used_bytes));
                }
        }
}

template <typename T, typename Distribution>
void test_arena(const std::size_t object_count, const Distribution d)
{
        MemoryArena::thread_local_instance().clear();

        const PCG engine;

        const std::vector<const Interface<T>*> pointers = create_data<T>(object_count, engine, d);
        check_arena<T>(object_count);
        compare_data<T>(pointers, engine, d);
}

template <typename T>
std::size_t random_object_count()
{
        PCG engine;
        std::uniform_int_distribution<int> uid(1, 5 * (MemoryArena::block_size() / sizeof(Impl<T>)));
        return uid(engine);
}

void test_int()
{
        const std::size_t object_count = random_object_count<int>();

        LOG("Test arena int, object count " + to_string(object_count) + ", block count "
            + to_string(compute_block_count<int>(object_count)));

        const std::uniform_int_distribution<int> uid(Limits<int>::lowest(), Limits<int>::max());

        for (int i = 0; i < 2; ++i)
        {
                test_arena<int>(object_count, uid);
        }
}

void test_float()
{
        const std::size_t object_count = random_object_count<long double>();

        LOG("Test arena long double, object count " + to_string(object_count) + ", block count "
            + to_string(compute_block_count<long double>(object_count)));

        const std::uniform_real_distribution<long double> urd(-1, 1);

        for (int i = 0; i < 2; ++i)
        {
                test_arena<long double>(object_count, urd);
        }
}

void test()
{
        test_int();
        test_float();

        LOG("Test arena passed");
}

TEST_SMALL("Memory Arena", test)
}
}
