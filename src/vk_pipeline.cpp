#include "vk_pipeline.h"
#include "vk_scene.h"
#include "vk_manager.h"

namespace vkutil {

	VkPipelineLayout PipelineLayoutCache::create_pipeline_layout(VkPipelineLayoutCreateInfo &info)
	{
		CORE_ASSERT(m_Device, "PipelineLayoutCache is not initialized");

		PipelineLayoutInfo layoutInfo;

		for (uint32_t i = 0; i < info.setLayoutCount; i++) {
			layoutInfo.m_Layouts.push_back(info.pSetLayouts[i]);
		}

		auto it = m_LayoutCache.find(layoutInfo);

		if (it != m_LayoutCache.end()) return (*it).second;
		else {
			VkPipelineLayout layout;
			vkCreatePipelineLayout(m_Device, &info, nullptr, &layout);

			m_LayoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	void PipelineLayoutCache::cleanup()
	{
		for (auto &pair : m_LayoutCache) {
			vkDestroyPipelineLayout(m_Device, pair.second, nullptr);
		}
	}

	bool PipelineLayoutCache::PipelineLayoutInfo::operator==(const PipelineLayoutInfo &other) const {

		if (other.m_Layouts.size() != m_Layouts.size()) return false;

		for (uint32_t i = 0; i < m_Layouts.size(); i++) {
			if (other.m_Layouts[i] != m_Layouts[i]) return false;
		}

		return true;
	}

	size_t PipelineLayoutCache::PipelineLayoutInfo::hash() const {
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(m_Layouts.size());

		for (const VkDescriptorSetLayout &l : m_Layouts) {
			result ^= hash<size_t>()((uint64_t)l);
		}

		return result;
	}

	PipelineBuilder::PipelineBuilder(VulkanManager &manager)
	{
		m_Device = manager.get_device();
		m_LayoutCache = &manager.get_pipeline_layout_cache();
		m_VertexInputInfo = vkinit::vertex_input_state_create_info();
	}

	PipelineBuilder &PipelineBuilder::set_color_format(VkFormat format)
	{
		m_ColorFormat = format;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::add_shader_module(VkShaderModule shaderModule, VkShaderStageFlagBits shaderType)
	{
		m_ShaderStageInfo.push_back(vkinit::pipeline_shader_stage_create_info(shaderType, shaderModule));
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_renderpass(VkRenderPass renderpass)
	{
		m_RenderPass = renderpass;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_vertex_description(
		std::vector<VkVertexInputAttributeDescription> &attributes,
		std::vector<VkVertexInputBindingDescription> &bindings) {
		m_VertexInputInfo.pVertexAttributeDescriptions = attributes.data();
		m_VertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
		m_VertexInputInfo.pVertexBindingDescriptions = bindings.data();
		m_VertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindings.size();
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_vertex_description(VertexInputDescription &desc)
	{
		return set_vertex_description(desc.attributes, desc.bindings);
	}

	PipelineBuilder &PipelineBuilder::set_vertex_description(
		VkVertexInputAttributeDescription *pAttributes, uint32_t attributesCount,
		VkVertexInputBindingDescription *pBindings, uint32_t bindingCount)
	{
		m_VertexInputInfo.pVertexAttributeDescriptions = pAttributes;
		m_VertexInputInfo.vertexAttributeDescriptionCount = attributesCount;
		m_VertexInputInfo.pVertexBindingDescriptions = pBindings;
		m_VertexInputInfo.vertexBindingDescriptionCount = bindingCount;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_depth_stencil(bool depthTest, bool depthWrite, VkCompareOp compareOp, VkFormat depthFormat)
	{
		m_EnableDepthStencil = true;
		m_DepthStencil = vkinit::depth_stencil_create_info(depthTest, depthWrite, compareOp);
		m_DepthFormat = depthFormat;
		return *this;
	}

	PipelineBuilder &PipelineBuilder::set_descriptor_layouts(std::initializer_list<VkDescriptorSetLayout> layouts)
	{
		m_DescriptorSetLayout = std::vector<VkDescriptorSetLayout>(layouts);
		return *this;
	}

	bool PipelineBuilder::build(VkPipeline *pipeline, VkPipelineLayout *pipelineLayout)
	{

		VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
		layoutInfo.pSetLayouts = m_DescriptorSetLayout.data();
		layoutInfo.setLayoutCount = (uint32_t)m_DescriptorSetLayout.size();

		VkPipelineLayout layout = m_LayoutCache->create_pipeline_layout(layoutInfo);

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		auto colorBlendAttachment = vkinit::color_blend_attachment_state();
		auto inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		auto rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
		auto multisampling = vkinit::multisampling_state_create_info();

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkFormat color_rendering_format = m_ColorFormat;

		VkPipelineRenderingCreateInfoKHR pipeline_create{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
		pipeline_create.pNext = VK_NULL_HANDLE;
		pipeline_create.colorAttachmentCount = 1;
		pipeline_create.pColorAttachmentFormats = &color_rendering_format;
		pipeline_create.depthAttachmentFormat = m_DepthFormat;
		//pipeline_create.stencilAttachmentFormat = 0;

		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.pNext = &pipeline_create;

		createInfo.stageCount = (uint32_t)m_ShaderStageInfo.size();
		createInfo.pStages = m_ShaderStageInfo.data();
		createInfo.pVertexInputState = &m_VertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssembly;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizer;
		createInfo.pMultisampleState = &multisampling;
		createInfo.pColorBlendState = &colorBlending;
		createInfo.layout = layout;
		createInfo.renderPass = m_RenderPass;
		createInfo.subpass = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (m_EnableDepthStencil)
			createInfo.pDepthStencilState = &m_DepthStencil;

		VkPipelineDynamicStateCreateInfo dynStateInfo{};
		VkDynamicState dynStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynStateInfo.pNext = nullptr;
		dynStateInfo.pDynamicStates = dynStates;
		dynStateInfo.dynamicStateCount = 2;

		createInfo.pDynamicState = &dynStateInfo;

		*pipelineLayout = layout;
		auto res = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &createInfo, nullptr, pipeline);

		return res == VK_SUCCESS;

	}

	bool PipelineBuilder::build(VkPipeline *pipeline)
	{
		VkPipelineLayout layout;
		return build(pipeline, &layout);
	}


} //namespace vkutil
