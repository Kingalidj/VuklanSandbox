#include "vk_pipeline.h"

namespace vkutil {

	PipelineBuilder &
		PipelineBuilder::add_shader_module(VkShaderModule shaderModule,
			ShaderType shaderType) {
		VkShaderStageFlagBits flag;

		switch (shaderType) {
		case ShaderType::Vertex:
			flag = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderType::Fragment:
			flag = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		}

		shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(flag, shaderModule));
		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_viewport(VkViewport viewport) {
		this->viewport = viewport;
		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_scissor(VkRect2D scissor) {
		this->scissor = scissor;
		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_scissor(VkOffset2D offset,
		VkExtent2D extent) {
		scissor.offset = offset;
		scissor.extent = extent;
		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_vertex_description(
		VkVertexInputAttributeDescription *pAttributes, uint32_t attributesCount,
		VkVertexInputBindingDescription *pBindings, uint32_t bindingCount) {
		vertexInputInfo.pVertexAttributeDescriptions = pAttributes;
		vertexInputInfo.vertexAttributeDescriptionCount = attributesCount;
		vertexInputInfo.pVertexBindingDescriptions = pBindings;
		vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_depth_stencil(bool depthTest,
		bool depthWrite,
		VkCompareOp compareOp) {
		enableDepthStencil = true;
		depthStencil = vkinit::depth_stencil_create_info(depthTest, depthWrite, compareOp);
		return *this;
	}

	std::optional<VkPipeline> PipelineBuilder::build() {

		if (pipelineLayout == VK_NULL_HANDLE) {
			CORE_ERROR("Could not build Pipeline: VkPipelineLayout is not set");
			return std::nullopt;
		}

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;

		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType =
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;

		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		if (enableDepthStencil)
			pipelineInfo.pDepthStencilState = &depthStencil;

		VkPipelineDynamicStateCreateInfo dynStateInfo{};
		VkDynamicState dynStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynStateInfo.pNext = nullptr;
		dynStateInfo.pDynamicStates = dynStates;
		dynStateInfo.dynamicStateCount = 2;

		pipelineInfo.pDynamicState = &dynStateInfo;

		VkPipeline pipeLine;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
			nullptr, &pipeLine) != VK_SUCCESS) {
			return std::nullopt;
		}
		else {
			return pipeLine;
		}
	}

	PipelineBuilder &PipelineBuilder::set_viewport(VkOffset2D offset,
		VkExtent2D size, glm::vec2 depth) {
		viewport.x = offset.x;
		viewport.y = offset.y;
		viewport.width = size.width;
		viewport.height = size.height;
		viewport.minDepth = depth.x;
		viewport.maxDepth = depth.y;

		set_scissor(offset, size);

		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_vertex_description(
		std::vector<VkVertexInputAttributeDescription> &attributes,
		std::vector<VkVertexInputBindingDescription> &bindings) {
		vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
		vertexInputInfo.vertexAttributeDescriptionCount = attributes.size();
		vertexInputInfo.pVertexBindingDescriptions = bindings.data();
		vertexInputInfo.vertexBindingDescriptionCount = bindings.size();
		return *this;
	}

} //namespace vkutil
