/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "compute.h"
#include "image.h"

#include "../../functionality.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/create.h>
#include <src/vulkan/device_compute.h>
#include <src/vulkan/instance.h>

namespace ns::gpu::renderer::test
{
namespace
{
constexpr unsigned IMAGE_SIZE = 100;

std::array<float, 3> pixel(const image::Image<2>& image, unsigned x, unsigned y)
{
        ASSERT(image.color_format == image::ColorFormat::R32G32B32);
        ASSERT(static_cast<int>(x) < image.size[0] && static_cast<int>(y) < image.size[1]);

        std::array<float, 3> rgb;
        const std::size_t offset = (static_cast<std::size_t>(y) * image.size[0] + x) * 3 * sizeof(float);
        std::memcpy(rgb.data(), image.pixels.data() + offset, 3 * sizeof(float));
        return rgb;
}

void test_pixel(const image::Image<2>& image, unsigned x, unsigned y, const std::array<float, 3>& rgb)
{
        const std::array<float, 3> p = pixel(image, x, y);
        if (p == rgb)
        {
                return;
        }
        error("pixel error: " + to_string(p) + " is not equal to " + to_string(rgb));
}

void check_ray_tracing_1(const image::Image<2>& image)
{
        test_pixel(image, 48, 48, {0.854999959, 0.0300000217, 0.115000017});
        test_pixel(image, 98, 48, {0.1, 0.1, 0.1});
}

void check_ray_query_1(const image::Image<2>& image)
{
        test_pixel(image, 48, 48, {0, 1, 0});
        test_pixel(image, 98, 48, {1, 0, 0});
}

void check_ray_tracing_2(const image::Image<2>& image)
{
        test_pixel(image, 48, 48, {0.754999995, 0.0300000217, 0.214999989});
        test_pixel(image, 98, 48, {0.0150000202, 0.0300000217, 0.954999924});
}

void check_ray_query_2(const image::Image<2>& image)
{
        test_pixel(image, 48, 48, {0, 1, 0});
        test_pixel(image, 98, 48, {0, 1, 0});
}

std::vector<vulkan::BottomLevelAccelerationStructure> create_bottom_level(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices)
{
        constexpr std::array VERTICES_0 = std::to_array<Vector3f>({{-0.5, 1, 0}, {-1, 0, 0}, {0, 0, 0}, {-0.5, -1, 0}});
        constexpr std::array INDICES_0 = std::to_array<std::uint32_t>({0, 1, 2, 1, 2, 3});

        constexpr std::array VERTICES_1 = std::to_array<Vector3f>({{0.5, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0.5, -1, 0}});
        constexpr std::array INDICES_1 = std::to_array<std::uint32_t>({0, 1, 2, 1, 2, 3});

        std::vector<vulkan::BottomLevelAccelerationStructure> bottom_level;

        bottom_level.push_back(create_bottom_level_acceleration_structure(
                device, compute_command_pool, compute_queue, family_indices, VERTICES_0, INDICES_0, std::nullopt));

        bottom_level.push_back(create_bottom_level_acceleration_structure(
                device, compute_command_pool, compute_queue, family_indices, VERTICES_1, INDICES_1, std::nullopt));

        return bottom_level;
}

std::vector<VkTransformMatrixKHR> create_matrices()
{
        constexpr VkTransformMatrixKHR MATRIX_0{{{1, 0, 0, 0.1}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
        constexpr VkTransformMatrixKHR MATRIX_1{{{1, 0, 0, -0.1}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

        std::vector<VkTransformMatrixKHR> matrices;
        matrices.push_back(MATRIX_0);
        matrices.push_back(MATRIX_1);
        return matrices;
}

vulkan::TopLevelAccelerationStructure create_top_level(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<vulkan::BottomLevelAccelerationStructure>& bottom_level,
        const std::vector<VkTransformMatrixKHR>& matrices)
{
        std::vector<std::uint64_t> references;
        references.reserve(bottom_level.size());
        for (const vulkan::BottomLevelAccelerationStructure& v : bottom_level)
        {
                references.push_back(v.device_address());
        }

        return create_top_level_acceleration_structure(
                device, compute_command_pool, compute_queue, family_indices, references, matrices);
}

void test_ray_tracing()
{
        const vulkan::DeviceCompute device_compute(
                vulkan::PhysicalDeviceSearchType::RANDOM, vulkan::Instance::handle(),
                device_ray_tracing_functionality());

        const vulkan::Device& device = device_compute.device();

        if (!ray_tracing_supported(device))
        {
                return;
        }

        const vulkan::Queue& queue = device_compute.compute_queue();

        const vulkan::CommandPool command_pool =
                vulkan::create_command_pool(device, device_compute.compute_family_index());

        const RayTracingImage ray_tracing_image(IMAGE_SIZE, IMAGE_SIZE, device, &command_pool, &queue);

        const std::vector<std::uint32_t> family_indices{command_pool.family_index()};

        const std::vector<vulkan::BottomLevelAccelerationStructure> bottom_level =
                create_bottom_level(device, command_pool, queue, family_indices);

        std::vector<VkTransformMatrixKHR> matrices = create_matrices();

        const vulkan::TopLevelAccelerationStructure top_level =
                create_top_level(device, command_pool, queue, family_indices, bottom_level, matrices);

        {
                const image::Image<2> image =
                        ray_tracing(device, command_pool, queue, ray_tracing_image, top_level.handle(), "ray_tracing");
                check_ray_tracing_1(image);
        }
        {
                const image::Image<2> image =
                        ray_query(device, command_pool, queue, ray_tracing_image, top_level.handle(), "ray_query");
                check_ray_query_1(image);
        }

        for (VkTransformMatrixKHR& m : matrices)
        {
                m.matrix[0][3] += 0.1;
        }

        top_level.update_matrices(device, command_pool, queue, matrices);

        {
                const image::Image<2> image = ray_tracing(
                        device, command_pool, queue, ray_tracing_image, top_level.handle(), "ray_tracing_update");
                check_ray_tracing_2(image);
        }
        {
                const image::Image<2> image = ray_query(
                        device, command_pool, queue, ray_tracing_image, top_level.handle(), "ray_query_update");
                check_ray_query_2(image);
        }
}

void test()
{
        LOG("Test Vulkan ray tracing");
        test_ray_tracing();
        LOG("Test Vulkan ray tracing passed");
}
}

TEST_SMALL("Vulkan ray tracing", test)
}
