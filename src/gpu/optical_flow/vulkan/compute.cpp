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

#include "../com/compute.h"

#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

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
void image_barrier(
        VkCommandBuffer command_buffer,
        const std::vector<VkImage>& images,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask)
{
        ASSERT(!images.empty());
        ASSERT(command_buffer != VK_NULL_HANDLE);
        ASSERT(std::all_of(images.cbegin(), images.cend(), [](VkImage image) { return image != VK_NULL_HANDLE; }));

        std::vector<VkImageMemoryBarrier> barriers(images.size());

        for (size_t i = 0; i < images.size(); ++i)
        {
                barriers[i] = {};

                barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barriers[i].oldLayout = old_layout;
                barriers[i].newLayout = new_layout;

                barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barriers[i].image = images[i];

                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barriers[i].subresourceRange.baseMipLevel = 0;
                barriers[i].subresourceRange.levelCount = 1;
                barriers[i].subresourceRange.baseArrayLayer = 0;
                barriers[i].subresourceRange.layerCount = 1;

                barriers[i].srcAccessMask = src_access_mask;
                barriers[i].dstAccessMask = dst_access_mask;
        }

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
}

void image_barrier(
        VkCommandBuffer command_buffer,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask)
{
        image_barrier(command_buffer, std::vector{image}, old_layout, new_layout, src_access_mask, dst_access_mask);
}

void buffer_barrier(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineStageFlags dst_stage_mask)
{
        ASSERT(buffer != VK_NULL_HANDLE);

        VkBufferMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst_stage_mask, VK_DEPENDENCY_BY_REGION_BIT, 0,
                nullptr, 1, &barrier, 0, nullptr);
}

