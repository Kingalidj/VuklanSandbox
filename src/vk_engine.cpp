#include "vk_engine.h"

#include "vk_initializers.h"
#include "vk_shader.h"
#include "vk_types.h"

#include "imgui_theme.h"

const uint8_t c_RobotoRegular[] = {
#include "robot_regular.embed"
};

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>

#include <cmath>

void VulkanEngine::init() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_Window = glfwCreateWindow(m_WindowExtent.width, m_WindowExtent.height,
		"Vulkan Engine", nullptr, nullptr);

	{
		int w, h;
		glfwGetFramebufferSize(m_Window, &w, &h);
		m_WindowExtent.width = static_cast<uint32_t>(w);
		m_WindowExtent.height = static_cast<uint32_t>(h);
	}

	init_vulkan();
	init_swapchain();
	init_commands();
	init_default_renderpass();
	init_framebuffers();
	init_sync_structures();
	init_descriptors();
	init_pipelines();

	init_imgui();

	load_images();
	load_meshes();

	init_scene();

	m_IsInitialized = true;
}

Material* VulkanEngine::create_material(VkPipeline pipeline,
	VkPipelineLayout layout,
	const std::string& name) {
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	m_Materials[name] = mat;
	return &m_Materials[name];
}

Material* VulkanEngine::get_material(const std::string& name) {
	auto it = m_Materials.find(name);
	if (it == m_Materials.end())
		return nullptr;
	return &(*it).second;
}

Mesh* VulkanEngine::get_mesh(const std::string& name) {
	auto it = m_Meshes.find(name);
	if (it == m_Meshes.end())
		return nullptr;
	return &(*it).second;
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize,
	VkBufferUsageFlags usage,
	VmaMemoryUsage memoryUsage) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo,
		&newBuffer.buffer, &newBuffer.allocation, nullptr));

	return newBuffer;
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject* first,
	int count) {
	glm::vec3 camPos = { 0.f, -3.f, -9.f };

	glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
	glm::mat4 projection = glm::perspective(
		glm::radians(70.f),
		(float)m_WindowExtent.width / (float)m_WindowExtent.height, 0.1f, 200.0f);
	projection[1][1] *= -1;

	glm::mat4 rotMat =
		glm::rotate(glm::mat4(1), glm::radians((float)m_FrameNumber * 2.0f),
			glm::vec3(0, 1, 0));

	GPUCameraData camData;
	camData.proj = projection;
	camData.view = view;
	camData.viewProj = projection * view;

	void* data;
	vmaMapMemory(m_Allocator, m_FrameData.cameraBuffer.allocation, &data);
	memcpy(data, &camData, sizeof(GPUCameraData));
	vmaUnmapMemory(m_Allocator, m_FrameData.cameraBuffer.allocation);

	float framed = (m_FrameNumber / 120.f);
	m_SceneParameters.ambientColor = { sin(framed), 0, cos(framed), 1 };

	char* sceneData;
	vmaMapMemory(m_Allocator, m_SceneParameterBuffer.allocation,
		(void**)&sceneData);

	int frameIndex = m_FrameNumber % FRAME_OVERLAP;
	sceneData += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
	memcpy(sceneData, &m_SceneParameters, sizeof(GPUSceneData));
	vmaUnmapMemory(m_Allocator, m_SceneParameterBuffer.allocation);

	void* objectData;
	vmaMapMemory(m_Allocator, m_FrameData.objectBuffer.allocation, &objectData);

	GPUObjectData* objectSSBO = (GPUObjectData*)objectData;

	for (int i = 0; i < count; i++) {
		RenderObject& object = first[i];
		objectSSBO[i].modelMatrix = object.transformMatrix * rotMat;
	}

	vmaUnmapMemory(m_Allocator, m_FrameData.objectBuffer.allocation);

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (int i = 0; i < count; i++) {
		RenderObject& object = first[i];

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			object.material->pipeline);
		if (object.material != lastMaterial) {

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				object.material->pipeline);
			lastMaterial = object.material;

			uint32_t uniformOffset =
				pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				object.material->pipelineLayout, 0, 1,
				&m_FrameData.globalDescriptor, 1, &uniformOffset);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				object.material->pipelineLayout, 1, 1,
				&m_FrameData.objectDescriptor, 0, nullptr);

			if (object.material->textureSet != VK_NULL_HANDLE) {
				// Texture &tex = m_Textures["empire_diffuse"];
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
					object.material->pipelineLayout, 2, 1,
					&object.material->textureSet, 0, nullptr);
				//&tex.descriptorSet, 0, nullptr);
			}
		}

		/* glm::mat4 model = object.transformMatrix * rotMat; */
		/* glm::mat4 meshMatrix = projection * view * model; */

		/*
	MeshPushConstants constants;
	constants.renderMatrix = object.transformMatrix * rotMat;

	// upload the mesh to the GPU via push constants
	vkCmdPushConstants(cmd, object.material->pipelineLayout,
						 VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
						 &constants);
		*/

		if (object.mesh != lastMesh) {
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer,
				&offset);
			lastMesh = object.mesh;
		}

		vkCmdDraw(cmd, (uint32_t)object.mesh->vertices.size(), 1, 0,
			i); // TODO: look at first instance
	}
}

