#pragma once

namespace vkutil {
	struct AllocatedBuffer;
	class VulkanBuffer;
}

namespace Atlas {

	namespace BufferType {
		enum : uint32_t {
			NONE = 0,
			VERTEX = BIT(0),
			INDEX_U16 = BIT(1),
			INDEX_U32 = BIT(2),
			UNIFORM = BIT(3),
			STORAGE = BIT(4),
		};
	}
	using BufferTypeFlags = uint32_t;

	struct BufferCreateInfo {
		BufferTypeFlags type;

		bool hostVisible;
		uint32_t bufferSize;
	};

	class Buffer {
	public:

		Buffer() = default;
		Buffer(BufferCreateInfo info);
		Buffer(const Buffer &other) = delete;

		void set_data(void *data, uint32_t size);
		void bind(uint64_t offset = 0);
		uint32_t size();

		vkutil::AllocatedBuffer *get_native_buffer();

		BufferTypeFlags get_type();
		inline bool is_init() { return m_Initialized; }

		static Buffer vertex(uint32_t size, bool hostVisible = false);
		static Buffer index_u16(uint32_t size, bool hostVisible = false);
		static Buffer index_u32(uint32_t size, bool hostVisible = false);
		static Buffer uniform(uint32_t size, bool hostVisible = false);

	private:

		Ref<vkutil::VulkanBuffer> m_Buffer;
		bool m_Initialized{ false };
	};
}
