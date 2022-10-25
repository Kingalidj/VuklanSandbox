#pragma once

#include "vk_types.h"

class DescriptorAllocator {
public:

	struct PoolSizes {
		std::vector<std::pair<VkDescriptorType, float>> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
		};
	};

	void reset_pools();
	bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

	void init(VkDevice newDevice);

	void cleanup();

	VkDevice m_Device;
private:
	VkDescriptorPool grab_pool();

	VkDescriptorPool m_CurrentPool{ VK_NULL_HANDLE };
	PoolSizes m_DescriptorSizes;
	std::vector<VkDescriptorPool> m_UsedPools;
	std::vector<VkDescriptorPool> m_FreePools;
};
