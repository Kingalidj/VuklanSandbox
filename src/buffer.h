#pragma once

namespace vkutil {
	class VulkanBuffer;
	struct AllocatedBuffer;
}

namespace Atlas {

	enum class BufferType : uint32_t {
		NONE = 0,
		VERTEX,
		INDEX_U16,
		INDEX_U32,
		UNIFORM,
		STORAGE,
	};

	struct BufferCreateInfo {
		BufferType type;

		bool constMemory;
		uint32_t size;
		void *memory;
	};

	class Buffer {
	public:

		Buffer() = default;
		Buffer(BufferType type, void *data, uint32_t size, bool constMemory = false);
		Buffer(BufferType type, uint32_t size);
		Buffer(const Buffer &other) = delete;

		void set_data(void *data, uint32_t size);
		void bind(uint64_t offset = 0);
		uint32_t size();

		vkutil::AllocatedBuffer *get_native_buffer();

		BufferType get_type();
		inline bool is_init() { return m_Initialized; }

	private:

		Ref<vkutil::VulkanBuffer> m_Buffer;
		bool m_Initialized{ false };
		BufferType m_Type{ BufferType::NONE };
	};
}
