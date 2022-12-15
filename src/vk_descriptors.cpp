#include "vk_descriptors.h"
#include "vk_manager.h"

namespace vkutil {

	VkDescriptorBufferInfo descriptor_buffer_info(AllocatedBuffer &buffer, uint32_t size)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = size;
		return bufferInfo;
	}

	std::vector<VkDescriptorImageInfo> descriptor_image_array_info(VkTexture *texture, uint32_t size)
	{
		std::vector<VkDescriptorImageInfo> descImageInfos;

		for (uint32_t i = 0; i < size; i++) {
			VkDescriptorImageInfo info{};
			info.sampler = texture[i].sampler;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info.imageView = texture[i].imageView;
			descImageInfos.push_back(info);
		}

		return descImageInfos;
	}

	VkDescriptorImageInfo descriptor_image_info(VkTexture &texture)
	{
		VkDescriptorImageInfo imageBufferInfo{};
		imageBufferInfo.sampler = texture.sampler;
		imageBufferInfo.imageView = texture.imageView;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		return imageBufferInfo;
	}

	VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes &poolSizes, int count, VkDescriptorPoolCreateFlags flags) {
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.sizes.size());

		for (auto &sz : poolSizes.sizes) {
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

	void DescriptorAllocator::reset_pools() {
		for (auto p : m_UsedPools) {
			vkResetDescriptorPool(m_Device, p, 0);
			m_FreePools.push_back(p);
		}

		m_UsedPools.clear();
		m_CurrentPool = VK_NULL_HANDLE;
	}

	bool DescriptorAllocator::allocate(VkDescriptorSet *set, VkDescriptorSetLayout layout) {
		CORE_ASSERT(m_Device, "DescriptorAllocator is not initialized");

		if (m_CurrentPool == VK_NULL_HANDLE) {
			m_CurrentPool = grab_pool();
			m_UsedPools.push_back(m_CurrentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorPool = m_CurrentPool;
		allocInfo.descriptorSetCount = 1;

		VkResult allocResult = vkAllocateDescriptorSets(m_Device, &allocInfo, set);
		bool needReallocate = false;

		switch (allocResult) {
		case VK_SUCCESS:
			return true;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			needReallocate = true;
			break;
		default:
			CORE_WARN("ERROR: {}", allocResult);
			CORE_WARN("Could not allocate descriptor set with layout: {}", (void *)layout);
			return false;
		}

		if (needReallocate) {
			m_CurrentPool = grab_pool();
			m_UsedPools.push_back(m_CurrentPool);

			allocResult = vkAllocateDescriptorSets(m_Device, &allocInfo, set);

			if (allocResult == VK_SUCCESS) return true;
		}

		CORE_WARN("ERROR: {}", allocResult);
		CORE_WARN("Could not allocate descriptor set with layout: {}", (void *)layout);
		return false;
	}

	void DescriptorAllocator::cleanup() {
		for (auto p : m_FreePools) {
			vkDestroyDescriptorPool(m_Device, p, nullptr);
		}
		for (auto p : m_UsedPools) {
			vkDestroyDescriptorPool(m_Device, p, nullptr);
		}
	}

	VkDescriptorPool DescriptorAllocator::grab_pool() {
		CORE_ASSERT(m_Device, "DescriptorAllocator is not initialized");

		if (m_FreePools.size() > 0) {
			VkDescriptorPool pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		else {
			return createPool(m_Device, m_DescriptorSizes, 1000, 0);
		}
	}

	void DescriptorLayoutCache::cleanup() {
		for (auto &pair : m_LayoutCache) {
			vkDestroyDescriptorSetLayout(m_Device, pair.second, nullptr);
		}
	}

	VkDescriptorSetLayout DescriptorLayoutCache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo &info) {
		CORE_ASSERT(m_Device, "DescriptorLayoutCache is not initialized");

		DescriptorLayoutInfo layoutInfo;
		bool isSorted = true;
		int lastBinding = -1;

		for (uint32_t i = 0; i < info.bindingCount; i++) {
			layoutInfo.bindings.push_back(info.pBindings[i]);

			if (info.pBindings[i].binding > (uint32_t)lastBinding) {
				lastBinding = info.pBindings[i].binding;
			}
			else {
				isSorted = false;
			}
		}

		if (!isSorted) {
			std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(),
				[](VkDescriptorSetLayoutBinding &a, VkDescriptorSetLayoutBinding &b) {
				return a.binding < b.binding;
			});
		}

		auto it = m_LayoutCache.find(layoutInfo);
		if (it != m_LayoutCache.end()) return (*it).second;
		else {
			VkDescriptorSetLayout layout;
			vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layout);

			m_LayoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo &other) const {
		if (other.bindings.size() != bindings.size()) return false;

		for (int i = 0; i < bindings.size(); i++) {
			if (other.bindings[i].binding != bindings[i].binding) return false;
			if (other.bindings[i].descriptorType != bindings[i].descriptorType)	return false;
			if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) return false;
			if (other.bindings[i].stageFlags != bindings[i].stageFlags) return false;
		}

		return true;
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const {
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(bindings.size());

		for (const VkDescriptorSetLayoutBinding &b : bindings) {
			size_t bindingHash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

			result ^= hash<size_t>()(bindingHash);
		}

		return result;
	}

	DescriptorBuilder::DescriptorBuilder(VulkanManager &manager)
	{
		m_LayoutCache = &manager.get_descriptor_layout_cache();
		m_Alloc = &manager.get_descriptor_allocator();
	}

	DescriptorBuilder &DescriptorBuilder::bind_buffer(uint32_t binding, AllocatedBuffer &buffer, uint32_t size, VkDescriptorType type, VkShaderStageFlags flags)
	{
		//VkDescriptorBufferInfo bufferInfo{};
		//bufferInfo.buffer = buffer.buffer;
		//bufferInfo.offset = 0;
		//bufferInfo.range = size;
		auto info = descriptor_buffer_info(buffer, size);

		auto [it, existed] = m_DescBufferInfos.insert({ m_DescInfoCount++, info });

		VkDescriptorSetLayoutBinding bind{};
		bind.descriptorCount = 1;
		bind.descriptorType = type;
		bind.pImmutableSamplers = nullptr;
		bind.stageFlags = flags;
		bind.binding = binding;

		m_Bindings.push_back(bind);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &it->second;
		write.dstBinding = binding;

		m_Writes.push_back(write);
		return *this;

	}

	DescriptorBuilder &DescriptorBuilder::bind_image(uint32_t binding, VkTexture &image, VkDescriptorType type, VkShaderStageFlags flags)
	{
		//VkDescriptorImageInfo imageBufferInfo{};
		//imageBufferInfo.sampler = image.sampler;
		//imageBufferInfo.imageView = image.imageView;
		//imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		auto info = descriptor_image_info(image);

		auto [it, existed] = m_DescImageInfos.insert({ m_DescInfoCount++, info });

		VkDescriptorSetLayoutBinding bind{};
		bind.descriptorCount = 1;
		bind.descriptorType = type;
		bind.pImmutableSamplers = nullptr;
		bind.stageFlags = flags;
		bind.binding = binding;

		m_Bindings.push_back(bind);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = &it->second;
		write.dstBinding = binding;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorBuilder &DescriptorBuilder::bind_image_array(uint32_t binding, VkTexture *images, uint32_t imageCount, VkDescriptorType type, VkShaderStageFlags flags)
	{
		//std::vector<VkDescriptorImageInfo> descImageInfos;
		//for (uint32_t i = 0; i < imageCount; i++) {
		//	VkDescriptorImageInfo info{};
		//	info.sampler = images[i].sampler;
		//	info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//	info.imageView = images[i].imageView;
		//	descImageInfos.push_back(info);
		//}
		auto info = descriptor_image_array_info(images, imageCount);

		auto [it, existed] = m_DescImageArrayInfos.insert({ m_DescInfoCount++, info });

		VkDescriptorSetLayoutBinding bind{};
		bind.descriptorCount = imageCount;
		bind.descriptorType = type;
		bind.pImmutableSamplers = nullptr;
		bind.stageFlags = flags;
		bind.binding = binding;

		m_Bindings.push_back(bind);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.descriptorCount = imageCount;
		write.descriptorType = type;
		write.pImageInfo = it->second.data();
		write.dstBinding = binding;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorBuilder &DescriptorBuilder::enable_push_descriptor()
	{
		m_Pushable = true;
		return *this;
	}

	bool DescriptorBuilder::build(VkDescriptorSet *set, VkDescriptorSetLayout *layout) {
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;
		layoutInfo.pBindings = m_Bindings.data();
		layoutInfo.bindingCount = (uint32_t)m_Bindings.size();

		if (m_Pushable) {
			layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			*layout = m_LayoutCache->create_descriptor_layout(layoutInfo);
			return true;
		}

		*layout = m_LayoutCache->create_descriptor_layout(layoutInfo);

		bool success = m_Alloc->allocate(set, *layout);
		if (!success) { return false; };

		for (uint32_t i = 0; i < m_Writes.size(); i++) {
			m_Writes.at(i).dstSet = *set;
		}

		vkUpdateDescriptorSets(m_Alloc->m_Device, (uint32_t)m_Writes.size(), m_Writes.data(), 0, nullptr);

		return true;
	}

	bool DescriptorBuilder::build(VkDescriptorSet *set) {
		VkDescriptorSetLayout layout{};
		return build(set, &layout);
	}

	uint32_t DescriptorBuilder::get_layout_count()
	{
		return uint32_t(m_DescImageInfos.size() + m_DescBufferInfos.size() + m_DescImageArrayInfos.size());
	}

	void descriptor_update_buffer(VulkanManager &manager, VkDescriptorSet *set, uint32_t binding,
		AllocatedBuffer &buffer, uint32_t size, VkDescriptorType type, VkShaderStageFlags flags)
	{
		//VkDescriptorBufferInfo bufferInfo{};
		//bufferInfo.buffer = buffer.buffer;
		//bufferInfo.offset = 0;
		//bufferInfo.range = size;
		auto info = descriptor_buffer_info(buffer, size);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &info;
		write.dstBinding = binding;
		write.dstSet = *set;

		vkUpdateDescriptorSets(manager.device(), 1, &write, 0, nullptr);
	}

	void descriptor_update_image(VulkanManager &manager, VkDescriptorSet *set, uint32_t binding,
		VkTexture &tex, VkDescriptorType type, VkShaderStageFlags flags)
	{
		//VkDescriptorImageInfo imageBufferInfo{};
		//imageBufferInfo.sampler = tex.sampler;
		//imageBufferInfo.imageView = tex.imageView;
		//imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		auto info = descriptor_image_info(tex);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = &info;
		write.dstBinding = binding;
		write.dstSet = *set;

		vkUpdateDescriptorSets(manager.device(), 1, &write, 0, nullptr);
	}

	void descriptor_update_image_array(VulkanManager &manager, VkDescriptorSet *set, uint32_t binding, VkTexture *tex, uint32_t imgCount, VkDescriptorType type, VkShaderStageFlags flags)
	{
		//std::vector<VkDescriptorImageInfo> descImageInfos;
		//for (uint32_t i = 0; i < imgCount; i++) {
		//	VkDescriptorImageInfo info{};
		//	info.sampler = tex[i].sampler;
		//	info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//	info.imageView = tex[i].imageView;
		//	descImageInfos.push_back(info);
		//}
		auto info = descriptor_image_array_info(tex, imgCount);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.descriptorCount = imgCount;
		write.descriptorType = type;
		write.pImageInfo = info.data();
		write.dstBinding = binding;
		write.dstSet = *set;

		vkUpdateDescriptorSets(manager.device(), 1, &write, 0, nullptr);
	}

} //namespace vkutil
