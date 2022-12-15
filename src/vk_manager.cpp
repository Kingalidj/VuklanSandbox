#include "vk_manager.h"
#include "vk_initializers.h"

namespace vkutil {

	void VulkanManager::init(VkDevice device, VmaAllocator allocator) {
		m_Device = device;
		m_Allocator = allocator;

		m_DescriptorAllocator = DescriptorAllocator(m_Device);
		m_DescriptorLayoutCache = DescriptorLayoutCache(m_Device);
		m_PipelineLayoutCache = PipelineLayoutCache(m_Device);
	}

	void VulkanManager::cleanup() {
		m_DeletionQueue.flush();
		m_DescriptorLayoutCache.cleanup();
		m_DescriptorAllocator.cleanup();
		m_PipelineLayoutCache.cleanup();
	}

	void VulkanManager::delete_func(std::function<void()> &&func) {
		m_DeletionQueue.push_function(std::move(func));
	}

	const VkDevice VulkanManager::device() const {
		CORE_ASSERT(m_Device, "ResourceManager not initialized");
		return m_Device;
	}

	const VmaAllocator VulkanManager::get_allocator() const {
		CORE_ASSERT(m_Device, "ResourceManager not initialized");
		return m_Allocator;
	}

	DescriptorAllocator &VulkanManager::get_descriptor_allocator() {
		CORE_ASSERT(m_Device, "ResourceManager not initialized");
		return m_DescriptorAllocator;
	}

	DescriptorLayoutCache &VulkanManager::get_descriptor_layout_cache() {
		CORE_ASSERT(m_Device, "ResourceManager not initialized");
		return m_DescriptorLayoutCache;
	}

	PipelineLayoutCache &VulkanManager::get_pipeline_layout_cache()
	{
		CORE_ASSERT(m_Device, "ResourceManager not initialized");
		return m_PipelineLayoutCache;
	}

	void VulkanManager::init_commands(VkQueue queue, uint32_t queueFamilyIndex) {
		CORE_ASSERT(m_Device, "ResourceManager not initialized");

		m_Queue = queue;
		m_QueueFamilyIndex = queueFamilyIndex;

		VkCommandPoolCreateInfo uploadCommandPoolInfo =
			vkinit::command_pool_create_info(queueFamilyIndex);

		VK_CHECK(vkCreateCommandPool(m_Device, &uploadCommandPoolInfo, nullptr, &m_UploadContext.commandPool));

		VkCommandBufferAllocateInfo cmdAllocInfo =
			vkinit::command_buffer_allocate_info(m_UploadContext.commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo,
			&m_UploadContext.commandBuffer));

		m_DeletionQueue.push_function([=]() {
			vkDestroyCommandPool(m_Device, m_UploadContext.commandPool, nullptr);
		});

	}

	void VulkanManager::init_sync_structures() {
		VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();

		VK_CHECK(vkCreateFence(m_Device, &uploadFenceCreateInfo, nullptr,
			&m_UploadContext.uploadFence));

		m_DeletionQueue.push_function([=]() {
			vkDestroyFence(m_Device, m_UploadContext.uploadFence, nullptr);
		});
	}

	void VulkanManager::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&func) {
		CORE_ASSERT(m_Queue, "Queue not initialized");

		VkCommandBuffer cmd = m_UploadContext.commandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		func(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkSubmitInfo submit = vkinit::submit_info(&cmd);

		VK_CHECK(vkQueueSubmit(m_Queue, 1, &submit, m_UploadContext.uploadFence));

		vkWaitForFences(m_Device, 1, &m_UploadContext.uploadFence, true, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_UploadContext.uploadFence);

		vkResetCommandPool(m_Device, m_UploadContext.commandPool, 0);
	}


	void AssetManager::cleanup(VulkanManager &manager) {
		destroy_queued(manager);

		for (auto &shader : m_Shaders) vkDestroyPipeline(manager.device(), shader->pipeline, nullptr);
		for (auto &texture : m_Textures) destroy_texture(manager, *texture.get());
		for (auto &buffer : m_Buffers) destroy_buffer(manager, *buffer.get());

		m_Shaders.clear();
		m_Textures.clear();
		m_Buffers.clear();
	}

	void AssetManager::destroy_queued(VulkanManager &manager)
	{
		for (auto &shader : m_DeletedShaders) vkDestroyPipeline(manager.device(), shader->pipeline, nullptr);
		for (auto &texture : m_DeletedTextures) destroy_texture(manager, *texture.get());
		for (auto &buffer : m_DeletedBuffers) destroy_buffer(manager, *buffer.get());

		m_DeletedShaders.clear();
		m_DeletedTextures.clear();
		m_DeletedBuffers.clear();
	}

	void AssetManager::queue_destory_buffer(Ref<AllocatedBuffer> &buffer) {
		auto it = m_Buffers.find(buffer);

		if (it == m_Buffers.end()) {
			CORE_WARN("Buffer was never registered: {}", buffer);
		}

		m_Buffers.erase(it);
		m_DeletedBuffers.insert(buffer);
	}

	void AssetManager::deregister_buffer(Ref<AllocatedBuffer> &buffer) {
		auto it = m_Buffers.find(buffer);

		if (it == m_Buffers.end()) {
			CORE_WARN("Buffer was never registered: {}", buffer);
		}

		m_Buffers.erase(it);
	}

	void AssetManager::queue_destroy_shader(Ref<Shader> &shader) {
		auto it = m_Shaders.find(shader);

		if (it == m_Shaders.end()) {
			CORE_WARN("Shader was never registered: {}", shader);
		}

		m_Shaders.erase(it);
		m_DeletedShaders.insert(shader);
	}

	void AssetManager::deregister_shader(Ref<Shader> &shader) {
		auto it = m_Shaders.find(shader);

		if (it == m_Shaders.end()) {
			CORE_WARN("Shader was never registered: {}", shader);
		}

		m_Shaders.erase(it);
	}

	void AssetManager::queue_destroy_texture(Ref<VkTexture> &texture) {
		auto it = m_Textures.find(texture);

		if (it == m_Textures.end()) {
			CORE_WARN("Texture was never registered: {}", texture);
		}

		m_Textures.erase(it);
		m_DeletedTextures.insert(texture);
	}

	void AssetManager::deregister_texture(Ref<VkTexture> &texture) {
		auto it = m_Textures.find(texture);

		if (it == m_Textures.end()) {
			CORE_WARN("Texture was never registered: {}", texture);
		}

		m_Textures.erase(it);
	}

}
