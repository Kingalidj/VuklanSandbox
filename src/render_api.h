#pragma once

#include "texture.h"
#include <glm/glm.hpp>

namespace Atlas {

	namespace RenderApi {
		void begin(Ref<Texture> color, Ref<Texture> depth, Color clearColor);
		void begin(Ref<Texture> color, Color clearColor, bool clearScreen = false);
		void end();

		void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
	}
}