class Impl final : public OpticalFlowCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        const vulkan::CommandPool& m_compute_command_pool;
        const vulkan::Queue& m_compute_queue;
        // const vulkan::CommandPool& m_transfer_command_pool;
        // const vulkan::Queue& m_transfer_queue;

        vulkan::Semaphore m_semaphore_first_pyramid;
        vulkan::Semaphore m_semaphore;

        std::optional<vulkan::CommandBuffer> m_command_buffer_first_pyramid;
        std::optional<vulkan::CommandBuffers> m_command_buffers;

        std::array<std::vector<vulkan::ImageWithMemory>, 2> m_images;
        std::vector<vulkan::ImageWithMemory> m_dx;
        std::vector<vulkan::ImageWithMemory> m_dy;
        std::vector<vulkan::BufferWithMemory> m_flow_buffers;

        OpticalFlowGrayscaleProgram m_grayscale_program;
        OpticalFlowGrayscaleMemory m_grayscale_memory;
        vec2i m_grayscale_groups;

        OpticalFlowDownsampleProgram m_downsample_program;
        std::vector<OpticalFlowDownsampleMemory> m_downsample_memory;
        std::vector<vec2i> m_downsample_groups;

        OpticalFlowSobelProgram m_sobel_program;
        std::vector<OpticalFlowSobelMemory> m_sobel_memory;
        std::vector<vec2i> m_sobel_groups;

        OpticalFlowFlowProgram m_flow_program;
        std::vector<OpticalFlowFlowMemory> m_flow_memory;
        std::vector<vec2i> m_flow_groups;

        int m_i_index = -1;

        std::vector<vulkan::ImageWithMemory> create_images(const std::vector<vec2i>& sizes, uint32_t family_index) const
        {
                std::vector<vulkan::ImageWithMemory> images;
                images.reserve(sizes.size());

                const bool storage = true;
                const std::unordered_set<uint32_t> family_indices(
                        {m_compute_command_pool.family_index(), family_index});
                const std::vector<VkFormat> formats({IMAGE_FORMAT});
                for (const vec2i& s : sizes)
                {
                        images.emplace_back(
                                m_device, m_compute_command_pool, m_compute_queue, family_indices, formats, s[0], s[1],
                                VK_IMAGE_LAYOUT_GENERAL, storage);
                }

                return images;
        }

        std::vector<vulkan::BufferWithMemory> create_flow_buffers(
                const std::vector<vec2i>& sizes,
                uint32_t family_index) const
        {
                std::vector<vulkan::BufferWithMemory> buffers;
                if (sizes.size() <= 1)
                {
                        return {};
                }
                buffers.reserve(sizes.size() - 1);

                const std::unordered_set<uint32_t> family_indices({family_index});
                for (size_t i = 1; i < sizes.size(); ++i)
                {
                        const int buffer_size = sizes[i][0] * sizes[i][1] * sizeof(vec2f);
                        buffers.emplace_back(
                                vulkan::BufferMemoryType::DeviceLocal, m_device, family_indices,
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);
                }

                return buffers;
        }

        static std::vector<OpticalFlowDownsampleMemory> create_downsample_memory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images)
        {
                ASSERT(images[0].size() == images[1].size());

                std::vector<OpticalFlowDownsampleMemory> downsample_images;

                for (unsigned i = 1; i < images[0].size(); ++i)
                {
                        downsample_images.emplace_back(device, descriptor_set_layout);
                        downsample_images.back().set_big(images[0][i - 1], images[1][i - 1]);
                        downsample_images.back().set_small(images[0][i], images[1][i]);
                }

                return downsample_images;
        }

        static std::vector<OpticalFlowSobelMemory> create_sobel_memory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy)
        {
                ASSERT(images[0].size() == images[1].size());
                ASSERT(images[0].size() == dx.size());
                ASSERT(images[0].size() == dy.size());

                std::vector<OpticalFlowSobelMemory> sobel_images;

                for (size_t i = 0; i < images[0].size(); ++i)
                {
                        sobel_images.emplace_back(device, descriptor_set_layout);
                        sobel_images.back().set_i(images[0][i], images[1][i]);
                        sobel_images.back().set_dx(dx[i]);
                        sobel_images.back().set_dy(dy[i]);
                }

                return sobel_images;
        }

        static std::vector<OpticalFlowFlowMemory> create_flow_memory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                uint32_t family_index,
                VkSampler sampler,
                const std::vector<vec2i>& sizes,
                const std::vector<vulkan::BufferWithMemory>& flow_buffers,
                int top_point_count_x,
                int top_point_count_y,
                const vulkan::BufferWithMemory& top_points,
                const vulkan::BufferWithMemory& top_flow,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy)
        {
                const size_t size = sizes.size();

                if (size <= 1)
                {
                        return {};
                }

                ASSERT(images[0].size() == size);
                ASSERT(images[1].size() == size);
                ASSERT(dx.size() == size);
                ASSERT(dy.size() == size);

                ASSERT(flow_buffers.size() + 1 == size);
                const auto flow_index = [&](size_t i) {
                        ASSERT(i > 0 && i < size);
                        return i - 1; // буферы начинаются с уровня 1
                };

                const std::unordered_set<uint32_t> family_indices{family_index};

                std::vector<OpticalFlowFlowMemory> flow_memory;

                for (size_t i = 0; i < size; ++i)
                {
                        const vulkan::BufferWithMemory* top_points_ptr;
                        const vulkan::BufferWithMemory* flow_ptr;
                        const vulkan::BufferWithMemory* flow_guess_ptr;

                        OpticalFlowFlowMemory::Data data;

                        const bool top = (i == 0);
                        const bool bottom = (i + 1 == size);

                        if (!top)
                        {
                                // Не самый верхний уровень, поэтому расчёт для всех точек
                                top_points_ptr = &top_points; // не используется
                                flow_ptr = &flow_buffers[flow_index(i)];
                                data.use_all_points = true;
                                data.point_count_x = sizes[i][0];
                                data.point_count_y = sizes[i][1];
                        }
                        else
                        {
                                // Самый верхний уровень, поэтому расчёт только для заданных
                                // точек для рисования на экране
                                top_points_ptr = &top_points;
                                flow_ptr = &top_flow;
                                data.use_all_points = false;
                                data.point_count_x = top_point_count_x;
                                data.point_count_y = top_point_count_y;
                        }

                        if (!bottom)
                        {
                                // Не самый нижний уровень, поэтому в качестве приближения
                                // использовать поток, полученный на меньших изображениях
                                int i_prev = i + 1;
                                data.use_guess = true;
                                data.guess_kx = (sizes[i_prev][0] != sizes[i][0]) ? 2 : 1;
                                data.guess_ky = (sizes[i_prev][1] != sizes[i][1]) ? 2 : 1;
                                data.guess_width = sizes[i_prev][0];
                                flow_guess_ptr = &flow_buffers[flow_index(i_prev)];
                        }
                        else
                        {
                                // Самый нижний уровень пирамиды, поэтому нет приближения
                                flow_guess_ptr = &flow_buffers[0]; // не используется
                                data.use_guess = false;
                        }

                        flow_memory.emplace_back(device, descriptor_set_layout, family_indices);

                        flow_memory[i].set_data(data);

                        flow_memory[i].set_top_points(*top_points_ptr);
                        flow_memory[i].set_flow(*flow_ptr);
                        flow_memory[i].set_flow_guess(*flow_guess_ptr);

                        flow_memory[i].set_dx(dx[i]);
                        flow_memory[i].set_dy(dy[i]);
                        flow_memory[i].set_i(images[0][i], images[1][i]);
                        flow_memory[i].set_j(sampler, images[1][i], images[0][i]);
                }

                return flow_memory;
        }

        void commands_compute_image_pyramid(int index, VkCommandBuffer command_buffer)
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_downsample_memory.size() == m_downsample_groups.size());
                ASSERT(m_downsample_memory.size() + 1 == m_images[index].size());

                // Уровень 0 заполняется по исходному изображению
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_grayscale_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_grayscale_program.pipeline_layout(),
                        OpticalFlowGrayscaleMemory::set_number(), 1, &m_grayscale_memory.descriptor_set(index), 0,
                        nullptr);
                vkCmdDispatch(command_buffer, m_grayscale_groups[0], m_grayscale_groups[1], 1);

                image_barrier(
                        command_buffer, m_images[index][0].image(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

                // Каждый следующий уровень меньше предыдущего
                for (unsigned i = 0; i < m_downsample_groups.size(); ++i)
                {
                        vkCmdBindPipeline(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_downsample_program.pipeline());
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_downsample_program.pipeline_layout(),
                                OpticalFlowDownsampleMemory::set_number(), 1,
                                &m_downsample_memory[i].descriptor_set(index), 0, nullptr);
                        vkCmdDispatch(command_buffer, m_downsample_groups[i][0], m_downsample_groups[i][1], 1);

                        image_barrier(
                                command_buffer, m_images[index][i + 1].image(), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void commands_compute_dxdy(int index, VkCommandBuffer command_buffer) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_sobel_memory.size() == m_sobel_groups.size());
                ASSERT(m_sobel_groups.size() == m_dx.size());
                ASSERT(m_sobel_groups.size() == m_dy.size());

                for (unsigned i = 0; i < m_sobel_groups.size(); ++i)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_sobel_program.pipeline());
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_sobel_program.pipeline_layout(),
                                OpticalFlowSobelMemory::set_number(), 1, &m_sobel_memory[i].descriptor_set(index), 0,
                                nullptr);
                        vkCmdDispatch(command_buffer, m_sobel_groups[i][0], m_sobel_groups[i][1], 1);
                }

                std::vector<VkImage> images;
                images.reserve(m_dx.size() + m_dy.size());
                for (unsigned i = 0; i < m_sobel_groups.size(); ++i)
                {
                        images.push_back(m_dx[i].image());
                        images.push_back(m_dy[i].image());
                }
                image_barrier(
                        command_buffer, images, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
        }

        void commands_compute_optical_flow(int index, VkCommandBuffer command_buffer, VkBuffer top_flow) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_flow_memory.size() == m_flow_groups.size());
                ASSERT(m_flow_buffers.size() + 1 == m_flow_groups.size());

                for (int i = static_cast<int>(m_flow_groups.size()) - 1; i >= 0; --i)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_flow_program.pipeline());
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_flow_program.pipeline_layout(),
                                OpticalFlowFlowMemory::set_number(), 1, &m_flow_memory[i].descriptor_set(index), 0,
                                nullptr);
                        vkCmdDispatch(command_buffer, m_flow_groups[i][0], m_flow_groups[i][1], 1);

                        buffer_barrier(
                                command_buffer, (i != 0) ? m_flow_buffers[i - 1] : top_flow,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
                }
        }

        void commands_images_to_sampler_layout(int index, VkCommandBuffer command_buffer)
        {
                for (const vulkan::ImageWithMemory& image : m_images[index])
                {
                        image_barrier(
                                command_buffer, image.image(), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void commands_images_to_general_layout(int index, VkCommandBuffer command_buffer)
        {
                for (const vulkan::ImageWithMemory& image : m_images[index])
                {
                        image_barrier(
                                command_buffer, image.image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void create_command_buffer_first_pyramid()
        {
                VkResult result;

                m_command_buffer_first_pyramid = vulkan::CommandBuffer(m_device, m_compute_command_pool);

                VkCommandBuffer command_buffer = *m_command_buffer_first_pyramid;

                VkCommandBufferBeginInfo command_buffer_info = {};
                command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                //

                commands_compute_image_pyramid(0, command_buffer);

                //

                result = vkEndCommandBuffer(command_buffer);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }

        void create_command_buffers(VkBuffer top_flow)
        {
                VkResult result;

                m_command_buffers = vulkan::CommandBuffers(m_device, m_compute_command_pool, 2);

                for (int index = 0; index < 2; ++index)
                {
                        VkCommandBuffer command_buffer = (*m_command_buffers)[index];

                        VkCommandBufferBeginInfo command_buffer_info = {};
                        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                        }

                        //

                        // i — предыдущее изображение, 1-i — текущее изображение
                        commands_compute_image_pyramid(1 - index, command_buffer);
                        commands_compute_dxdy(index, command_buffer);

                        commands_images_to_sampler_layout(1 - index, command_buffer);
                        commands_compute_optical_flow(index, command_buffer, top_flow);
                        commands_images_to_general_layout(1 - index, command_buffer);

                        //

                        result = vkEndCommandBuffer(command_buffer);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                        }
                }
        }

        VkSemaphore compute(const vulkan::Queue& queue, VkSemaphore wait_semaphore) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(queue.family_index() == m_compute_command_pool.family_index());
                ASSERT(m_command_buffers && m_command_buffers->count() == 2);
                ASSERT(m_i_index == -1 || m_i_index == 0 || m_i_index == 1);

                if (m_i_index < 0)
                {
                        m_i_index = 0;

                        ASSERT(m_command_buffer_first_pyramid);
                        vulkan::queue_submit(
                                wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, *m_command_buffer_first_pyramid,
                                m_semaphore_first_pyramid, queue);
                        wait_semaphore = m_semaphore_first_pyramid;
                }
                else
                {
                        m_i_index = 1 - m_i_index;
                }

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*m_command_buffers)[m_i_index],
                        m_semaphore, queue);

                return m_semaphore;
        }

        void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const Region<2, int>& rectangle,
                unsigned top_point_count_x,
                unsigned top_point_count_y,
                const vulkan::BufferWithMemory& top_points,
                const vulkan::BufferWithMemory& top_flow) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(rectangle.is_positive());
                ASSERT(rectangle.x1() <= static_cast<int>(input.width()));
                ASSERT(rectangle.y1() <= static_cast<int>(input.height()));

                const std::vector<vec2i> sizes =
                        optical_flow_pyramid_sizes(input.width(), input.height(), OPTICAL_FLOW_BOTTOM_IMAGE_SIZE);

                const uint32_t family_index = m_compute_command_pool.family_index();

                m_images[0] = create_images(sizes, family_index);
                m_images[1] = create_images(sizes, family_index);
                m_dx = create_images(sizes, family_index);
                m_dy = create_images(sizes, family_index);
                m_flow_buffers = create_flow_buffers(sizes, family_index);

                constexpr vec2i GROUPS = OPTICAL_FLOW_GROUP_SIZE;
                constexpr int GROUPS_X = OPTICAL_FLOW_GROUP_SIZE[0];
                constexpr int GROUPS_Y = OPTICAL_FLOW_GROUP_SIZE[1];

                m_grayscale_groups = optical_flow_grayscale_groups(GROUPS, sizes);
                m_grayscale_program.create_pipeline(GROUPS_X, GROUPS_Y, rectangle);
                m_grayscale_memory.set_src(sampler, input);
                m_grayscale_memory.set_dst(m_images[0][0], m_images[1][0]);

                m_downsample_groups = optical_flow_downsample_groups(GROUPS, sizes);
                m_downsample_program.create_pipeline(GROUPS_X, GROUPS_Y);
                m_downsample_memory =
                        create_downsample_memory(m_device, m_downsample_program.descriptor_set_layout(), m_images);

                m_sobel_groups = optical_flow_sobel_groups(GROUPS, sizes);
                m_sobel_program.create_pipeline(GROUPS_X, GROUPS_Y);
                m_sobel_memory =
                        create_sobel_memory(m_device, m_sobel_program.descriptor_set_layout(), m_images, m_dx, m_dy);

                m_flow_groups = optical_flow_flow_groups(GROUPS, sizes, top_point_count_x, top_point_count_y);
                m_flow_program.create_pipeline(
                        GROUPS_X, GROUPS_Y, OPTICAL_FLOW_RADIUS, OPTICAL_FLOW_ITERATION_COUNT,
                        OPTICAL_FLOW_STOP_MOVE_SQUARE, OPTICAL_FLOW_MIN_DETERMINANT);
                m_flow_memory = create_flow_memory(
                        m_device, m_flow_program.descriptor_set_layout(), family_index, sampler, sizes, m_flow_buffers,
                        top_point_count_x, top_point_count_y, top_points, top_flow, m_images, m_dx, m_dy);

                create_command_buffer_first_pyramid();
                create_command_buffers(top_flow);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_command_buffer_first_pyramid.reset();
                m_command_buffers.reset();

                m_grayscale_program.delete_pipeline();
                m_downsample_program.delete_pipeline();
                m_sobel_program.delete_pipeline();
                m_flow_program.delete_pipeline();

                m_images[0].clear();
                m_images[1].clear();
                m_dx.clear();
                m_dy.clear();
                m_flow_buffers.clear();
                m_downsample_memory.clear();
                m_downsample_memory.clear();
                m_sobel_memory.clear();
                m_flow_memory.clear();
        }

        void reset() override
        {
                m_i_index = -1;
        }

public:
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& compute_command_pool,
             const vulkan::Queue& compute_queue,
             const vulkan::CommandPool& /*transfer_command_pool*/,
             const vulkan::Queue& /*transfer_queue*/)
                : m_instance(instance),
                  m_device(instance.device()),
                  m_compute_command_pool(compute_command_pool),
                  m_compute_queue(compute_queue),
                  // m_transfer_command_pool(transfer_command_pool),
                  // m_transfer_queue(transfer_queue),
                  m_semaphore_first_pyramid(instance.device()),
                  m_semaphore(instance.device()),
                  m_grayscale_program(instance.device()),
                  m_grayscale_memory(m_device, m_grayscale_program.descriptor_set_layout()),
                  m_downsample_program(instance.device()),
                  m_sobel_program(instance.device()),
                  m_flow_program(instance.device())
        {
                ASSERT(compute_command_pool.family_index() == compute_queue.family_index());
                // ASSERT(transfer_command_pool.family_index() == transfer_queue.family_index());
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

std::unique_ptr<OpticalFlowCompute> create_optical_flow_compute(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue)
{
        return std::make_unique<Impl>(
                instance, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue);
}
}
