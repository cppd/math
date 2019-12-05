/*
Copyright (C) 2017-2019 Topological Manifold

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

/*
По книге

Aaftab Munshi, Benedict R. Gaster, Timothy G. Mattson, James Fung, Dan Ginsburg.
OpenCL Programming Guide.
Addison-Wesley, 2011.
Chapter 19. Optical Flow.

Дополнительная информация

Salil Kapur, Nisarg Thakkar.
Mastering OpenCV Android Application Programming.
Packt Publishing, 2015.
Chapter 5. Tracking Objects in Videos.
*/

#include "compute.h"

#include "compute_downsample.h"
#include "compute_flow.h"
#include "compute_grayscale.h"
#include "compute_sobel.h"

#include "gpu/optical_flow/com/compute.h"

#include <thread>

constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

namespace gpu_vulkan
{
namespace
{
class Impl final : public OpticalFlowCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        const vulkan::CommandPool& m_compute_command_pool;
        const vulkan::Queue& m_compute_queue;
        // const vulkan::CommandPool& m_transfer_command_pool;
        // const vulkan::Queue& m_transfer_queue;

        std::array<std::vector<vulkan::ImageWithMemory>, 2> m_images;
        std::vector<vulkan::ImageWithMemory> m_dx;
        std::vector<vulkan::ImageWithMemory> m_dy;
        std::vector<vulkan::BufferWithMemory> m_flow_buffers;

        OpticalFlowGrayscaleProgram m_grayscale_program;
        std::array<OpticalFlowGrayscaleMemory, 2> m_grayscale_memory;
        vec2i m_grayscale_groups;

        OpticalFlowDownsampleProgram m_downsample_program;
        std::array<std::vector<OpticalFlowDownsampleMemory>, 2> m_downsample_memory;
        std::vector<vec2i> m_downsample_groups;

        OpticalFlowSobelProgram m_sobel_program;
        std::vector<vec2i> m_sobel_groups;

        OpticalFlowFlowProgram m_flow_program;
        std::vector<vec2i> m_flow_groups;

        std::vector<vulkan::ImageWithMemory> create_images(const std::vector<vec2i>& sizes) const
        {
                std::vector<vulkan::ImageWithMemory> images;
                images.reserve(sizes.size());

                const bool storage = true;
                const std::unordered_set<uint32_t> family_indices({m_compute_command_pool.family_index()});
                const std::vector<VkFormat> formats({IMAGE_FORMAT});
                for (const vec2i& s : sizes)
                {
                        images.emplace_back(m_device, m_compute_command_pool, m_compute_queue, family_indices, formats, s[0],
                                            s[1], VK_IMAGE_LAYOUT_GENERAL, storage);
                }

                return images;
        }

        std::vector<vulkan::BufferWithMemory> create_flow_buffers(const std::vector<vec2i>& sizes) const
        {
                std::vector<vulkan::BufferWithMemory> buffers;
                if (sizes.size() <= 1)
                {
                        return {};
                }
                buffers.reserve(sizes.size() - 1);

                const std::unordered_set<uint32_t> family_indices({m_compute_command_pool.family_index()});
                for (const vec2i& s : sizes)
                {
                        const int buffer_size = s[0] * s[1] * sizeof(vec2f);
                        buffers.emplace_back(vulkan::BufferMemoryType::DeviceLocal, m_device, family_indices,
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);
                }

                return buffers;
        }

        static std::array<OpticalFlowGrayscaleMemory, 2> create_grayscale_memory(const vulkan::Device& device,
                                                                                 VkDescriptorSetLayout descriptor_set_layout)
        {
                return {OpticalFlowGrayscaleMemory(device, descriptor_set_layout),
                        OpticalFlowGrayscaleMemory(device, descriptor_set_layout)};
        }

        static std::array<std::vector<OpticalFlowDownsampleMemory>, 2> create_downsample_memory(
                const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images)
        {
                ASSERT(images[0].size() == images[1].size());

                std::array<std::vector<OpticalFlowDownsampleMemory>, 2> downsample_images;

                for (unsigned i = 1; i < images[0].size(); ++i)
                {
                        downsample_images[0].emplace_back(device, descriptor_set_layout);
                        downsample_images[1].emplace_back(device, descriptor_set_layout);
                        downsample_images[0].back().set(images[0][i - 1], images[0][i]);
                        downsample_images[1].back().set(images[1][i - 1], images[1][i]);
                }

                return downsample_images;
        }