void VulkanEngine::cleanup() {
	if (m_IsInitialized) {

		VK_CHECK(vkDeviceWaitIdle(m_Device));

		m_MainDeletionQueue.flush();

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
}

void VulkanEngine::draw() {
	// wait for last frame to finish. Timeout of 1 second
	VK_CHECK(
		vkWaitForFences(m_Device, 1, &m_FrameData.renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_Device, 1, &m_FrameData.renderFence));

	// since we waited the buffer is empty
	VK_CHECK(vkResetCommandBuffer(m_FrameData.mainCommandBuffer, 0));

	// we will write to this image index (framebuffer)
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX,
		m_FrameData.presentSemaphore, nullptr,
		&swapchainImageIndex));
	// uint32_t swapchainImageIndex = m_WindowData.FrameIndex;

	// ImGui_ImplVulkanH_Frame* fd =
	// &m_WindowData.Frames[m_WindowData.FrameIndex];

	VkCommandBuffer cmd = m_FrameData.mainCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue clearValue{};
	clearValue.color = { {0.0f, 0.0f, 0.0, 1.0f} };

	VkClearValue depthClear{};
	depthClear.depthStencil.depth = 1.f;

	{
		VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(
			m_RenderPass, m_WindowExtent,
			m_FrameBuffers[swapchainImageIndex] /*fd->Framebuffer*/);

		rpInfo.clearValueCount = 2;
		VkClearValue clearValues[] = { clearValue, depthClear };
		/* VkClearValue clearValues[] = {clearValue, depthClear}; */
		rpInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		draw_objects(cmd, m_RenderObjects.data(), (int)m_RenderObjects.size());

		vkCmdEndRenderPass(cmd);
	}

	{
		VkRenderPassBeginInfo rpInfo =
			vkinit::renderpass_begin_info(m_ImGuiRenderPass, m_WindowExtent,
				m_ImGuiFrameBuffers[swapchainImageIndex]);

		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRenderPass(cmd);
	}

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);

	VkPipelineStageFlags waitStage =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submitInfo.pWaitDstStageMask = &waitStage;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_FrameData.presentSemaphore;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_FrameData.renderSemaphore;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VK_CHECK(
		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_FrameData.renderFence));

	VkPresentInfoKHR presentInfo = vkinit::present_info();

	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &m_FrameData.renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

	m_FrameNumber++;
}

