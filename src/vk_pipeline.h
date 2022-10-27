#pragma once

#include "vk_types.h"
#include "vk_initializers.h"

#include <glm/glm.hpp>

namespace vkutil {

	struct PipelineBuilder {
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkViewport viewport = {};
		VkRect2D scissor = {};
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineLayout pipelineLayout;
		bool enableDepthStencil = false;
		VkPipelineDepthStencilStateCreateInfo depthStencil{};

		VkDevice device;
		VkRenderPass renderPass;

		PipelineBuilder(VkDevice _device, VkRenderPass _renderPass, VkPipelineLayout _layout)
			: device(_device), renderPass(_renderPass), pipelineLayout(_layout),
			multisampling(vkinit::multisampling_state_create_info()),
			colorBlendAttachment(vkinit::color_blend_attachment_state()),
			rasterizer(vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL)),
			inputAssembly(
				vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)),
			vertexInputInfo(vkinit::vertex_input_state_create_info()) {
		}

		PipelineBuilder &add_shader_module(VkShaderModule shaderModule,
			ShaderType shaderType);
		PipelineBuilder &set_viewport(VkViewport viewport);
		PipelineBuilder &set_viewport(VkOffset2D offset, VkExtent2D size,
			glm::vec2 depth);
		PipelineBuilder &set_scissor(VkRect2D scissor);
		PipelineBuilder &set_scissor(VkOffset2D offset, VkExtent2D extent);
		PipelineBuilder &set_depth_stencil(bool depthTest, bool depthWrite,
			VkCompareOp compareOp);
		PipelineBuilder &set_vertex_description(
			VkVertexInputAttributeDescription *pAttributes, uint32_t attributesCount,
			VkVertexInputBindingDescription *pBindings, uint32_t bindingCount);
		PipelineBuilder &set_vertex_description(
			std::vector<VkVertexInputAttributeDescription> &attributes,
			std::vector<VkVertexInputBindingDescription> &bindings);

		std::optional<VkPipeline> build();
	};

} //namespace vkutil
