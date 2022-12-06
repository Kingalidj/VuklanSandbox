#pragma once
#include <glm/glm.hpp>
#include "texture.h"

namespace Atlas {

	class Camera;

	namespace Render2D {

		struct Vertex {
			glm::vec3 position;
			glm::vec4 color;
			glm::vec2 uv;
		};

		void init();
		void cleanup();
		void set_camera(Camera &camera);

		void draw_test_triangle();

		void begin(Texture &color, Texture &depth, glm::vec4 &clearColor);
		void begin(Texture &color, glm::vec4 &clearColor);
		void end(Texture &color);

	}

}