void VulkanEngine::run() {

	while (!glfwWindowShouldClose(m_Window)) {
		glfwPollEvents();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();
		ImGui::Begin("Texture Viewer");
		Texture& tex = m_Textures["empire_diffuse"];
		ImGui::Image((ImTextureID)tex.descriptorSet, ImVec2(500, 500));
		// ImGui::Image((ImTextureID)mat->textureSet, ImVec2(500, 500));
		ImGui::End();

		draw();

		ImGuiIO& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
}

VkBool32
spdlog_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
	VkDebugUtilsMessageTypeFlagsEXT msgType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*) {
	auto mt = vkb::to_string_message_type(msgType);

	switch (msgSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		CORE_TRACE("({}) {}", mt, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		CORE_INFO("({}) {}", mt, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		CORE_WARN("({}) {}", mt, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		CORE_ERROR("({}) {}", mt, pCallbackData->pMessage);
		break;
	}

	return VK_FALSE;
}

void VulkanEngine::init_vulkan() {
	vkb::Instance vkb_inst = vkb::InstanceBuilder()
		.set_app_name("Vulkan Application")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.set_debug_callback(spdlog_debug_callback)
		.build()
		.value();

	m_Instance = vkb_inst.instance;
	m_DebugMessenger = vkb_inst.debug_messenger;

	VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface));

	vkb::PhysicalDevice physicalDevice = vkb::PhysicalDeviceSelector(vkb_inst)
		.set_minimum_version(1, 1)
		.set_surface(m_Surface)
		.select()
		.value();

	VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
	shaderDrawParametersFeatures.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shaderDrawParametersFeatures.pNext = nullptr;
	shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;
	vkb::Device vkbDevice = vkb::DeviceBuilder(physicalDevice)
		.add_pNext(&shaderDrawParametersFeatures)
		.build()
		.value();

	m_GPUProperties = vkbDevice.physical_device.properties;
	CORE_INFO("The GPU has a minimum buffer alignment of {}",
		m_GPUProperties.limits.minUniformBufferOffsetAlignment);

	m_Device = vkbDevice.device;
	m_PhysicalDevice = physicalDevice.physical_device;

	m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily =
		vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_Device;
	allocatorInfo.instance = m_Instance;
	vmaCreateAllocator(&allocatorInfo, &m_Allocator);

	m_MainDeletionQueue.push_function([=]() {
		vmaDestroyAllocator(m_Allocator);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
		vkDestroyInstance(m_Instance, nullptr);
		});
}

void VulkanEngine::init_swapchain() {

	vkb::Swapchain vkbSwapchain =
		vkb::SwapchainBuilder(m_PhysicalDevice, m_Device, m_Surface)
		//.use_default_format_selection()
		//.set_desired_format()
		//.set_desired_format({VK_FORMAT_R8G8B8A8_UNORM
		//,VK_COLORSPACE_SRGB_NONLINEAR_KHR})
		.set_desired_format(
			{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR })
		//.set_desired_format({VK_FORMAT_R8G8B8A8_UNORM,VK_COLORSPACE_SRGB_NONLINEAR_KHR})
		//.add_fallback_format({VK_FORMAT_R8G8B8A8_UNORM,VK_COLORSPACE_SRGB_NONLINEAR_KHR})
		// set vsync
		//.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		//.set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
		.set_desired_extent(m_WindowExtent.width, m_WindowExtent.height)
		.build()
		.value();

	m_Swapchain = vkbSwapchain.swapchain;
	m_SwapchainImages = vkbSwapchain.get_images().value();
	m_SwapchainImageViews = vkbSwapchain.get_image_views().value();

	m_SwapchainImageFormat = vkbSwapchain.image_format;
	CORE_INFO("Swachain Format: {}", m_SwapchainImageFormat);

	// m_Swapchain = m_WindowData.Swapchain;

	VkExtent3D depthImageExtent = { m_WindowExtent.width, m_WindowExtent.height,
								   1 };

	m_DepthFormat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo dimgInfo = vkinit::image_create_info(
		m_DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		depthImageExtent);

	VmaAllocationCreateInfo dimgAllocinfo{};
	dimgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimgAllocinfo.requiredFlags =
		VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // TODO:

	vmaCreateImage(m_Allocator, &dimgInfo, &dimgAllocinfo, &m_DepthImage.image,
		&m_DepthImage.allocation, nullptr);

	VkImageViewCreateInfo dviewInfo = vkinit::imageview_create_info(
		m_DepthFormat, m_DepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(m_Device, &dviewInfo, nullptr, &m_DepthImageView));

	m_MainDeletionQueue.push_function([=]() {
		vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
		vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		});
}

void VulkanEngine::init_commands() {
	VkCommandPoolCreateInfo cmdPoolInfo = vkinit::command_pool_create_info(
		m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	{
		VK_CHECK(vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr,
			&m_FrameData.commandPool));

		VkCommandBufferAllocateInfo cmdAllocInfo =
			vkinit::command_buffer_allocate_info(m_FrameData.commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo,
			&m_FrameData.mainCommandBuffer));

		m_MainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(m_Device, m_FrameData.commandPool, nullptr);
			});
	}

	VkCommandPoolCreateInfo uploadCommandPoolInfo =
		vkinit::command_pool_create_info(m_GraphicsQueueFamily);

	VK_CHECK(vkCreateCommandPool(m_Device, &uploadCommandPoolInfo, nullptr,
		&m_UploadContext.commandPool));

	VkCommandBufferAllocateInfo cmdAllocInfo =
		vkinit::command_buffer_allocate_info(m_UploadContext.commandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo,
		&m_UploadContext.commandBuffer));

	m_MainDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(m_Device, m_UploadContext.commandPool, nullptr);
		});
}

