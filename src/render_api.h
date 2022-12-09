#pragma once

#include "texture.h"
#include <glm/glm.hpp>

namespace Atlas {

	namespace RenderApi {
		void begin(Texture &color, Texture &depth, glm::vec4 clearColor);
		void begin(Texture &color, glm::vec4 clearColor);
		void end(Texture &color);

		void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
	}
}
