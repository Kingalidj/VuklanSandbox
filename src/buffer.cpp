#include "buffer.h"
#include "application.h"

#include "atl_vk_utils.h"
#include "vk_engine.h"

//void upload_to_gpu(void *data, uint32_t size, vkutil::AllocatedBuffer *buffer) {
//	Atlas::Application::get_engine().manager().upload_to_gpu(data, size, *buffer,
//		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
//}

//GPU_ONLY VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
//CPU_ONLY VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
//VISIBLE VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT

VkBufferUsageFlagBits atlas_to_vk_buffer_type(Atlas::BufferType type) {
	switch (type)
	{
	case Atlas::BufferType::VERTEX:
		return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		break;
	case Atlas::BufferType::INDEX_U32:
	case Atlas::BufferType::INDEX_U16:
		return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		break;
	case Atlas::BufferType::UNIFORM:
		return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	default:
		CORE_WARN("unknown buffer type");
		return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	}
}

namespace vkutil {

	class VulkanBuffer {
	public:

		VulkanBuffer(Atlas::BufferCreateInfo &info)
			: m_Size(info.size), m_HostVisible(!info.constMemory), m_Type(info.type)
		{

			vkutil::VulkanManager &manager = Atlas::Application::get_engine().manager();

			if (info.constMemory && info.memory == nullptr) {
				CORE_WARN("VulkanBuffer: memory pointer is nullptr");
				return;
			}

			VkBufferUsageFlagBits bufferUsage = atlas_to_vk_buffer_type(info.type);
			vkutil::AllocatedBuffer buffer;
			if (m_HostVisible) {
				vkutil::create_buffer(manager, info.size,
					bufferUsage,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					&buffer);

				if (info.memory != nullptr) {
					vkutil::map_memory(Atlas::Application::get_engine().manager(), &buffer, info.memory, info.size);
					//set_data(info.memory, info.size);
				}
			}
			else {
				Atlas::Application::get_engine().manager().upload_to_gpu(info.memory, info.size, buffer,
					bufferUsage);
			}

			m_Buffer = Atlas::Application::get_engine().asset_manager().register_buffer(buffer);
		}

		~VulkanBuffer()
		{
			if (auto shared = m_Buffer.lock()) {
				Atlas::Application::get_engine().asset_manager().queue_destory_buffer(shared);
				//Atlas::Application::get_engine().asset_manager().deregister_buffer(shared);
				//vkutil::destroy_buffer(Atlas::Application::get_engine().manager(), *shared.get());
			}
		}

		void set_data(void *data, uint32_t size)
		{
			if (size > m_Size) {
				CORE_WARN("buffer memory not big enough!");
				return;
			}

			if (!m_HostVisible) {
				CORE_WARN("buffer has constant memory and can not be written to");
				return;
			}

			vkutil::map_memory(Atlas::Application::get_engine().manager(), get_native_buffer(), data, size);
		}

		vkutil::AllocatedBuffer *get_native_buffer()
		{
			if (auto buffer = m_Buffer.lock()) {
				return buffer.get();
			}

			CORE_WARN("This buffer was never created / or deleted!");
			return nullptr;
		}

		void bind(uint64_t offset)
		{
			VkCommandBuffer cmd = Atlas::Application::get_engine().get_active_command_buffer();

			switch (m_Type)
			{
			case Atlas::BufferType::VERTEX:
				vkCmdBindVertexBuffers(cmd, 0, 1, &get_native_buffer()->buffer, &offset);
				break;
			case Atlas::BufferType::INDEX_U16:
				vkCmdBindIndexBuffer(cmd, get_native_buffer()->buffer, offset, VK_INDEX_TYPE_UINT16);
				break;
			case Atlas::BufferType::INDEX_U32:
				vkCmdBindIndexBuffer(cmd, get_native_buffer()->buffer, offset, VK_INDEX_TYPE_UINT32);
				break;
			default:
				CORE_WARN("can't bind this type of buffer: {}", (uint32_t)m_Type);
				return;
			}
		}

		inline uint32_t size() { return m_Size; }

	private:
		WeakRef<AllocatedBuffer> m_Buffer;
		bool m_HostVisible{ true };
		uint32_t m_Size{ 0 };
		Atlas::BufferType m_Type{ Atlas::BufferType::NONE };
	};

}

namespace Atlas {


	Buffer::Buffer(BufferType type, void *data, uint32_t size, bool constMemory)
		:m_Type(type), m_Initialized(true)
	{
		BufferCreateInfo info{};
		info.type = type;
		info.constMemory = constMemory;
		info.size = size;
		info.memory = data;

		m_Buffer = make_ref<vkutil::VulkanBuffer>(info);
	}

	Buffer::Buffer(BufferType type, uint32_t size)
		:m_Type(type), m_Initialized(true)
	{
		BufferCreateInfo info{};
		info.type = type;
		info.constMemory = false;
		info.size = size;
		info.memory = nullptr;

		m_Buffer = make_ref<vkutil::VulkanBuffer>(info);
	}

	void Buffer::set_data(void *data, uint32_t size) {
		if (!m_Initialized) {
			CORE_WARN("Buffer was never created / or deleted");
			return;
		}

		m_Buffer->set_data(data, size);
	}

	void Buffer::bind(uint64_t offset) {
		if (!m_Initialized) {
			CORE_WARN("Buffer was never created / or deleted");
			return;
		}

		m_Buffer->bind(offset);
	}

	uint32_t Buffer::size()
	{
		if (!m_Initialized) {
			CORE_WARN("Buffer was never created / or deleted");
			return 0;
		}

		return m_Buffer->size();
	}

	vkutil::AllocatedBuffer *Buffer::get_native_buffer() {
		if (!m_Initialized) {
			CORE_WARN("Buffer was never created / or deleted");
			return nullptr;
		}

		return m_Buffer->get_native_buffer();
	}

	BufferType Buffer::get_type() {
		if (!m_Initialized) {
			CORE_WARN("Buffer was never created / or deleted");
			return BufferType::NONE;
		}

		return m_Type;
	}

}