void VulkanEngine::init_default_renderpass() {
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		// attachment number will index into the pAttachments array in the
		// parent renderpass
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.flags = 0;
		depthAttachment.format = m_DepthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency depthDependency = {};
		depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.dstSubpass = 0;
		depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.srcAccessMask = 0;
		depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
		VkSubpassDependency dependencies[2] = { dependency, depthDependency };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = &attachments[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = &dependencies[0];

		VK_CHECK(
			vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));
	}

	{
		VkAttachmentDescription attachment = {};
		attachment.format = m_SwapchainImageFormat;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment = {};
		color_attachment.attachment = 0;
		color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = 1;
		info.pAttachments = &attachment;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;
		VK_CHECK(vkCreateRenderPass(m_Device, &info, nullptr, &m_ImGuiRenderPass));
	}

	m_MainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		vkDestroyRenderPass(m_Device, m_ImGuiRenderPass, nullptr);
		});
}

void VulkanEngine::init_framebuffers() {
	VkFramebufferCreateInfo fbInfo =
		vkinit::framebuffer_create_info(m_RenderPass, m_WindowExtent);

	VkFramebufferCreateInfo imguiFbInfo =
		vkinit::framebuffer_create_info(m_ImGuiRenderPass, m_WindowExtent);

	const uint32_t swapchainImagecount =
		static_cast<uint32_t>(m_SwapchainImages.size());
	m_FrameBuffers = std::vector<VkFramebuffer>(swapchainImagecount);
	m_ImGuiFrameBuffers = std::vector<VkFramebuffer>(swapchainImagecount);

	// create framebuffers for each of the swapchain image views
	for (uint32_t i = 0; i < swapchainImagecount; i++) {
		VkImageView attachments[2];
		attachments[0] = m_SwapchainImageViews[i];
		attachments[1] = m_DepthImageView;

		fbInfo.pAttachments = attachments;
		fbInfo.attachmentCount = 2;

		imguiFbInfo.pAttachments = attachments;
		imguiFbInfo.attachmentCount = 1;

		VK_CHECK(
			vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &m_FrameBuffers[i]));

		VK_CHECK(vkCreateFramebuffer(m_Device, &imguiFbInfo, nullptr,
			&m_ImGuiFrameBuffers[i]));

		m_MainDeletionQueue.push_function([=]() {
			vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
			vkDestroyFramebuffer(m_Device, m_ImGuiFrameBuffers[i], nullptr);

			vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
			});
	}
}

