#include "vk_manager.h"

#include "vk_initializers.h"

void VulkanManager::init(VkDevice device, VmaAllocator allocator) {
	m_Device = device;
	m_Allocator = allocator;
}

void VulkanManager::set_texture(const std::string &name, Ref<Texture> tex) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	m_Textures[name] = tex;
}

void VulkanManager::set_mesh(const std::string &name, Ref<Mesh> mesh) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	m_Meshes[name] = mesh;
}

void VulkanManager::set_material(const std::string &name, Ref<Material> material) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	m_Materials[name] = material;
}

void VulkanManager::cleanup() {
	m_DeletionQueue.flush();
}

std::optional<Ref<Texture>> VulkanManager::get_texture(const std::string &name) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");

	auto tex = m_Textures.find(name);

	if (tex != m_Textures.end()) {
		return tex->second;
	}
	else {
		return std::nullopt;
	}
}

std::optional<Ref<Mesh>> VulkanManager::get_mesh(const std::string &name) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");

	auto mesh = m_Meshes.find(name);

	if (mesh != m_Meshes.end()) {
		return mesh->second;
	}
	else {
		return std::nullopt;
	}
}

std::optional<Ref<Material>> VulkanManager::get_material(const std::string &name) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");

	auto mat = m_Materials.find(name);

	if (mat != m_Materials.end()) {
		return mat->second;
	}
	else {
		return std::nullopt;
	}
}

std::optional<Ref<Material>> VulkanManager::create_material(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout) {
	Ref<Material> mat = make_ref<Material>();
	mat->pipeline = pipeline;
	mat->pipelineLayout = layout;
	set_material(std::move(name), mat);

	return mat;
}

void VulkanManager::delete_func(std::function<void()> &&func) {
	m_DeletionQueue.push_function(std::move(func));
}

void VulkanManager::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, AllocatedBuffer *buffer) {

	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = memoryUsage;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo,
		&buffer->buffer, &buffer->allocation, nullptr));
}

void VulkanManager::upload_to_gpu(void *copyData, uint32_t size, AllocatedBuffer &buffer, VkBufferUsageFlags flags) {

	VkBufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.size = size;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &vmaAllocInfo,
		&stagingBuffer.buffer, &stagingBuffer.allocation,
		nullptr));
	void *data;
	vmaMapMemory(m_Allocator, stagingBuffer.allocation, &data);
	memcpy(data, copyData, size);
	vmaUnmapMemory(m_Allocator, stagingBuffer.allocation);

	VkBufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	vertexBufferInfo.size = size;
	vertexBufferInfo.usage = flags;

	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &vertexBufferInfo, &vmaAllocInfo,
		&buffer.buffer, &buffer.allocation, nullptr));

	immediate_submit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = size;
		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer.buffer, 1, &copy);
		});

	vmaDestroyBuffer(m_Allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

VkDevice VulkanManager::get_device() {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	return m_Device;
}

VmaAllocator VulkanManager::get_allocator() {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	return m_Allocator;
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

