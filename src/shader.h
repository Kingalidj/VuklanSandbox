#pragma once

namespace vkutil {
	class VulkanShader;
}

namespace Atlas {

	class Descriptor;

	enum class VertexAttribute {
		INT,
		INT2,
		INT3,
		INT4,
		FLOAT,
		FLOAT2,
		FLOAT3,
		FLOAT4,
	};

	enum class ShaderStage {
		NONE,
		VERTEX,
		FRAGMENT,
	};

	class VertexDescription {
	public:

		template <typename T, typename U>
		VertexDescription &push_attrib(VertexAttribute type, U T:: *member) {
			if (m_SizeOfVertex != 0 && m_SizeOfVertex != sizeof(T)) {
				CORE_WARN("previous Vertex had size: {} now the size is: {}", m_SizeOfVertex, sizeof(T));
				return *this;
			}

			m_SizeOfVertex = sizeof(T);
			m_Attributes.push_back({ type, (uint32_t)offset_of(member) });
			return *this;
		}

		std::vector<std::pair<VertexAttribute, uint32_t>> get_attributes() { return m_Attributes; }

		inline uint32_t get_stride() { return m_SizeOfVertex; }

	private:

		template<typename T, typename U> constexpr size_t offset_of(U T:: *member)
		{
			return (char *)&((T *)nullptr->*member) - (char *)nullptr;
		}

		std::vector<std::pair<VertexAttribute, uint32_t>> m_Attributes; //type, offset
		uint32_t m_SizeOfVertex = 0;
	};

	class ShaderModule {
	public:
		ShaderModule() = default;
		ShaderModule(const char *path, ShaderStage stage, bool optimize = false);


		inline ShaderStage get_stage() { return m_Stage; }
		std::vector<uint32_t> &get_data() { return *m_Data.get(); }
		const char *get_file_path() { return m_Path.c_str(); }

	private:

		friend std::optional<ShaderModule> load_shader_module(const char *path, ShaderStage stage, bool optimize = false);

		ShaderStage m_Stage{ ShaderStage::NONE };
		Ref<std::vector<uint32_t>> m_Data;
		std::string m_Path{};
		bool m_Optimization{ false };
	};

	struct ShaderCreateInfo {
		std::vector<ShaderModule> modules;
		VertexDescription vertexDescription;

		std::vector<Descriptor> descriptors;
	};


	class Shader {
	public:

		Shader() = default;
		Shader(ShaderCreateInfo &info);
		Shader(const Shader &other) = delete;

		void bind();

	private:

		Ref<vkutil::VulkanShader> m_Shader;
	};
}