        void compute_commands(VkCommandBuffer /*command_buffer*/) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void create_buffers(VkSampler sampler, const vulkan::ImageWithMemory& input, unsigned x, unsigned y, unsigned width,
                            unsigned height, unsigned top_point_count_x, unsigned top_point_count_y,
                            const vulkan::BufferWithMemory& /*top_points*/, const vulkan::BufferWithMemory& /*top_flow*/,
                            uint32_t /*family_index*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(width > 0 && height > 0);
                ASSERT(x + width <= input.width() && y + height <= input.height());

                const std::vector<vec2i> sizes =
                        optical_flow_pyramid_sizes(input.width(), input.height(), OPTICAL_FLOW_BOTTOM_IMAGE_SIZE);

                m_images[0] = create_images(sizes);
                m_images[1] = create_images(sizes);
                m_dx = create_images(sizes);
                m_dy = create_images(sizes);
                m_flow_buffers = create_flow_buffers(sizes);

                constexpr vec2i GROUPS = OPTICAL_FLOW_GROUP_SIZE;
                constexpr int GROUPS_X = OPTICAL_FLOW_GROUP_SIZE[0];
                constexpr int GROUPS_Y = OPTICAL_FLOW_GROUP_SIZE[1];

                m_grayscale_groups = optical_flow_grayscale_groups(GROUPS, sizes);
                m_grayscale_program.create_pipeline(GROUPS_X, GROUPS_Y, x, y, width, height);
                m_grayscale_memory[0].set_src(sampler, input);
                m_grayscale_memory[0].set_dst(m_images[0][0]);
                m_grayscale_memory[1].set_src(sampler, input);
                m_grayscale_memory[1].set_dst(m_images[1][0]);

                m_downsample_groups = optical_flow_downsample_groups(GROUPS, sizes);
                m_downsample_program.create_pipeline(GROUPS_X, GROUPS_Y);
                m_downsample_memory = create_downsample_memory(m_device, m_downsample_program.descriptor_set_layout(), m_images);

                m_sobel_groups = optical_flow_sobel_groups(GROUPS, sizes);
                m_sobel_program.create_pipeline(GROUPS_X, GROUPS_Y);

                m_flow_groups = optical_flow_flow_groups(GROUPS, sizes, top_point_count_x, top_point_count_y);
                m_flow_program.create_pipeline(GROUPS_X, GROUPS_Y, OPTICAL_FLOW_RADIUS, OPTICAL_FLOW_ITERATION_COUNT,
                                               OPTICAL_FLOW_STOP_MOVE_SQUARE, OPTICAL_FLOW_MIN_DETERMINANT);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_grayscale_program.delete_pipeline();
                m_downsample_program.delete_pipeline();
                m_sobel_program.delete_pipeline();
                m_flow_program.delete_pipeline();

                m_images[0].clear();
                m_images[1].clear();
                m_dx.clear();
                m_dy.clear();
                m_flow_buffers.clear();
                m_downsample_memory[0].clear();
                m_downsample_memory[1].clear();
        }

        void reset() override
        {
        }

public:
        Impl(const vulkan::VulkanInstance& instance, const vulkan::CommandPool& compute_command_pool,
             const vulkan::Queue& compute_queue, const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue)
                : m_instance(instance),
                  m_device(instance.device()),
                  m_compute_command_pool(compute_command_pool),
                  m_compute_queue(compute_queue),
                  // m_transfer_command_pool(transfer_command_pool),
                  // m_transfer_queue(transfer_queue),
                  m_grayscale_program(instance.device()),
                  m_grayscale_memory(create_grayscale_memory(m_device, m_grayscale_program.descriptor_set_layout())),
                  m_downsample_program(instance.device()),
                  m_sobel_program(instance.device()),
                  m_flow_program(instance.device())
        {
                ASSERT(compute_command_pool.family_index() == compute_queue.family_index());
                ASSERT(transfer_command_pool.family_index() == transfer_queue.family_index());
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan optical flow compute destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> OpticalFlowCompute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<OpticalFlowCompute> create_optical_flow_compute(const vulkan::VulkanInstance& instance,
                                                                const vulkan::CommandPool& compute_command_pool,
                                                                const vulkan::Queue& compute_queue,
                                                                const vulkan::CommandPool& transfer_command_pool,
                                                                const vulkan::Queue& transfer_queue)
{
        return std::make_unique<Impl>(instance, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue);
}
}
