#pragma once
#include <glm/glm.hpp>

namespace Atlas {

	namespace Render2D {

		struct Vertex {
			glm::vec3 position;
			glm::vec4 color;
			glm::vec2 uv;
		};

		void init();
		void cleanup();

		void draw_test_triangle();

	}

}
