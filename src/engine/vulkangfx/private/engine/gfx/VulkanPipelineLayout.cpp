#include "engine/gfx/VulkanPipelineLayout.hpp"
#include "engine/gfx/VulkanBuffer.hpp"
#include "engine/gfx/VulkanTexture.hpp"
#include "engine/gfx/VulkanTextureView.hpp"

namespace cb::gfx
{

static constexpr uint32_t max_descriptor_sets_per_pool = 32;
static constexpr uint32_t default_descriptor_count_per_type = 32;

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& in_device, 
	VkPipelineLayout in_pipeline_layout,
	const std::vector<VkDescriptorSetLayout>& in_set_layouts) : device(in_device), pipeline_layout(in_pipeline_layout),
	set_layouts(in_set_layouts)
{
	descriptor_pool_sizes =
	{
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, default_descriptor_count_per_type },
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, default_descriptor_count_per_type },
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER, default_descriptor_count_per_type },
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, default_descriptor_count_per_type },
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, default_descriptor_count_per_type },
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, default_descriptor_count_per_type },
	};
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
	for(const auto& set_layout : set_layouts)
		vkDestroyDescriptorSetLayout(device.get_device(), set_layout, nullptr);

	vkDestroyPipelineLayout(device.get_device(), pipeline_layout, nullptr);	
}

BackendDeviceResource VulkanPipelineLayout::allocate_set(const uint32_t in_set, const std::span<Descriptor, max_bindings>& in_descriptors)
{
	uint64_t hash = 0;
	for(const auto& descriptor : in_descriptors)
		hash_combine(hash, descriptor);

	auto it = descriptor_sets[in_set].find(hash);
	if(it != descriptor_sets[in_set].end())
	{
		it->second.lifetime = 0;
		return reinterpret_cast<BackendDeviceResource>(it->second.set);
	}

	VkDescriptorSet handle = VK_NULL_HANDLE;

	auto& queue = free_descriptor_sets[in_set];
	if(queue.empty())
	{
		allocate_pool();

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = nullptr;
		alloc_info.descriptorPool = descriptor_pools.back().pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &set_layouts[in_set];

		VkResult result = vkAllocateDescriptorSets(device.get_device(),
			&alloc_info,
			&handle);
		CB_CHECK(result == VK_SUCCESS);
	}
	else
	{
		handle = queue.front();
		queue.pop();
	}

	CB_ASSERT(handle != VK_NULL_HANDLE);

	std::vector<VkWriteDescriptorSet> writes;
	writes.reserve(in_descriptors.size());
	vkUpdateDescriptorSets(device.get_device(),
		static_cast<uint32_t>(writes.size()),
		writes.data(),
		0,
		nullptr);

	for(const auto& descriptor : in_descriptors)
	{
		if(descriptor.info.index() == Descriptor::None)
			continue;

		std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> info;

		if(descriptor.info.index() == Descriptor::BufferInfo)
		{
			DescriptorBufferInfo buffer = std::get<DescriptorBufferInfo>(descriptor.info);
			info = VkDescriptorBufferInfo { get_resource<VulkanBuffer>(buffer.handle)->get_buffer(), buffer.offset, buffer.range };
		}
		else
		{
			DescriptorTextureInfo texture = std::get<DescriptorTextureInfo>(descriptor.info);
			info = VkDescriptorImageInfo {
				VK_NULL_HANDLE,
				get_resource<VulkanTextureView>(texture.texture_view)->get_image_view(),
				convert_texture_layout(texture.layout) };
		}

		writes.emplace_back(
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			handle,
			descriptor.binding,
			0,
			1,
			convert_descriptor_type(descriptor.type),
			descriptor.info.index() == Descriptor::TextureInfo ? &std::get<VkDescriptorImageInfo>(info) : nullptr,
			descriptor.info.index() == Descriptor::BufferInfo ? &std::get<VkDescriptorBufferInfo>(info) : nullptr,
			nullptr);

		vkUpdateDescriptorSets(device.get_device(),
			static_cast<uint32_t>(writes.size()),
			writes.data(),
			0,
			nullptr);
	}

	descriptor_sets[in_set].insert({ hash, SetEntry(device, handle )});

	return reinterpret_cast<BackendDeviceResource>(handle);
}

void VulkanPipelineLayout::allocate_pool()
{
	VkDescriptorPoolCreateInfo create_info = {};
	create_info.pNext = nullptr;
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.flags = 0;
	create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
	create_info.pPoolSizes = descriptor_pool_sizes.data();
	create_info.maxSets = max_descriptor_sets_per_pool;

	VkDescriptorPool pool;
	vkCreateDescriptorPool(device.get_device(),
		&create_info,
		nullptr,
		&pool);
	descriptor_pools.emplace_back(device, pool);
}

}