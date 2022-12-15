#pragma once

#include "vk_types.h"
#include "vk_initializers.h"

#include <glm/glm.hpp>

namespace vkutil {

	class VulkanManager;

	struct VertexInputDescription {
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;

		VkPipelineVertexInputStateCreateFlags flags = 0;

	};

	enum class VertexAttributeType : int {
		INT = VK_FORMAT_R32_SINT,
		INT2 = VK_FORMAT_R32G32_SINT,
		INT3 = VK_FORMAT_R32G32B32_SINT,
		INT4 = VK_FORMAT_R32G32B32A32_SINT,
		FLOAT = VK_FORMAT_R32_SFLOAT,
		FLOAT2 = VK_FORMAT_R32G32_SFLOAT,
		FLOAT3 = VK_FORMAT_R32G32B32_SFLOAT,
		FLOAT4 = VK_FORMAT_R32G32B32A32_SFLOAT,
	};

	class VertexInputDescriptionBuilder {
	public:

		VertexInputDescriptionBuilder(uint32_t size);

		inline VertexInputDescription value() { return m_Description; }

		template <typename T, typename U>
		VertexInputDescriptionBuilder &push_attrib(VertexAttributeType type, U T:: *member) {

			VkVertexInputAttributeDescription attribute{};
			attribute.binding = 0;
			attribute.location = m_AttribLocation++;
			attribute.format = (VkFormat)type;
			attribute.offset = (uint32_t)offset_of(member);

			m_Description.attributes.push_back(attribute);
			return *this;
		}

		VertexInputDescriptionBuilder &push_attrib(VertexAttributeType type, uint32_t offset);

	private:

		VertexInputDescription m_Description;
		uint32_t m_AttribLocation = 0;

		template<typename T, typename U> constexpr size_t offset_of(U T:: *member)
		{
			return (char *)&((T *)nullptr->*member) - (char *)nullptr;
		}

	};

	struct Shader {
		VkPipelineLayout layout;
		VkPipeline pipeline;
	};

	class PipelineLayoutCache {
	public:

		PipelineLayoutCache() = default;
		PipelineLayoutCache(VkDevice device)
			: m_Device(device) {}

		VkPipelineLayout create_pipeline_layout(VkPipelineLayoutCreateInfo &info);

		void cleanup();

		struct PipelineLayoutInfo {
			std::vector<VkDescriptorSetLayout> m_Layouts;

			bool operator==(const PipelineLayoutInfo &other) const;

			size_t hash() const;
		};

	private:

		struct PipelineLayoutHash {
			std::size_t operator()(const PipelineLayoutInfo &k) const {
				return k.hash();
			}
		};

		VkDevice m_Device{ VK_NULL_HANDLE };
		std::unordered_map<PipelineLayoutInfo, VkPipelineLayout, PipelineLayoutHash> m_LayoutCache;

	};

	class PipelineBuilder {
	public:

		PipelineBuilder(VulkanManager &manager);

		PipelineBuilder &set_color_format(VkFormat format);

		PipelineBuilder &add_shader_module(VkShaderModule shaderModule, VkShaderStageFlagBits shaderType);

		PipelineBuilder &set_renderpass(VkRenderPass renderpass);

		PipelineBuilder &set_vertex_description(VertexInputDescription &descriptrion);
		PipelineBuilder &set_vertex_description(std::vector<VkVertexInputAttributeDescription> &attributes,
			std::vector<VkVertexInputBindingDescription> &bindings);
		PipelineBuilder &set_vertex_description(
			VkVertexInputAttributeDescription *pAttributes, uint32_t attributesCount,
			VkVertexInputBindingDescription *pBindings, uint32_t bindingCount);

		PipelineBuilder &set_depth_stencil(bool depthTest, bool depthWrite, VkCompareOp compareOp, VkFormat depthFormat);

		PipelineBuilder &set_descriptor_layouts(std::vector<VkDescriptorSetLayout> layouts);

		bool build(VkPipeline *pipeline, VkPipelineLayout *layout);
		bool build(VkPipeline *pipeline);
		bool build(Shader *shader);

	private:

		VkDevice m_Device{ VK_NULL_HANDLE };
		VkRenderPass m_RenderPass{ VK_NULL_HANDLE };

		PipelineLayoutCache *m_LayoutCache{ nullptr };

		VkFormat m_ColorFormat;
		VkFormat m_DepthFormat;

		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageInfo;
		VkPipelineVertexInputStateCreateInfo m_VertexInputInfo{};

		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayout;

		bool m_EnableDepthStencil = false;
		VkPipelineDepthStencilStateCreateInfo m_DepthStencil{};

	};

	bool compile_shader_module(uint32_t *buffer, uint32_t byteSize,
		VkShaderModule *outShaderModule,
		const VkDevice device);

	std::vector<uint32_t> compile_glsl_to_spirv(const std::string &source_name,
		VkShaderStageFlagBits stage, const char *source, size_t sourceSize,
		bool optimize = false);

	bool load_spirv_shader_module(const char *filePath,
		VkShaderModule *outShaderModule,
		const VkDevice device);

	bool load_glsl_shader_module(const VulkanManager &manager, std::filesystem::path filePath,
		VkShaderStageFlagBits type,
		VkShaderModule *outShaderModule);

	bool load_glsl_shader_module(const VulkanManager &manager, std::filesystem::path filePath, VkShaderModule *outShaderModule);

	void create_compute_shader(VulkanManager &manager, VkShaderModule module, std::vector<VkDescriptorSetLayout> layouts, VkPipeline *pipeline, VkPipelineLayout *pipelineLayout);
} //namespace vkutil
