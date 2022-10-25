#include "vk_descriptors.h"

VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags) {
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(poolSizes.sizes.size());

	for (auto sz : poolSizes.sizes) {
		sizes.push_back({ sz.first, uint32_t(sz.second * count) });
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.maxSets = count;
	poolInfo.poolSizeCount = (uint32_t)sizes.size();
	poolInfo.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

	return descriptorPool;
}

void DescriptorAllocator::init(VkDevice device)
{
	m_Device = device;
}

void DescriptorAllocator::cleanup()
{
	for (auto p : m_FreePools) 
	{
		vkDestroyDescriptorPool(m_Device, p, nullptr);
	}
	for (auto p : m_UsedPools)
	{
		vkDestroyDescriptorPool(m_Device, p, nullptr);
	}
}
