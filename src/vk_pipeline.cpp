#include "vk_pipeline.h"
#include "vk_scene.h"

namespace vkutil {

	PipelineBuilder &
		PipelineBuilder::add_shader_module(VkShaderModule shaderModule, VkShaderStageFlagBits shaderType) {

		shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(shaderType, shaderModule));
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

	PipelineBuilder &PipelineBuilder::set_vertex_description(VertexInputDescription &desc)
	{
		return set_vertex_description(desc.attributes, desc.bindings);
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

		pipelineInfo.stageCount = (uint32_t)shaderStages.size();
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
		viewport.x = (float)offset.x;
		viewport.y = (float)offset.y;
		viewport.width = (float)size.width;
		viewport.height = (float)size.height;
		viewport.minDepth = depth.x;
		viewport.maxDepth = depth.y;

		set_scissor(offset, size);

		return *this;
	}
	PipelineBuilder &PipelineBuilder::set_vertex_description(
		std::vector<VkVertexInputAttributeDescription> &attributes,
		std::vector<VkVertexInputBindingDescription> &bindings) {
		vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
		vertexInputInfo.pVertexBindingDescriptions = bindings.data();
		vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindings.size();
		return *this;
	}

} //namespace vkutil
