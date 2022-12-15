#include "buffer.h"
#include "application.h"

#include "atl_vk_utils.h"
#include "vk_engine.h"

//GPU_ONLY VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
//CPU_ONLY VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
//VISIBLE VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT

VkBufferUsageFlags atlas_to_vk_buffer_type(Atlas::BufferTypeFlags type) {
	VkBufferUsageFlags flagBits{};
	using namespace Atlas;

	if (type & BufferType::VERTEX) flagBits |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (type & BufferType::INDEX_U32) flagBits |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (type & BufferType::INDEX_U16) flagBits |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (type & BufferType::UNIFORM) flagBits |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	CORE_ASSERT(type, "undefined BufferType");

	return flagBits;
}

namespace vkutil {
	using namespace Atlas;

	class VulkanBuffer {
	public:

		VulkanBuffer(BufferCreateInfo &info)
			: m_Size(info.bufferSize), m_HostVisible(info.hostVisible), m_Type(info.type)
		{

			vkutil::VulkanManager &manager = Application::get_engine().manager();

			vkutil::AllocatedBuffer buffer{};
			VkBufferUsageFlags bufferUsage = atlas_to_vk_buffer_type(info.type) | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			if (m_HostVisible) memoryFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

			vkutil::create_buffer(manager, info.bufferSize, bufferUsage, memoryFlags, &buffer);

			m_Buffer = Application::get_engine().asset_manager().register_buffer(buffer);
		}

		~VulkanBuffer()
		{
			if (auto shared = m_Buffer.lock()) {
				Application::get_engine().asset_manager().queue_destory_buffer(shared);
				//Application::get_engine().asset_manager().deregister_buffer(shared);
				//vkutil::destroy_buffer(Application::get_engine().manager(), *shared.get());
			}
		}

		void set_data(void *data, uint32_t size)
		{
			if (data == nullptr || size == 0) return;

			if (size > m_Size) {
				CORE_WARN("buffer memory not big enough!");
				return;
			}

			if (m_HostVisible) {
				vkutil::map_memory(Application::get_engine().manager(), *get_native_buffer(), data, size);
			}
			else {
				vkutil::staged_upload_to_buffer(Application::get_engine().manager(),
					*get_native_buffer(), data, size);
			}

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
			VkCommandBuffer cmd = Application::get_engine().get_active_command_buffer();

			switch (m_Type)
			{
			case BufferType::VERTEX:
				vkCmdBindVertexBuffers(cmd, 0, 1, &get_native_buffer()->buffer, &offset);
				break;
			case BufferType::INDEX_U16:
				vkCmdBindIndexBuffer(cmd, get_native_buffer()->buffer, offset, VK_INDEX_TYPE_UINT16);
				break;
			case BufferType::INDEX_U32:
				vkCmdBindIndexBuffer(cmd, get_native_buffer()->buffer, offset, VK_INDEX_TYPE_UINT32);
				break;
			default:
				CORE_WARN("can't bind this type of buffer: {}", m_Type);
				return;
			}
		}

		BufferTypeFlags get_type() {
			return m_Type;
		}

		inline uint32_t size() { return m_Size; }

	private:
		WeakRef<vkutil::AllocatedBuffer> m_Buffer;
		bool m_HostVisible{ false };
		uint32_t m_Size{ 0 };
		BufferTypeFlags m_Type{ BufferType::NONE };
	};

}

namespace Atlas {

	Buffer::Buffer(BufferCreateInfo info)
		:m_Initialized(true)
	{
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

	BufferTypeFlags Buffer::get_type() {
		if (!m_Initialized) {
			CORE_WARN("Buffer was never created / or deleted");
			return BufferType::NONE;
		}

		return m_Buffer->get_type();
	}

	Buffer Buffer::vertex(uint32_t size, bool hostVisible)
	{
		BufferCreateInfo info{};
		info.bufferSize = size;
		info.hostVisible = hostVisible;
		info.type = BufferType::VERTEX;
		return Buffer(info);
	}

	Buffer Buffer::index_u16(uint32_t size, bool hostVisible)
	{
		BufferCreateInfo info{};
		info.bufferSize = size;
		info.hostVisible = hostVisible;
		info.type = BufferType::INDEX_U16;
		return Buffer(info);
	}

	Buffer Buffer::index_u32(uint32_t size, bool hostVisible)
	{
		BufferCreateInfo info{};
		info.bufferSize = size;
		info.hostVisible = hostVisible;
		info.type = BufferType::INDEX_U32;
		return Buffer(info);
	}
	Buffer Buffer::uniform(uint32_t size, bool hostVisible)
	{
		BufferCreateInfo info{};
		info.bufferSize = size;
		info.hostVisible = hostVisible;
		info.type = BufferType::UNIFORM;
		return Buffer(info);
	}
}