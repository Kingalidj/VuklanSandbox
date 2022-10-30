#include "vk_resources.h"

#include "vk_initializers.h"

void VulkanResourceManager::init(VkDevice device, VmaAllocator allocator) {
	m_Device = device;
	m_Allocator = allocator;
}

void VulkanResourceManager::set_texture(const std::string &name, Ref<Texture> tex) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	m_Textures[name] = tex;
}

void VulkanResourceManager::set_mesh(const std::string &name, Ref<Mesh> mesh) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	m_Meshes[name] = mesh;
}

void VulkanResourceManager::set_material(const std::string &name, Ref<Material> material) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	m_Materials[name] = material;
}

void VulkanResourceManager::cleanup() {
	m_DeletionQueue.flush();
}

std::optional<Ref<Texture>> VulkanResourceManager::get_texture(const std::string &name) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");

	auto tex = m_Textures.find(name);

	if (tex != m_Textures.end()) {
		return tex->second;
	}
	else {
		return std::nullopt;
	}
}

std::optional<Ref<Mesh>> VulkanResourceManager::get_mesh(const std::string &name) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");

	auto mesh = m_Meshes.find(name);

	if (mesh != m_Meshes.end()) {
		return mesh->second;
	}
	else {
		return std::nullopt;
	}
}

std::optional<Ref<Material>> VulkanResourceManager::get_material(const std::string &name) {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");

	auto mat = m_Materials.find(name);

	if (mat != m_Materials.end()) {
		return mat->second;
	}
	else {
		return std::nullopt;
	}
}

std::optional<Ref<Material>> VulkanResourceManager::create_material(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout) {
	Ref<Material> mat = make_ref<Material>();
	mat->pipeline = pipeline;
	mat->pipelineLayout = layout;
	set_material(std::move(name), mat);

	return mat;
}

void VulkanResourceManager::delete_func(std::function<void()> &&func) {
	m_DeletionQueue.push_function(std::move(func));
}

AllocatedBuffer VulkanResourceManager::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {

	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo,
		&newBuffer.buffer, &newBuffer.allocation, nullptr));

	return newBuffer;
}

inline VkDevice VulkanResourceManager::get_device() {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	return m_Device;
}

inline VmaAllocator VulkanResourceManager::get_allocator() {
	CORE_ASSERT(m_Device, "ResourceManager not initialized");
	return m_Allocator;
}