void VulkanEngine::init_sync_structures() {
	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();

	VK_CHECK(vkCreateFence(m_Device, &uploadFenceCreateInfo, nullptr,
		&m_UploadContext.uploadFence));
	m_MainDeletionQueue.push_function([=]() {
		vkDestroyFence(m_Device, m_UploadContext.uploadFence, nullptr);
		});

	VkFenceCreateInfo fenceCreateInfo =
		vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr,
		&m_FrameData.renderFence));

	m_MainDeletionQueue.push_function(
		[=]() { vkDestroyFence(m_Device, m_FrameData.renderFence, nullptr); });

	VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
		&m_FrameData.presentSemaphore));
	VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr,
		&m_FrameData.renderSemaphore));

	m_MainDeletionQueue.push_function([=]() {
		vkDestroySemaphore(m_Device, m_FrameData.presentSemaphore, nullptr);
		vkDestroySemaphore(m_Device, m_FrameData.renderSemaphore, nullptr);
		});
}

void VulkanEngine::init_pipelines() {

	{
		VkShaderModule texturedMeshFragShader;
		if (!vkutil::load_spirv_shader_module("res/shaders/textured_lit.frag.spv",
			&texturedMeshFragShader, m_Device)) {
			return;
		}
		else {
			CORE_INFO("textured_mesh fragment shader successfully loaded");
		}

		VkShaderModule texturedMeshVertexShader;
		if (!vkutil::load_spirv_shader_module("res/shaders/textured_mesh.vert.spv",
			&texturedMeshVertexShader,
			m_Device)) {
			return;
		}
		else {
			CORE_INFO("textured_mesh vertex shader successfully loaded");
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo =
			vkinit::pipeline_layout_create_info();

		VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr,
			&m_TrianglePipelineLayout));

		VkPipelineLayoutCreateInfo meshPipelineLayoutInfo =
			vkinit::pipeline_layout_create_info();

		/* VkPushConstantRange pushConstant; */
		/* pushConstant.offset = 0; */
		/* pushConstant.size = sizeof(MeshPushConstants); */
		/* pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; */

		VkDescriptorSetLayout setLayouts[] = { m_GlobalSetLayout, m_ObjectSetLayout,
											  m_SingleTextureSetLayout };

		/* meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant; */
		/* meshPipelineLayoutInfo.pushConstantRangeCount = 1; */
		meshPipelineLayoutInfo.setLayoutCount = 3;
		meshPipelineLayoutInfo.pSetLayouts = setLayouts;

		VK_CHECK(vkCreatePipelineLayout(m_Device, &meshPipelineLayoutInfo, nullptr,
			&m_MeshPipelineLayout));

		VertexInputDescription vertexDescription = Vertex::get_vertex_description();

		m_MeshPipeline =
			vkinit::PipelineBuilder(m_Device, m_RenderPass, m_MeshPipelineLayout)
			.set_vertex_description(vertexDescription.attributes,
				vertexDescription.bindings)
			.add_shader_module(texturedMeshVertexShader,
				vkutil::ShaderType::Vertex)
			.add_shader_module(texturedMeshFragShader,
				vkutil::ShaderType::Fragment)
			.set_viewport({ 0, 0 }, m_WindowExtent, { 0.0f, 1.0f })
			.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
			.build()
			.value();

		create_material(m_MeshPipeline, m_MeshPipelineLayout, "defaultmaterial");

		/* m_MeshPipeline = pipelineBuilder.build(); */

		vkDestroyShaderModule(m_Device, texturedMeshFragShader, nullptr);
		vkDestroyShaderModule(m_Device, texturedMeshVertexShader, nullptr);
	}

	m_MainDeletionQueue.push_function([=]() {
		vkDestroyPipeline(m_Device, m_MeshPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_TrianglePipelineLayout, nullptr);
		vkDestroyPipelineLayout(m_Device, m_MeshPipelineLayout, nullptr);
		});
}

