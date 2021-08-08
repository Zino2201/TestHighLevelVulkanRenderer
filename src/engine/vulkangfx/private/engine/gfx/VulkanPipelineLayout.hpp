#pragma once

#include "Vulkan.hpp"
#include "engine/gfx/PipelineLayout.hpp"
#include "engine/gfx/VulkanDevice.hpp"
#include <queue>
#include <robin_hood.h>

namespace cb::gfx
{

class VulkanDevice;

class VulkanPipelineLayout final
{
	struct PoolEntry
	{
		VulkanDevice& device;
		uint32_t allocations;
		VkDescriptorPool pool;

		PoolEntry(VulkanDevice& in_device, VkDescriptorPool in_pool) : device(in_device),
			allocations(0), pool(in_pool) {}
		~PoolEntry()
		{
			if(pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(device.get_device(), pool, nullptr);
		}

		PoolEntry(PoolEntry&& in_pool) : device(in_pool.device),
			allocations(std::move(in_pool.allocations)),
			pool(std::exchange(in_pool.pool, VK_NULL_HANDLE)) {}
	};

	struct SetEntry
	{
		static constexpr uint8_t max_frame_lifetime = 10;

		VulkanDevice& device;
		VkDescriptorSet set;
		uint8_t lifetime;

		SetEntry(VulkanDevice& in_device, const VkDescriptorSet& in_set) : device(in_device),
			set(in_set), lifetime(0) {}
	};
public:
	VulkanPipelineLayout(VulkanDevice& in_device, 
		VkPipelineLayout in_pipeline_layout,
		const std::vector<VkDescriptorSetLayout>& in_set_layouts);
	VulkanPipelineLayout(VulkanPipelineLayout&& in_other) noexcept = delete;
	
	~VulkanPipelineLayout();

	/**
	 * Allocate (or get) a descriptor set suitable for the specified descriptor est
	 */
	BackendDeviceResource allocate_set(const uint32_t in_set, const std::span<Descriptor, max_bindings>& in_descriptors);

	[[nodiscard]] VulkanDevice& get_device() const { return device; }
	[[nodiscard]] VkPipelineLayout get_pipeline_layout() const { return pipeline_layout; }
private:
	void allocate_pool();
private:
	VulkanDevice& device;
	VkPipelineLayout pipeline_layout;
	std::vector<VkDescriptorSetLayout> set_layouts;
	std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;

	/** Descriptor set management */
	std::array<robin_hood::unordered_map<uint64_t, SetEntry>, max_descriptor_sets> descriptor_sets;
	std::array<std::queue<VkDescriptorSet>, max_descriptor_sets> free_descriptor_sets;
	std::vector<PoolEntry> descriptor_pools;

};

inline VkDescriptorType convert_descriptor_type(const DescriptorType& in_type)
{
	switch(in_type)
	{
	case DescriptorType::UniformBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case DescriptorType::InputAttachment:
		return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	case DescriptorType::Sampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	case DescriptorType::StorageTexture:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescriptorType::SampledTexture:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	}
}

}