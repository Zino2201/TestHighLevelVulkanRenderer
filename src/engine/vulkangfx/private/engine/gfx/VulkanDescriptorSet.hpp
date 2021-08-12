#pragma once

#include "Vulkan.hpp"
#include "engine/gfx/PipelineLayout.hpp"
#include <queue>
#include <robin_hood.h>

namespace cb::gfx
{

class VulkanDevice;
class VulkanPipelineLayout;

/**
 * Class managing descriptor set of a single pipeline set layout
 * This is managed by the device and created on-demand with a pipeline layout
 * Descriptor sets are allocated and stored in a hashmap, and after 10 frames they are marked as unused and can be recycled
 */
class VulkanDescriptorSetAllocator
{
	struct Node
	{
		static constexpr uint8_t node_max_unused_lifetime = 10;

		VkDescriptorSet set;
		uint8_t frame;

		Node(VkDescriptorSet in_set) : set(in_set), frame(0) {}
	};

	using HashMapType = robin_hood::unordered_flat_map<uint64_t, Node>;
public:
	VulkanDescriptorSetAllocator(VulkanDevice& in_device,
		VulkanPipelineLayout& in_pipeline_layout,
		VkDescriptorSetLayout in_set_layout);
	~VulkanDescriptorSetAllocator();

	void new_frame();
	VkDescriptorSet allocate(const std::span<Descriptor, max_bindings>& in_descriptors);
private:
	void allocate_pool();
private:
	VulkanDevice& device;
	VulkanPipelineLayout& pipeline_layout;
	VkDescriptorSetLayout set_layout;
	std::vector<VkDescriptorPool> pools;
	std::queue<VkDescriptorSet> free_sets;
	std::vector<VkDescriptorPoolSize> pool_sizes;
	HashMapType hashmap;
};

}