void VulkanEngine::load_meshes() {
	m_MonkeyMesh = load_mesh_from_obj("res/models/lost_empire.obj").value();

	m_TriangleMesh.vertices.resize(3);
	m_TriangleMesh.vertices[0].position = { 0.5f, 0.5f, 0.0f };
	m_TriangleMesh.vertices[1].position = { -0.5f, 0.5f, 0.0f };
	m_TriangleMesh.vertices[2].position = { 0.f, -0.5f, 0.0f };

	m_TriangleMesh.vertices[0].color = { 1.f, 0.f, 0.0f };
	m_TriangleMesh.vertices[1].color = { 0.f, 1.f, 0.0f };
	m_TriangleMesh.vertices[2].color = { 0.f, 0.f, 1.0f };

	upload_mesh(m_MonkeyMesh);
	upload_mesh(m_TriangleMesh);

	m_Meshes["empire"] = m_MonkeyMesh;
	m_Meshes["triangle"] = m_TriangleMesh;
}

void VulkanEngine::upload_mesh(Mesh& mesh) {

	const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);

	VkBufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	/* stagingBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; */

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	/* vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO; */
	/* vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
	 * | */
	 /*                      VMA_ALLOCATION_CREATE_MAPPED_BIT; */

	 /* VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo, */
	 /*                          &mesh.vertexBuffer.buffer, */
	 /*                          &mesh.vertexBuffer.allocation, nullptr)); */

	 /* m_MainDeletionQueue.push_function([=]() { */
	 /*   vmaDestroyBuffer(m_Allocator, mesh.vertexBuffer.buffer, */
	 /*                    mesh.vertexBuffer.allocation); */
	 /* }); */

	AllocatedBuffer stagingBuffer;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &vmaAllocInfo,
		&stagingBuffer.buffer, &stagingBuffer.allocation,
		nullptr));
	void* data;
	vmaMapMemory(m_Allocator, stagingBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), bufferSize);
	vmaUnmapMemory(m_Allocator, stagingBuffer.allocation);

	VkBufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	vertexBufferInfo.size = bufferSize;
	vertexBufferInfo.usage =
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &vertexBufferInfo, &vmaAllocInfo,
		&mesh.vertexBuffer.buffer,
		&mesh.vertexBuffer.allocation, nullptr));

	immediate_submit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1,
			&copy);
		});

	m_MainDeletionQueue.push_function([=]() {
		vmaDestroyBuffer(m_Allocator, mesh.vertexBuffer.buffer,
			mesh.vertexBuffer.allocation);
		});

	vmaDestroyBuffer(m_Allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void VulkanEngine::init_scene() {
	RenderObject empire;
	empire.mesh = get_mesh("empire");
	empire.material = get_material("defaultmaterial");
	empire.transformMatrix = glm::translate(glm::vec3{ 5, -10, 0 });

	m_RenderObjects.push_back(empire);

	Material* texturedMat = get_material("defaultmaterial");

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_SingleTextureSetLayout;

	vkAllocateDescriptorSets(m_Device, &allocInfo, &texturedMat->textureSet);

	VkSamplerCreateInfo samplerInfo =
		vkinit::sampler_create_info(VK_FILTER_NEAREST);

	VkSampler blockySampler;
	vkCreateSampler(m_Device, &samplerInfo, nullptr, &blockySampler);

	m_MainDeletionQueue.push_function(
		[=]() { vkDestroySampler(m_Device, blockySampler, nullptr); });

	VkDescriptorImageInfo imageBufferInfo;
	imageBufferInfo.sampler = blockySampler;
	imageBufferInfo.imageView = m_Textures["empire_diffuse"].imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet texture1 = vkinit::write_descriptor_image(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMat->textureSet,
		&imageBufferInfo, 0);

	vkUpdateDescriptorSets(m_Device, 1, &texture1, 0, nullptr);

	/* for (int x = -20; x <= 20; x++) { */
	/*   for (int y = -20; y <= 20; y++) { */

	/*     RenderObject tri{}; */
	/*     tri.mesh = get_mesh("triangle"); */
	/*     tri.material = get_material("defaultmaterial"); */
	/*     glm::mat4 translation = */
	/*         glm::translate(glm::mat4{1.0}, glm::vec3(x, 0, y)); */
	/*     glm::mat4 scale = glm::scale(glm::mat4{1.0}, glm::vec3(0.2, 0.2, 0.2));
	 */
	 /*     tri.transformMatrix = translation * scale; */

	 /*     m_RenderObjects.push_back(tri); */
	 /*   } */
	 /* } */
}

void VulkanEngine::init_descriptors() {

	std::vector<VkDescriptorPoolSize> sizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10} };

	// create a descriptor pool that will hold 10 uniform buffers
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = (uint32_t)sizes.size();
	poolInfo.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);

	{
		VkDescriptorSetLayoutBinding cameraBind =
			vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT, 0);
		VkDescriptorSetLayoutBinding sceneBind =
			vkinit::descriptorset_layout_binding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

		VkDescriptorSetLayoutBinding bindings[] = { cameraBind, sceneBind };

		/* VkDescriptorSetLayoutBinding camBufferBinding{}; */
		/* camBufferBinding.binding = 0; */
		/* camBufferBinding.descriptorCount = 1; */
		/* camBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; */
		/* camBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; */

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.pNext = nullptr;
		setInfo.bindingCount = 2;
		setInfo.flags = 0;
		setInfo.pBindings = bindings;

		vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr,
			&m_GlobalSetLayout);
	}

	{
		VkDescriptorSetLayoutBinding objectBind =
			vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT, 0);

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.pNext = nullptr;
		setInfo.bindingCount = 1;
		setInfo.flags = 0;
		setInfo.pBindings = &objectBind;

		vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr,
			&m_ObjectSetLayout);
	}

	{
		VkDescriptorSetLayoutBinding textureBind =
			vkinit::descriptorset_layout_binding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT, 0);

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.pNext = nullptr;
		setInfo.bindingCount = 1;
		setInfo.flags = 0;
		setInfo.pBindings = &textureBind;

		vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr,
			&m_SingleTextureSetLayout);
	}

	const size_t sceneParamBufferSize =
		FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));

	m_SceneParameterBuffer =
		create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);

	const int MAX_OBJECTS = 10000; // TODO: dynamic object buffer?

	m_FrameData.objectBuffer = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	m_FrameData.cameraBuffer =
		create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_GlobalSetLayout;

	vkAllocateDescriptorSets(m_Device, &allocInfo, &m_FrameData.globalDescriptor);

	VkDescriptorSetAllocateInfo objectAlloc{};
	objectAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	objectAlloc.pNext = nullptr;
	objectAlloc.descriptorPool = m_DescriptorPool;
	objectAlloc.descriptorSetCount = 1;
	objectAlloc.pSetLayouts = &m_ObjectSetLayout;

	vkAllocateDescriptorSets(m_Device, &objectAlloc,
		&m_FrameData.objectDescriptor);

	VkDescriptorBufferInfo cameraInfo;
	cameraInfo.buffer = m_FrameData.cameraBuffer.buffer;
	cameraInfo.offset = 0;
	cameraInfo.range = sizeof(GPUCameraData);

	VkDescriptorBufferInfo sceneInfo;
	sceneInfo.buffer = m_SceneParameterBuffer.buffer;
	sceneInfo.offset = 0;
	sceneInfo.range = sizeof(GPUSceneData);

	VkDescriptorBufferInfo objectBufferInfo;
	objectBufferInfo.buffer = m_FrameData.objectBuffer.buffer;
	objectBufferInfo.offset = 0;
	objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

	VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_buffer(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_FrameData.globalDescriptor,
		&cameraInfo, 0);

	VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_buffer(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_FrameData.globalDescriptor,
		&sceneInfo, 1);

	VkWriteDescriptorSet objectWrite = vkinit::write_descriptor_buffer(
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_FrameData.objectDescriptor,
		&objectBufferInfo, 0);

	VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite, objectWrite };

	/* VkDescriptorBufferInfo bufferInfo{}; */
	/* bufferInfo.buffer = m_Frames[i].cameraBuffer.buffer; */
	/* bufferInfo.offset = 0; */
	/* bufferInfo.range = sizeof(GPUCameraData); */

	/* VkWriteDescriptorSet setWrite{}; */
	/* setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; */
	/* setWrite.pNext = nullptr; */
	/* setWrite.dstBinding = 0; */
	/* setWrite.dstSet = m_Frames[i].globalDescriptor; */
	/* setWrite.descriptorCount = 1; */
	/* setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; */
	/* setWrite.pBufferInfo = &bufferInfo; */

	vkUpdateDescriptorSets(m_Device, 3, setWrites, 0, nullptr);

	m_MainDeletionQueue.push_function([&]() {
		vmaDestroyBuffer(m_Allocator, m_SceneParameterBuffer.buffer,
			m_SceneParameterBuffer.allocation);

		vkDestroyDescriptorSetLayout(m_Device, m_GlobalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_ObjectSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_SingleTextureSetLayout, nullptr);

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

		vmaDestroyBuffer(m_Allocator, m_FrameData.objectBuffer.buffer,
			m_FrameData.objectBuffer.allocation);
		vmaDestroyBuffer(m_Allocator, m_FrameData.cameraBuffer.buffer,
			m_FrameData.cameraBuffer.allocation);
		});
}

