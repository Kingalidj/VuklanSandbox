#include "render_api.h"
#include "application.h"

#include "vk_engine.h"

namespace Atlas {

	namespace RenderApi {

		void begin(Texture &color, Texture &depth, glm::vec4 clearColor)
		{
			Application::get_engine().begin_renderpass(*color.get_native_texture(), *depth.get_native_texture(), clearColor);
		}

		void begin(Texture &color, glm::vec4 clearColor)
		{
			Application::get_engine().begin_renderpass(*color.get_native_texture(), clearColor);
		}

		void end(Texture &color)
		{
			Application::get_engine().end_renderpass(*color.get_native_texture());
		}

		void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
		{
			VkCommandBuffer cmd = Application::get_engine().get_active_command_buffer();
			vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}
	}

}