void VulkanEngine::init_imgui() {
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = {
		{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000} };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable
	// Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
	// Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport /
														// Platform Windows
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

	io.FontGlobalScale = 1.0f;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplGlfw_InitForVulkan(m_Window, true);

	// this initializes imgui for Vulkan
	{
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = m_Instance;
		initInfo.PhysicalDevice = m_PhysicalDevice;
		initInfo.Device = m_Device;
		initInfo.Queue = m_GraphicsQueue;
		initInfo.DescriptorPool = imguiPool;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		ImGui_ImplVulkan_Init(&initInfo, m_ImGuiRenderPass);
	}

	{
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF(
			(void*)c_RobotoRegular, sizeof(c_RobotoRegular), 23.0f, &fontConfig);
		io.FontDefault = robotoFont;

		immediate_submit(
			[&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });
	}

	// clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ImGui::SetOneDarkTheme();

	// add the destroy the imgui created structures
	m_MainDeletionQueue.push_function([=]() {
		vkDestroyDescriptorPool(m_Device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});
}

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize) {
	size_t minUboAlignment =
		m_GPUProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}

void VulkanEngine::immediate_submit(
	std::function<void(VkCommandBuffer cmd)>&& function) {
	VkCommandBuffer cmd = m_UploadContext.commandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);

	VK_CHECK(
		vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_UploadContext.uploadFence));

	vkWaitForFences(m_Device, 1, &m_UploadContext.uploadFence, true, 9999999999);
	vkResetFences(m_Device, 1, &m_UploadContext.uploadFence);

	vkResetCommandPool(m_Device, m_UploadContext.commandPool, 0);
}

void VulkanEngine::load_images() {
	/*
	Texture lostEmpire;

	int w, h, nC;
	if (!vkutil::load_alloc_image_from_file("res/images/lost_empire-RGBA.png",
	*this, &lostEmpire.imageBuffer, &w, &h, &nC)) { CORE_WARN("could not load
	image!"); return;
	}
	else {
			CORE_INFO("successfully loaded image: lost_empire-RGBA.png");
	}

	VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(
			VK_FORMAT_R8G8B8A8_SRGB, lostEmpire.imageBuffer.image,
			VK_IMAGE_ASPECT_COLOR_BIT);

	vkCreateImageView(m_Device, &imageInfo, nullptr, &lostEmpire.imageView);
	*/

	m_Textures["empire_diffuse"] =
		vkutil::load_const_texture("res/images/lost_empire-RGBA.png", *this)
		.value();

	// m_MainDeletionQueue.push_function([=]() {
	//	vkDestroyImageView(m_Device, lostEmpire.imageView, nullptr);
	//	});
}
